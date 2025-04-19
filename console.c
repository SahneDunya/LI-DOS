// console.c
// Lİ-DOS Konsol Modulu Implementasyonu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89

#include "console.h" // Konsol arayuzu ve sabitler
// Düşük seviye G/Ç fonksiyonları için bildirimler (asm.S'teki fonksiyonlar)
// Bu fonksiyonlarin baska bir baslik dosyasinda (ornegin asm.h) bildirildigini varsayalim
extern void outb(uint16_t port, uint8_t value);
// extern uint8_t inb(uint16_t port); // Ornekte kullanilmiyor ama gerekirse eklenir

// Konsolun durumu (global degiskenler)
static uint16_t cursor_x = 0; // Imlecin sutun pozisyonu (0-79)
static uint16_t cursor_y = 0; // Imlecin satir pozisyonu (0-24)
static uint8_t current_attribute = DEFAULT_ATTRIBUTE; // Aktif yazi ozelligi

// Video bellegine byte bazinda erisim icin pointer
// C'de 16-bit real mode'da far pointer kullanimi derleyiciye ozgu olabilir.
// Ornekte basit bir cast kullanildi, derleyicinizin desteklediginden emin olun.
// Daha saglam bir yol, segment ve offset'i ayri tutup hesaplama yapmaktir.
static uint8_t *vram_byte_ptr = (uint8_t *)0xB8000;


// Dahili yardimci fonksiyonlar
static void scroll(void);
static void update_cursor(void);


// Konsol sistemini baslatir
void console_init(void) {
    cursor_x = 0;
    cursor_y = 0;
    current_attribute = DEFAULT_ATTRIBUTE;
    console_clear(); // Ekrani temizle
    update_cursor();   // Imleci guncelle
}

// Ekrani varsayilan ozellikle temizler
void console_clear(void) {
    console_clear_with_attribute(DEFAULT_ATTRIBUTE);
}

// Belirtilen ozellikle ekrani temizler
void console_clear_with_attribute(uint8_t attribute) {
    int i;
    uint16_t cell_value = (attribute << 8) | ' '; // Bosluk karakteri ve ozelligi

    // VRAM'i bosluk ve ozellik degeriyle doldur
    // Her karakter 2 byte: [ASCII, Attribute]
    // VRAM boyutu: CONSOLE_WIDTH * CONSOLE_HEIGHT * 2
    for (i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
        // VRAM_ADDRESS bir uint16_t pointer oldugu icin, her i artisi 2 byte ilerler.
        VRAM_ADDRESS[i] = cell_value;
    }

    // Imleci baslangica al
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}


