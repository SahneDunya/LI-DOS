// boot_sector_bin.h
// Lİ-DOS Boot Sektörü İkili Verisi
// Yazar: Sahne Dünya
// Amac: Kurulum programinin hedef diske yazacagi boot sektoru imaji.

#ifndef _BOOT_SECTOR_BIN_H
#define _BOOT_SECTOR_BIN_H

#include "inst.h" // SECTOR_SIZE tanimi icin

// Lİ-DOS boot sektörü ikili verisi (Derlenmiş boot/boot.bin dosyası)
// Bu array, derlenmiş boot sektörünüzün içeriğiyle doldurulmalıdır.
// Ornek: { 0xEB, 0x3C, 0x90, ... boot kodu ve VBPB alanlari ... 0x55, 0xAA }
unsigned char boot_sector_data[SECTOR_SIZE] = {
    // Buraya derlenmis boot/boot.bin dosyasinin 512 byte'lik ham verisi kopyalanacak.
    // Ornegin, bir baska tool ile boot.bin dosyasini okuyup bu arrayi olusturabilirsiniz.
    // Ornek placeholder verisi (gecersiz boot sektoru):
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, // 0-15
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, // 16-31
    // ... 512 byte olana kadar devam et ...
    // Boot Signature (Offset 510 ve 511)
    // ... 0x00, 0x00, 0x00, 0x00, ... , 0x00, 0x00, 0x55, 0xAA // Son 2 byte
};

#endif // _BOOT_SECTOR_BIN_H