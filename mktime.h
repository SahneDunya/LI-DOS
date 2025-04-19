// mktime.h
// Lİ-DOS Zaman Dönüşüm Modulu Arayuzu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Yapısal zamani (struct tm) saniye cinsinden takvim zamanina donusturmek.

#ifndef _MKTIME_H
#define _MKTIME_H

// Gerekli tureler icin basit typedef'ler
// Kernelinizde baska bir baslik dosyasinda (types.h gibi) olabilir.
typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

// time_t tipinin tanimi - Epoch'tan bu yana saniye
// 32-bit olmasi 2038 yilini asmak icin kritiktir.
typedef uint32_t time_t;

// Yapısal Zaman (Broken-down time) yapısı
// Standart C'deki struct tm'ye benzer.
// tm_year: Yıl (1900'den itibaren fark)
// tm_mon: Ay (0-11, 0=Ocak)
// tm_mday: Ayin gunu (1-31)
// tm_hour: Saat (0-23)
// tm_min: Dakika (0-59)
// tm_sec: Saniye (0-59)
// tm_wday: Haftanin gunu (0-6, 0=Pazar) - mktime hesaplayabilir veya göz ardı edilebilir
// tm_yday: Yilin gunu (0-365) - mktime hesaplayabilir veya göz ardı edilebilir
// tm_isdst: Yaz saati uygulaması (0=Hayir, 1=Evet, -1=Bilinmiyor) - Basit implementasyonda goz ardi edilebilir
struct tm {
    int tm_sec;   // Saniyeler (0-59)
    int tm_min;   // Dakikalar (0-59)
    int tm_hour;  // Saatler (0-23)
    int tm_mday;  // Ayin gunu (1-31)
    int tm_mon;   // Ay (0-11)
    int tm_year;  // Yil (1900'den itibaren)
    int tm_wday;  // Haftanin gunu (0-6) - genelde mktime tarafindan hesaplanir
    int tm_yday;  // Yilin gunu (0-365) - genelde mktime tarafindan hesaplanir
    int tm_isdst; // Yaz saati uygulaması flag (-1, 0, 1) - genelde mktime goz ardi edilebilir
};

// Epoch baslangici: 1 Ocak 1970 00:00:00 UTC

// struct tm'de verilen yapısal zamani, epoch'tan bu yana saniye cinsinden time_t'ye donusturur.
// tm_isdst degeri bu basit implementasyonda dikkate alinmayabilir.
// tm_wday ve tm_yday alanlarini hesaplayip struct'a yazabilir veya goz ardi edebilir.
time_t mktime(struct tm *tm_ptr);

#endif // _MKTIME_H