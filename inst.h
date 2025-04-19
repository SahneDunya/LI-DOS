// inst.h
// Lİ-DOS Kurulum Programı Arayuzu (Güncellenmiş)
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Temel Kurulum, MBR/Partition destekli hedefleme, boot sektor yamalama.

#ifndef _INST_H
#define _INST_H

#include "types.h" // uint8_t, uint16_t, uint32_t, size_t gibi
#include "asm.h"   // cli, hlt icin (opsiyonel)

// Temel sabitler (aynı kaldı)
#define SECTOR_SIZE 512
#define TARGET_DRIVE_ID 0x80 // Hedef Disk (İlk Hard Disk)
#define KERNEL_FILENAME "KERNEL.BIN" // Kurulum medyasındaki çekirdek dosyasının adı (8.3 format)

// MBR (Master Boot Record) Yapısı (Disk LBA 0)
// İlk 440 byte boot code, 4 byte disk signature, 2 byte reserved, 64 byte Partition Table, 2 byte Boot Signature
#define MBR_PARTITION_TABLE_OFFSET 0x1BE // MBR icindeki Partition Table'in offseti
#define MBR_BOOT_SIGNATURE_OFFSET  0x1FE // MBR icindeki Boot Signature'in offseti (0xAA55)

// Partition Table Girişi Yapısı (16 byte)
struct __attribute__((packed)) partition_entry { // packed non-standard C89
    uint8_t boot_indicator;     // Bootable (0x80) or Not Bootable (0x00)
    uint8_t start_head;         // Starting Head for CHS
    uint8_t start_sector;       // Starting Sector for CHS (bits 0-5) & high bits of Cylinder (bits 6-7)
    uint8_t start_cylinder_high; // Starting Cylinder (bits 8-9) for CHS
    uint8_t system_id;          // Partition Type (e.g., 0x01=FAT12, 0x04=FAT16 <32MB, 0x06=FAT16 >=32MB)
    uint8_t end_head;           // Ending Head for CHS
    uint8_t end_sector;         // Ending Sector for CHS & high bits of Cylinder
    uint8_t end_cylinder_high;  // Ending Cylinder
    uint32_t start_lba;         // Starting LBA
    uint32_t total_sectors;     // Total Sectors in Partition
};

// --- Kurulum Programı Fonksiyonları (Aynı Kaldı) ---
extern void c_installer_main(void);
extern void inst_putc(char c);
extern void inst_puts(const char *s);
extern char inst_getc(void);
extern int inst_checkkey(void);

// --- Güncellenmiş Disk I/O Fonksiyonları (inst_disk.S'te) ---
// Bu fonksiyonlar disk geometrisini (SPT, Heads) kullanacak
extern uint8_t inst_read_sectors(uint8_t drive, uint32_t lba, uint8_t count, uint16_t buffer_segment, uint16_t buffer_offset);
extern uint8_t inst_write_sectors(uint8_t drive, uint32_t lba, uint8_t count, uint16_t buffer_segment, uint16_t buffer_offset);
// Yeni: Sürücü parametrelerini (geometri) oku (int 13h AH=08h)
// spt ve heads değerlerini bir yapıda veya global değişkenlerde dondurebilir.
// Basitlik icin global degiskenlere yazsin.
extern uint8_t inst_get_drive_params(uint8_t drive); // Global SPT, Heads'i ayarlar. Hata kodu dondurur.

// --- Boot Sektörü Yamalama Sabitleri ---
// Bootloader kodunuzun KERNEL LBA ve Boyutunu beklediği offsetler.
// Bunlar boot/boot.S kodunuza göre belirlenmelidir!
#define BOOT_SECTOR_KERNEL_LBA_OFFSET _0xXXXX // Örnek: Boot sektöründe LBA'nın yazilacagi offset
#define BOOT_SECTOR_KERNEL_SIZE_OFFSET_0xYYYY // Örnek: Boot sektöründe boyutun yazilacagi offset

// Boot sektörü ikili verisi (boot_sector_bin.h'ta tanımlanır)
extern unsigned char boot_sector_data[SECTOR_SIZE]; // Derlenmis Lİ-DOS boot sektoru sablonu


#endif // _INST_H