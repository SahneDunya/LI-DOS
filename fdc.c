// fdc.c
// Lİ-DOS Disket Sürücüsü Modulu Implementasyonu (BIOS int 13h)
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: BIOS int 13h kullanarak disket okuma/yazma.

#include "fdc.h"
#include "printk.h" // Debug cikti icin

// --- Disket Geometrisi (1.44MB Disket icin Ornek) ---
// Farkli disket formatlari icin bu degerler degisir.
// BIOS int 13h AH=08h (Get Drive Parameters) ile bu bilgiler alinabilir.
#define FDC_SECTORS_PER_TRACK 18
#define FDC_NUM_HEADS         2
#define FDC_NUM_CYLINDERS     80 // (0-79)

// LBA adresini CHS adresine çevirir (Disket icin 1.44MB ornegi)
void lba_to_chs_fdc(uint32_t lba, uint16_t *cylinder, uint8_t *head, uint8_t *sector) {
    if (!cylinder || !head || !sector) return;

    *sector = (uint8_t)((lba % FDC_SECTORS_PER_TRACK) + 1); // Sektor 1-based
    uint32_t temp_cyl = lba / FDC_SECTORS_PER_TRACK;
    *head = (uint8_t)(temp_cyl % FDC_NUM_HEADS); // Head 0-based
    *cylinder = (uint16_t)(temp_cyl / FDC_NUM_HEADS); // Cylinder 0-based
}


int fdc_init(void) {
    // BIOS int 13h AH=08h (Get Drive Parameters) ile disket sürücülerini kontrol etme
    // veya sadece sürücü ID'leri 0x00 ve 0x01'in var oldugunu varsayma.
    // Basitlik icin var oldugunu varsayalim.
    printk("FDC Init: Disket sürücüleri 0x00 ve 0x01 varsayiliyor.\r\n");
    return 0;
}

// Disketten sektor(ler) okur.
uint8_t fdc_read_sectors(uint8_t drive, uint32_t lba, uint8_t count, uint16_t buffer_segment, uint16_t buffer_offset) {
    uint16_t cylinder;
    uint8_t head, sector;
    uint8_t error;

    // LBA -> CHS çevrimini yap
    lba_to_chs_fdc(lba, &cylinder, &head, &sector);

    // BIOS int 13h cagrisini yap (asm.h'taki bios_disk_io fonksiyonunu kullanarak)
    // bios_disk_io parametre sirasi: command, count, cylinder, head, sector, buffer_segment, buffer_offset, drive
    error = bios_disk_io(0x02, count, cylinder, head, sector, buffer_segment, buffer_offset, drive);

    if (error) {
         printk("FDC Read Error: Drive 0x%x, LBA 0x%lx, Count %u, Error 0x%x\r\n", drive, lba, count, error);
    } else {
         printk("FDC Read OK: Drive 0x%x, LBA 0x%lx, Count %u\r\n", drive, lba, count);
    }

    return error; // BIOS hata kodunu dondur
}

// Diskete sektor(ler) yazar.
uint8_t fdc_write_sectors(uint8_t drive, uint32_t lba, uint8_t count, uint16_t buffer_segment, uint16_t buffer_offset) {
    uint16_t cylinder;
    uint8_t head, sector;
    uint8_t error;

    // LBA -> CHS çevrimini yap
    lba_to_chs_fdc(lba, &cylinder, &head, &sector);

     // BIOS int 13h cagrisini yap
     error = bios_disk_io(0x03, count, cylinder, head, sector, buffer_segment, buffer_offset, drive);

     if (error) {
         printk("FDC Write Error: Drive 0x%x, LBA 0x%lx, Count %u, Error 0x%x\r\n", drive, lba, count, error);
     } else {
          // printk("FDC Write OK: Drive 0x%x, LBA 0x%lx, Count %u\r\n", drive, lba, count);
     }

     return error; // BIOS hata kodunu dondur
}

// fdc.c sonu