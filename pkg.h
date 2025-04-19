// pkg.h
// Lİ-DOS Paket Yöneticisi Arayuzu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Minimal yazilim paketi kurulumu ve listeleme.

#ifndef _PKG_H
#define _PKG_H

// Gerekli temel tureler
#include "types.h" // uint8_t, uint16_t, uint32_t, size_t gibi

// Gerekli diger kernel modulleri
#include "fs.h"    // Dosya sistemi erisimi icin (yazma destekli olmali!)
#include "tty_io.h" // Kullanici G/Ç'si icin (veya printk)

// Paket formatı sabitleri
#define PKG_MAGIC        0x4C49504B // "LIPK" veya baska bir sihirli sayi
#define PKG_NAME_MAX     31        // Paket adi icin maks uzunluk (null haric)
#define PKG_VERSION_MAX  15        // Versiyon icin maks uzunluk (null haric)
#define PKG_FILENAME_MAX 63        // Paket icindeki dosya adi/yolu icin maks uzunluk (null haric)
#define PKG_MAX_FILES    32        // Bir paketteki maks dosya sayisi

// Paket veritabanı dosyası yolu
#define PKG_DB_PATH      "/PACKAGES.LST"

// Paket Dosyası Header Yapısı (Disk üzerindeki paketin başı)
struct __attribute__((packed)) pkg_header { // packed non-standard C89, manuel okuma tercih edilebilir.
    uint32_t magic;
    char pkg_name[PKG_NAME_MAX + 1];
    char pkg_version[PKG_VERSION_MAX + 1];
    uint16_t num_files;
    // Dosya girdileri hemen burada başlar (sayısı num_files kadar)
};

// Paket Dosyası İçindeki Dosya Girişi Yapısı
struct __attribute__((packed)) pkg_file_entry { // packed non-standard C89, manuel okuma tercih edilebilir.
    char filename[PKG_FILENAME_MAX + 1];
    uint32_t file_size;
    uint32_t data_offset; // Paketin basindan bu dosyanin verisine olan offset.
};

// Paket Yöneticisi durum/hata kodları
#define PKG_STATUS_OK             0
#define PKG_STATUS_ERROR          -1
#define PKG_STATUS_INVALID_PACKAGE -2
#define PKG_STATUS_FILE_ERROR     -3
#define PKG_STATUS_DB_ERROR       -4
#define PKG_STATUS_ALREADY_INSTALLED -5
#define PKG_STATUS_NOT_IMPLEMENTED   -6 // Yazma destegi yoksa

// Paket yöneticisi modülünü baslatir (Veritabanını kontrol eder/oluşturur).
void pkg_init(void);

// Belirtilen paketi (dosya yolu) kurar.
// package_filepath: Kurulacak paket dosyasinin yolu (örn. "/A/MYAPP.PKG").
// Donus degeri: PKG_STATUS_x sabitlerinden biri.
int pkg_install(const char *package_filepath);

// Kurulu paketlerin bir listesini ekrana yazar.
// Donus degeri: PKG_STATUS_x sabitlerinden biri.
int pkg_list_installed(void);

// Belirtilen isme sahip paketin kurulu olup olmadığını kontrol eder.
// package_name: Kontrol edilecek paket adi.
// Donus degeri: 1 kurulu ise, 0 kurulu degilse, <0 hata ise.
int pkg_is_installed(const char *package_name);

// Belirtilen paketi kaldırır (minimal implementasyonda desteklenmez).
 int pkg_remove(const char *package_name);

#endif // _PKG_H