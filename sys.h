// sys.h
// Lİ-DOS Genel Sistem Modulu Arayuzu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Kernel genelinde kullanilan tanimlar ve merkezi baslatma.

#ifndef _SYS_H
#define _SYS_H

// Gerekli temel tureler
// Kernelinizde baska bir baslik dosyasinda (types.h gibi) olabilir.
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef int int32_t;

// Kernel tarafindan desteklenen maksimum RAM (Tasarim limiti)
#define MAX_OS_SUPPORTED_RAM_KB 64 // 64 KB

// Temel kernel durum/hata kodlari (ornektir)
#define SYS_STATUS_OK       0
#define SYS_STATUS_ERROR    -1
#define SYS_STATUS_BUSY     1
// ... Diger genel durumlar eklenebilir

// Kernel baslangic fonksiyonu
// Assembly head kodu tarafindan C'ye gecildiginde cagirilan ilk C fonksiyonu.
// Diger kernel modullerini baslatir ve sistemi calismaya hazirlar.
void sys_init(void); // veya kernel_main(void);

// Sistemle ilgili temel bilgiler donduren fonksiyonlar (ornektir)
// uint32_t get_total_memory(void); // Tespit edilen toplam bellek (implementasyonu mm veya baska yerde olabilir)
// const char* get_cpu_model(void); // CPU modelini dondur (cok basit olabilir)

// Kernelin durdurulmasi icin bir fonksiyon (panik disinda kontrollu kapatma icin)
// void sys_shutdown(void);

#endif // _SYS_H