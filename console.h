// console.h
// Lİ-DOS Konsol Modulu Arayuzu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Metin konsoluna cikti yazmak, imleci yonetmek.

#ifndef _CONSOLE_H
#define _CONSOLE_H

// Gerekli tureler icin (ornegin size_t yerine) basit typedef'ler
// Gercek projenizde bu turler baska bir kernel-wide baslik dosyasinda olabilir (ornegin types.h)
typedef unsigned int size_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

// Konsol boyutlari
#define CONSOLE_WIDTH 80
#define CONSOLE_HEIGHT 25

// Metin modunda video belleği (VRAM) adresi
// CGA/EGA/VGA renkli metin modu: Segment 0xB800, Offset 0x0000
#define VRAM_SEGMENT 0xB800
#define VRAM_ADDRESS ((uint16_t *)0xB8000) // Ornek: Segment:Offset olarak 0xB800:0x0000

// Varsayilan yazi ozelligi (attribute) - Ornegin acik gri uzerine siyah arka plan
#define DEFAULT_ATTRIBUTE 0x07

// CRT Controller Port Adresleri (CGA/EGA/VGA icin ortak)
#define CRT_CTRL_REG_PORT 0x3D4 // Control Register Port
#define CRT_DATA_REG_PORT 0x3D5 // Data Register Port

// CRT Register Indexleri (Imlec konumu icin)
#define CRT_CURSOR_HIGH_REG 0x0E // Imlec konumu yuksek byte registeri
#define CRT_CURSOR_LOW_REG  0x0F // Imlec konumu dusuk byte registeri

// Konsol modülünün fonksiyon prototipleri

// Konsol sistemini baslatir (VRAM'i temizler, imleci baslangica alir)
void console_init(void);

// Ekrani temizler
void console_clear(void);

// Belirtilen ozellikle ekrani temizler
void console_clear_with_attribute(uint8_t attribute);

// Tek bir karakteri imlecin oldugu konuma yazar, ozel karakterleri isler (\n, \r, \b, \t)
void console_putc(char c);

// Null terminated string'i ekrana yazar
void console_puts(const char *s);

// Belirtilen buffer icerigini (count kadar karakter) ekrana yazar
void console_write(const char *buf, size_t count);

// Konsolun yazi ozelligini (renk, arka plan, yanip sonme) ayarlar
void console_set_attribute(uint8_t attribute);

// Imleci guncel pozisyona gore donanim uzerinde ayarlar
void update_cursor(void); // Dahili kullanim icin, disariya acilmayabilir ama ornekte gosterildi

#endif // _CONSOLE_H