// Tek bir karakteri isler ve ekrana yazar
void console_putc(char c) {
    uint16_t offset;

    // Ozel karakterleri isle
    switch (c) {
        case '\n': // Yeni satir
            cursor_x = 0;
            cursor_y++;
            break;
        case '\r': // Satir basi
            cursor_x = 0;
            break;
        case '\b': // Geri silme (Backspace)
            if (cursor_x > 0) {
                cursor_x--;
            } else if (cursor_y > 0) {
                // Satir basindaysa bir ust satirin sonuna git
                cursor_y--;
                cursor_x = CONSOLE_WIDTH - 1;
            }
            // Silinen karakterin yerine bosluk yaz
            offset = (cursor_y * CONSOLE_WIDTH + cursor_x) * 2;
            vram_byte_ptr[offset] = ' ';
            vram_byte_ptr[offset + 1] = current_attribute;
            break;
        case '\t': // Tab
            // Imleci bir sonraki 8'in katı sütununa taşı
            cursor_x = (cursor_x + 8) & ~(8 - 1);
            if (cursor_x >= CONSOLE_WIDTH) {
                cursor_x = 0;
                cursor_y++;
            }
            break;
        // Diger ozel karakterler (ornegin form feed, vertical tab vb.) burada islenebilir
        default:
            // Normal karakter
            offset = (cursor_y * CONSOLE_WIDTH + cursor_x) * 2;
            vram_byte_ptr[offset] = (uint8_t)c;
            vram_byte_ptr[offset + 1] = current_attribute;
            cursor_x++;
            break;
    }

    // Satir sonuna gelindiyse bir alt satira gec
    if (cursor_x >= CONSOLE_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    // Ekran dolduysa kaydir
    if (cursor_y >= CONSOLE_HEIGHT) {
        scroll();
    }

    // Donanim imlecini guncelle
    update_cursor();
}

// Null terminated string'i ekrana yazar
void console_puts(const char *s) {
    while (*s) { // Null terminator karakterine gelene kadar
        console_putc(*s++); // Karakteri yaz ve pointeri ilerlet
    }
}

// Belirtilen buffer icerigini ekrana yazar
void void console_write(const char *buf, size_t count) {
    size_t i;
    for (i = 0; i < count; i++) {
        console_putc(buf[i]);
    }
}


// Konsolun yazi ozelligini ayarlar
void console_set_attribute(uint8_t attribute) {
    current_attribute = attribute;
    // Not: Mevcut ekrandaki karakterlerin ozellikleri degismez,
    // sadece bu noktadan sonra yazilan karakterler yeni ozelligi kullanir.
    // Tum ekrani yeni ozellikle tekrar yazmak istenirse, ekran temizlenip
    // icerik yeniden cizilmelidir (bu daha karmasik bir islem gerektirir).
}


// --- Dahili Fonksiyonlar ---

// Ekrani bir satir yukari kaydirir
static void scroll(void) {
    int i;
    uint16_t row_size_bytes = CONSOLE_WIDTH * 2; // Bir satirin byte cinsinden boyutu
    uint16_t screen_size_bytes = CONSOLE_WIDTH * CONSOLE_HEIGHT * 2; // Ekranin byte cinsinden boyutu

    // Ikinci satirdan baslayarak her satiri bir ust satira kopyala
    // (CONSOLE_HEIGHT - 1) kadar satir kopyalanir (Son satir haric)
    for (i = 0; i < CONSOLE_HEIGHT - 1; i++) {
        // Hedef adres: VRAM'in baslangici + i. satirin adresi
        // Kaynak adres: VRAM'in baslangici + (i+1). satirin adresi
        // row_size_bytes kadar byte kopyala
        // memcpy yerine manuel kopyalama dongusu (standart kutuphane kullanilmayacak)
        uint8_t *dest = vram_byte_ptr + i * row_size_bytes;
        uint8_t *src  = vram_byte_ptr + (i + 1) * row_size_bytes;
        size_t j;
        for(j = 0; j < row_size_bytes; j++) {
            dest[j] = src[j];
        }
    }

    // Son satiri (artik en alttaki bos satir) mevcut ozellikle bosluklarla doldur
    uint8_t *last_row_ptr = vram_byte_ptr + (CONSOLE_HEIGHT - 1) * row_size_bytes;
    uint16_t cell_value = (current_attribute << 8) | ' ';
    for (i = 0; i < CONSOLE_WIDTH; i++) {
         // uint16_t pointer gibi erisim daha kolay olabilir bu dongu icin
         ((uint16_t *)last_row_ptr)[i] = cell_value;
    }

    // Imleci en alt satirin basina al
    cursor_y = CONSOLE_HEIGHT - 1;
    cursor_x = 0;
    // update_cursor() console_putc icinde cagrilacagi icin burada tekrar cagirmaya gerek yok.
    // Ancak scroll islemi direk cagrilirsa update_cursor() sonra cagrilmali.
}

// Donanim imlecini guncel pozisyona gore ayarlar
static void update_cursor(void) {
    uint16_t cursor_location = cursor_y * CONSOLE_WIDTH + cursor_x;

    // Imlecin yuksek byte'ini ayarla (Register 0x0E)
    outb(CRT_CTRL_REG_PORT, CRT_CURSOR_HIGH_REG);
    outb(CRT_DATA_REG_PORT, (uint8_t)(cursor_location >> 8)); // Cursor konumunun yuksek 8 bitini gonder

    // Imlecin dusuk byte'ini ayarla (Register 0x0F)
    outb(CRT_CTRL_REG_PORT, CRT_CURSOR_LOW_REG);
    outb(CRT_DATA_REG_PORT, (uint8_t)(cursor_location & 0xFF)); // Cursor konumunun dusuk 8 bitini gonder
}

// console.c sonu