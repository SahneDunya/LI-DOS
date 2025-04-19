// lpt.h
// Lİ-DOS Paralel Port Modulu Arayuzu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Paralel port uzerinden basit cikti (örn. yaziciya).

#ifndef _LPT_H
#define _LPT_H

#include "types.h" // uint8_t, uint16_t gibi
#include "asm.h"   // outb fonksiyonu icin
// Sure implementasyonu icin (opsiyonel)
 #include "sys.h"

// Yaygin Paralel Port Base Adresleri
#define LPT1_PORT 0x378
#define LPT2_PORT 0x278
#define LPT3_PORT 0x3BC (Genellikle Mono VGA ile iliskili)

// Paralel Port Kayitcik Offsetleri (Base Port'a Gore)
#define LPT_DATA_PORT     0 // Veri Kayitcigi (R/W)
#define LPT_STATUS_PORT   1 // Durum Kayitcigi (R) - Error, Paper Out, Busy, Ack, Online
#define LPT_CONTROL_PORT  2 // Kontrol Kayitcigi (W) - Strobe, AutoFeed, Init, Select In, IRQ Enable

// Paralel Port modülünü baslatir.
// port_base: Kullanilacak portun base adresi (örn. LPT1_PORT).
// Donus degeri: 0 basari, -1 hata.
int lpt_init(uint16_t port_base);

// Belirtilen paralel porta tek karakter (byte) gonderir.
// port_base: Kullanilacak portun base adresi.
// c: Gonderilecek karakter/byte.
// Donus degeri: 0 basari, -1 hata.
int lpt_putc(uint16_t port_base, uint8_t c);

// Belirtilen paralel porta null terminated string (byte dizisi) gonderir.
// port_base: Kullanilacak portun base adresi.
// s: Gonderilecek string/byte dizisi.
void lpt_puts(uint16_t port_base, const char *s);


#endif // _LPT_H