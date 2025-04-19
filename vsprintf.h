// vsprintf.h
// Lİ-DOS Kernel Formatli Cikti Modulu Arayuzu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: vsprintf benzeri formatlama fonksiyonu saglamak.

#ifndef _VSPRINTF_H
#define _VSPRINTF_H

// Gerekli temel tureler
// Kernelinizde baska bir baslik dosyasinda (types.h gibi) olabilir.
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef int int32_t;
typedef uint16_t size_t; // 16-bit ortamda size_t genellikle uint16_t olabilir.

// Variadic argumanlar icin gerekli tipler ve makrolar.
// Bu basligin (ornegin va_list.h) kernelinizde tanimli oldugunu varsayiyoruz.
#include "va_list.h" // Kernelin custom va_list tanimlarini iceren baslik

// Kernel Safe vsprintf (vsnprintf benzeri) fonksiyonu.
// Formatli ciktıyı 'buf' bufferina 'size' byte'i gecmeyecek sekilde yazar.
// fmt: Format stringi.
// args: va_list'teki argumanlar.
// Donus degeri: Buffer'a yazilan karakter sayisi (NULL terminator haric).
// Buffer'in 'size' byte'tan kisa olmasi durumunda tasmayi onler.
int vsprintf(char *buf, size_t size, const char *fmt, va_list args);

// vsprintf'in eski signature'ina benzer bir macro (istege bagli, dikkatli kullanin!)
 #define sprintf(buf, fmt, ...) vsprintf(buf, 0xFFFF, fmt, ##__VA_ARGS__) // Size kontrolu zayif
 #define snprintf(buf, size, fmt, ...) vsprintf(buf, size, fmt, ##__VA_ARGS__) // Daha guvenli

#endif // _VSPRINTF_H