// hd.h
// Lİ-DOS Sabit Disk (HD) Modulu Arayuzu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: BIOS int 13h kullanarak diskten sektor okuma/yazma.

#ifndef _HD_H
#define _HD_H

// Gerekli tureler icin (ornegin size_t yerine) basit typedef'ler
// Gercek projenizde bu turler baska bir kernel-wide baslik dosyasinda olabilir (ornegin types.h)
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t; // LBA icin gerekebilir, simdilik CHS kullanacagiz ama tipi taniyabiliriz.

// Sabitler
#define SECTOR_SIZE 512         // Standart disk sektor boyutu (byte)
#define HD_PRIMARY_DRIVE 0x80   // BIOS: İlk sabit disk sürücüsü
#define HD_SECONDARY_DRIVE 0x81 // BIOS: İkinci sabit disk sürücüsü

// BIOS int 13h fonksiyon kodlari
#define BIOS_READ_SECTORS  0x02
#define BIOS_WRITE_SECTORS 0x03
// ... Diger int 13h fonksiyonlari (Get Parameters, Reset, vb.) eklenebilir

// BIOS int 13h hata kodlari (AH registerinda donerse)
#define BIOS_ERR_NO_ERROR          0x00
#define BIOS_ERR_INVALID_COMMAND   0x01
#define BIOS_ERR_ADDRESS_MARK_NOT_FOUND 0x02
#define BIOS_ERR_WRITE_PROTECTED   0x03
#define BIOS_ERR_SECTOR_NOT_FOUND  0x04
#define BIOS_ERR_RESET_FAILED      0x05
#define BIOS_ERR_DISK_CHANGED      0x06
#define BIOS_ERR_BAD_PARAM         0x07 // Bad parameter (e.g., invalid CHS for drive)
#define BIOS_ERR_DMA_BOUNDARY      0x09 // DMA boundary error (shouldn't happen with buffer below 1MB)
#define BIOS_ERR_BAD_SECTOR        0x0A // Bad sector detected
#define BIOS_ERR_BAD_TRACK         0x0B // Bad track detected
#define BIOS_ERR_MEDIA_TYPE        0x0C // Media type not found
#define BIOS_ERR_SECTOR_COUNT_ERROR 0x0D // Invalid sector count
#define BIOS_ERR_DMA_ERROR         0x10 // DMA error
#define BIOS_ERR_ECC_BAD_DISK      0x11 // ECC error
#define BIOS_ERR_ECC_CORRECTED     0x12 // ECC corrected error
#define BIOS_ERR_CONTROLLER_ERROR  0x20 // Controller error
#define BIOS_ERR_SEEK_ERROR        0x40 // Seek error
#define BIOS_ERR_TIMEOUT           0x80 // Timeout
#define BIOS_ERR_DRIVE_NOT_READY   0xAA // Drive not ready
#define BIOS_ERR_UNDEFINED_ERROR   0xBB // Undefined error
#define BIOS_ERR_NO_MEDIA          0xFF // No media present

// HD modülünün fonksiyon prototipleri

// Disk sistemini baslatir (simdilik pek bir sey yapmayabilir)
void hd_init(void);

// Belirtilen surucuden (drive) CHS adresine (cylinder, head, sector)
// 'count' adet sektoru 'buffer_segment:buffer_offset' adresine okur.
// Donus degeri: BIOS hata kodu (0 basari).
// Not: buffer_segment ve buffer_offset 16-bit Real Mode adresin segment ve offset kısımlarıdır.
uint8_t hd_read_sectors_chs(uint8_t drive, uint16_t cylinder, uint8_t head, uint8_t sector, uint8_t count, uint16_t buffer_segment, uint16_t buffer_offset);

// Belirtilen surucuye (drive) CHS adresine (cylinder, head, sector)
// 'count' adet sektoru 'buffer_segment:buffer_offset' adresinden yazar.
// Donus degeri: BIOS hata kodu (0 basari).
// Not: buffer_segment ve buffer_offset 16-bit Real Mode adresin segment ve offset kısımlarıdır.
uint8_t hd_write_sectors_chs(uint8_t drive, uint16_t cylinder, uint8_t head, uint8_t sector, uint8_t count, uint16_t buffer_segment, uint16_t buffer_offset);

// Belki LBA fonksiyonlari da eklenebilir (BIOS EHCI veya AHCI desteklerse 286+ uzerinde)
// uint8_t hd_read_sectors_lba(uint8_t drive, uint32_t lba, uint8_t count, uint16_t buffer_segment, uint16_t buffer_offset);
// uint8_t hd_write_sectors_lba(uint8_t drive, uint32_t lba, uint8_t count, uint16_t buffer_segment, uint16_t buffer_offset);

#endif // _HD_H