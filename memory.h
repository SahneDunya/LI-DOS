// memory.h
// Lİ-DOS Bellek Yönetimi (MM) Modulu Arayuzu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: 64KB adres alani icindeki bellegi tahsis ve serbest birakma.

#ifndef _MEMORY_H
#define _MEMORY_H

// Gerekli temel tureler
// Kernelinizde baska bir baslik dosyasinda (types.h gibi) olabilir.
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t; // 32-bit hesaplamalar icin
typedef uint16_t size_t; // 16-bit ortamda size_t genellikle uint16_t olabilir.

// Bellek havuzundaki bos bloklari yonetmek icin yapi.
// Bu yapi, bir bos bellek blogunun basinda yer alir.
// next field'i ayni 64KB segment icindeki baska bir bos blogun offset adresini tutar.
struct free_block_header {
    size_t size; // Blogun boyutu (header dahil, byte cinsinden)
    struct free_block_header *next; // Sonraki bos bloga pointer (ayni segment icinde offset)
    // Padding veya hizalama icin ek alan gerekebilir.
     size_t dummy; // Ornegin, 8 byte'lik hizalama icin
};

// Bellek Yönetimi modülünü baslatir.
// mem_pool_start: Yönetilecek bellek havuzunun başlangıç adresi (ayni 64KB segment icindeki offset).
// mem_pool_size: Yönetilecek bellek havuzunun boyutu (byte).
// Not: mem_pool_start ve mem_pool_size ile belirtilen alan kernel veri segmenti icinde olmalidir.
void mm_init(void *mem_pool_start, size_t mem_pool_size);

// Belirtilen boyutta bellek tahsis eder.
// size: Tahsis edilecek bellek boyutu (byte).
// Donus degeri: Tahsis edilen bellek blogunun baslangic adresi (pointer) veya basarisiz olursa NULL.
// Donen pointer, mm_free ile serbest birakilabilir.
void *mm_alloc(size_t size);

// Daha once mm_alloc ile tahsis edilmis bir bellek blogunu serbest birakir.
// ptr: Serbest birakilacak bellek blogunun adresi (mm_alloc tarafindan dondurulen pointer).
void mm_free(void *ptr);

// Stack icin bellek tahsis eder. Genellikle mm_alloc ile aynidir, ancak
// bazi allocatorlarda stackler icin ozel yonetim olabilir (orn. hizalama, buyume yonu).
// Bu basit ornekte mm_alloc ile aynidir, ancak stackler genellikle yuksek adreslerden baslar.
void *mm_alloc_stack(size_t size);


#endif // _MEMORY_H