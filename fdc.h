// fdc.h
// Lİ-DOS Disket Sürücüsü Modulu Arayuzu (BIOS int 13h)
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: BIOS int 13h kullanarak disket okuma/yazma.

#ifndef _FDC_H
#define _FDC_H

#include "types.h" // uint8_t, uint16_t, uint32_t, size_t gibi
#include "asm.h"   // bios_disk_io fonksiyonu icin
#include "fs.h"    // SECTOR_SIZE gibi sabitler icin (eger uygunsa)

// BIOS Disket Sürücüsü ID'leri
#define FLOPPY_DRIVE_A 0x00
#define FLOPPY_DRIVE_B 0x01

// FDC modülünü baslatir.
// Donus degeri: 0 basari, -1 hata.
int fdc_init(void);

// Disketten sektor(ler) okur.
// drive: Sürücü ID'si (FLOPPY_DRIVE_A/B).
// lba: Okunacak ilk sektorun LBA adresi.
// count: Okunacak sektor sayisi (maks 255).
// buffer_segment: Verinin yazilacagi bufferin segment adresi.
// buffer_offset: Verinin yazilacagi bufferin offset adresi.
// Donus degeri: 0 basari, BIOS hata kodu.
uint8_t fdc_read_sectors(uint8_t drive, uint32_t lba, uint8_t count, uint16_t buffer_segment, uint16_t buffer_offset);

// Diskete sektor(ler) yazar.
// drive: Sürücü ID'si (FLOPPY_DRIVE_A/B).
// lba: Yazilacak ilk sektorun LBA adresi.
// count: Yazilacak sektor sayisi (maks 255).
// buffer_segment: Verinin okunacagi bufferin segment adresi.
// buffer_offset: Verinin okunacagi bufferin offset adresi.
// Donus degeri: 0 basari, BIOS hata kodu.
uint8_t fdc_write_sectors(uint8_t drive, uint32_t lba, uint8_t count, uint16_t buffer_segment, uint16_t buffer_offset);

// LBA adresini CHS adresine çevirir (Disket icin)
// Bu hesaplama, disketin geometrisini bilmeyi gerektirir (ornegin 1.44MB icin 80 silindir, 2 kafa, 18 sektor/iz).
// Bu fonksiyon inst_disk.S ornegindeki LBA->CHS mantigiyla aynidir ama disket parametreleri kullanir.
// inst_disk.S'teki bios_disk_io CHS bekliyorsa bu gereklidir.
// inst_disk.S LBA bekliyorsa gerek yok.
// Varsayalim bios_disk_io CHS bekliyor, bu durumda bu C fonksiyonu gereklidir.
void lba_to_chs_fdc(uint32_t lba, uint16_t *cylinder, uint8_t *head, uint8_t *sector);


#endif // _FDC_H