// memory.c
// Lİ-DOS Bellek Yönetimi (MM) Modulu Implementasyonu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: 64KB adres alani icindeki bellegi tahsis ve serbest birakma (Heap).

#include "memory.h" // Bellek Yönetimi arayuzu
// Debug cikti icin (opsiyonel)
#include "printk.h"
// Bellek kopyalama/doldurma gibi temel fonksiyonlar (string.h yerine)
// Bu fonksiyonlarin baska bir kernel yardimci modülünde (örn. lib.c veya asm.S)
// implemente edilip burada bildirildigini varsayalim.
extern void *memcpy(void *dest, const void *src, size_t n);
extern void *memset(void *s, int c, size_t n);

// --- Dahili Degiskenler ---

// Bos bellek bloklari listesinin basi.
// Bu liste, yönetilen bellek havuzu icindeki bos bloklari tutar.
static struct free_block_header *free_list_head = (struct free_block_header *)0;

// Yönetilen bellek havuzunun baslangic ve bitis adresleri.
// Bu adresler ayni 64KB segment icindeki offsetlerdir.
static void *mem_pool_start_addr = (void *)0;
static void *mem_pool_end_addr = (void *)0;


// --- Dahili Yardimci Fonksiyonlar ---

// Bellek blogu boyutunu hizalama gereksinimine gore yuvarlar.
// Ornegin, header boyutu veya 4/8 byte hizalama.
// Bu ornekte basitlik icin sadece header boyutunu dikkate alalim ve 2 byte hizalama varsayalim.
#define ALIGN_SIZE 2 // 2 byte hizalama
#define HEADER_SIZE sizeof(struct free_block_header)
// Allocated data must be aligned after the header
#define BLOCK_OVERHEAD ((HEADER_SIZE + ALIGN_SIZE - 1) & ~(ALIGN_SIZE - 1))

static size_t align_size(size_t size) {
    // size'i ALIGN_SIZE'in katina yuvarla
    return (size + ALIGN_SIZE - 1) & ~(ALIGN_SIZE - 1);
}


// Bellek Yönetimi modülünü baslatir.
void mm_init(void *mem_pool_start, size_t mem_pool_size) {
    // Bellek havuzu gecerlilik kontrolu (NULL degil ve boyutu yeterli)
    if (!mem_pool_start || mem_pool_size < BLOCK_OVERHEAD) {
        // printk("MM init error: Invalid memory pool parameters.\n");
        return;
    }

    // Yönetilen havuzun adreslerini kaydet
    mem_pool_start_addr = mem_pool_start;
    mem_pool_end_addr = (uint8_t *)mem_pool_start + mem_pool_size;

    // Tum havuzu tek bir bos blok olarak baslat
    free_list_head = (struct free_block_header *)mem_pool_start;
    free_list_head->size = mem_pool_size;
    free_list_head->next = (struct free_block_header *)0; // Listenin sonu
    // printk("MM initialized. Pool from %p to %p, size %u bytes.\n", mem_pool_start_addr, mem_pool_end_addr, mem_pool_size);
}

