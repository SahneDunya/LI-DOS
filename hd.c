// hd.c
// Lİ-DOS Sabit Disk Modulu Implementasyonu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89

#include "hd.h" // HD modulu arayuzu
// BIOS int 13h cagrisi icin Assembly yardimci fonksiyonu bildirimi
extern uint8_t bios_disk_io(uint8_t command, uint8_t count, uint16_t cylinder, uint8_t head, uint8_t sector, uint16_t buffer_segment, uint16_t buffer_offset, uint8_t drive);
// Gerekirse console modulu icin
// #include "console.h"
// extern void console_puts(const char *s);


// Disk sistemini baslatir
void hd_init(void) {
    // Simdilik basit bir baslatma.
    // Gerekirse burada disk varligini kontrol etmek (int 13h AH=08h),
    // partition table okumak gibi islemler yapilabilir.
    // console_puts("HD init...\n"); // Hata ayiklama icin
}

// Belirtilen surucuden (drive) CHS adresine (cylinder, head, sector)
// 'count' adet sektoru 'buffer_segment:buffer_offset' adresine okur.
uint8_t hd_read_sectors_chs(uint8_t drive, uint16_t cylinder, uint8_t head, uint8_t sector, uint8_t count, uint16_t buffer_segment, uint16_t buffer_offset) {

    uint8_t error_code;

    // bios_disk_io Assembly fonksiyonunu cagir
    // Parametre sirasi Assembly fonksiyonunun bekledigi siraya gore ayarlanmali (C'den itilen sira)
    error_code = bios_disk_io(BIOS_READ_SECTORS, count, cylinder, head, sector, buffer_segment, buffer_offset, drive);

    // Donen hata kodunu kontrol et
    if (error_code != BIOS_ERR_NO_ERROR) {
        // Hata durumunu isleyebilir veya bir hata mesaji yazdirabilirsiniz.
        // console_puts("Disk okuma hatasi! Kod: ");
        // printk("%x\n", error_code); // printk gibi bir fonksiyon kullanilabilir
    }

    return error_code; // Hata kodunu dondur (0 basari)
}

// Belirtilen surucuye (drive) CHS adresine (cylinder, head, sector)
// 'count' adet sektoru 'buffer_segment:buffer_offset' adresinden yazar.
uint8_t hd_write_sectors_chs(uint8_t drive, uint16_t cylinder, uint8_t head, uint8_t sector, uint8_t count, uint16_t buffer_segment, uint16_t buffer_offset) {

    uint8_t error_code;

    // bios_disk_io Assembly fonksiyonunu cagir
    error_code = bios_disk_io(BIOS_WRITE_SECTORS, count, cylinder, head, sector, buffer_segment, buffer_offset, drive);

    // Donen hata kodunu kontrol et
    if (error_code != BIOS_ERR_NO_ERROR) {
        // Hata durumunu isleyebilir veya bir hata mesaji yazdirabilirsiniz.
        // console_puts("Disk yazma hatasi! Kod: ");
        // printk("%x\n", error_code); // printk gibi bir fonksiyon kullanilabilir
    }

    return error_code; // Hata kodunu dondur (0 basari)
}

// hd.c sonu