// Belirtilen boyutta bellek tahsis eder.
void *mm_alloc(size_t size) {
    struct free_block_header *current = free_list_head;
    struct free_block_header *prev = (struct free_block_header *)0;

    if (size == 0) return (void *)0; // 0 boyutunda tahsis yapma

    // Tahsis edilecek toplam boyut = istenen boyut + header boyutu + hizalama
    size_t total_alloc_size = align_size(size) + BLOCK_OVERHEAD;

    // Bos listeyi dolas (ilk uygun blogu bul)
    while (current) {
        if (current->size >= total_alloc_size) {
            // Yeterli buyuklukte bir bos blok bulundu

            // Blogu bolmek gerekiyorsa (eger kalan alan yeni bir bos blok icin yeterliyse)
            size_t remaining_size = current->size - total_alloc_size;
            if (remaining_size >= BLOCK_OVERHEAD) {
                // Blogu bol: Yeni bos blok, tahsis edilen blogun hemen sonrasinda baslar.
                struct free_block_header *new_free_block = (struct free_block_header *)((uint8_t *)current + total_alloc_size);
                new_free_block->size = remaining_size;
                new_free_block->next = current->next;

                // Tahsis edilen blogun boyutunu ayarla
                current->size = total_alloc_size;

                // Bos listede guncelleme yap: Yeni bos blogu mevcut blogun yerine koy.
                if (prev) {
                    prev->next = new_free_block;
                } else {
                    free_list_head = new_free_block; // Ilk blok bolundu
                }
            } else {
                // Blogu bolmek icin yeterli yer yok, blogun tamamini tahsis et.
                // Bu blogu bos listeden cikar.
                if (prev) {
                    prev->next = current->next;
                } else {
                    free_list_head = current->next; // Ilk blok tahsis edildi
                }
                 // Tahsis edilen blogun boyutu zaten dogru (current->size)
            }

            // Tahsis edilen bellek blogunun veri alaninin adresini dondur (header'dan sonra)
             printk("Allocated %u bytes at %p\n", size, (uint8_t *)current + BLOCK_OVERHEAD);
            return (uint8_t *)current + BLOCK_OVERHEAD;
        }

        // Siradaki bos bloga gec
        prev = current;
        current = current->next;
    }

    // Yeterli buyuklukte bos blok bulunamadi
     printk("MM alloc failed: Not enough memory for %u bytes.\n", size);
    return (void *)0;
}

// Daha once mm_alloc ile tahsis edilmis bir bellek blogunu serbest birakir.
void mm_free(void *ptr) {
    // NULL pointer veya yonetilen havuz disindaki pointerlar icin kontrol
    if (!ptr || ptr < mem_pool_start_addr || ptr >= mem_pool_end_addr) {
        // printk("MM free error: Invalid pointer %p\n", ptr);
        return;
    }

    // Serbest birakilan blogun header adresini al
    struct free_block_header *freed_block = (struct free_block_header *)((uint8_t *)ptr - BLOCK_OVERHEAD);

    // Serbest birakilan blogu bos listeye ekle
    // Basitlik icin listenin basina ekleyelim (Daha gelismis allocatorlar adres sirasina gore ekler ve birlestirme yapar)
    freed_block->next = free_list_head;
    free_list_head = freed_block;

    // !!! BIRLESTIRME (COALESCING) !!!
    // Gercek bir allocator'da, serbest birakilan blogun kendisinden onceki veya sonraki
    // bos bloklarla birlestirilmesi (merge edilmesi) gereklidir.
    // Bu, bellek parcalanmasini (fragmentasyon) azaltir.
    // Birlestirme implementasyonu, bellek adreslerine gore sirali bir bos liste veya
    // bloklarin boyut bilgisini kullanarak onceki/sonraki blogu bulma yontemi gerektirir.
    // Bu basit ornekte birlestirme implemente edilmemistir.
     printk("Freed block at %p\n", ptr);
}

// Stack icin bellek tahsis eder. Bu ornekte mm_alloc ile aynidir.
void *mm_alloc_stack(size_t size) {
     // Stackler genellikle yuksek adreslerden baslar.
     // mm_alloc dusuk adres dondurur, bu adres stackin en altidir.
     // Stack pointer (SP) genellikle stackin en ustune (en yuksek adrese) ayarlanir.
     // Yani dondurulen pointer + size, stack pointer olarak kullanilmalidir.
    return mm_alloc(size); // Basitlik icin mm_alloc ile aynisi
}


// --- page.S tarafindan cagrilabilecek veya kullanilabilecek fonksiyonlar ---
// Eger page.S assembly'de bellek probing vb. yapiyorsa,
// buradan mm_init veya baska mm fonksiyonlarini cagirabilir.
// Ornekte page.S sadece basit register fonksiyonlari iceriyor.


// memory.c sonu