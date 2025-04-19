// installer.c
// Lİ-DOS Kurulum Programı Ana Mantığı (Güncellenmiş)
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Çekirdeği ve boot sektörü hedef diske yazmak (Partition destegi, Boot sektor yamalama).

#include "inst.h"      // Kurulum arayuzu, sabitler (güncellenmiş)
#include "inst_io.h"   // Düşük seviye G/Ç
#include "inst_disk.h" // Düşük seviye disk G/Ç (güncellenmiş)
// Minimum FAT okuma yardımcıları (güncellenmiş)
#include "inst_fat_read.h"

// Lİ-DOS Boot Sektörü ikili verisi sablonu
// boot/boot.bin'den generate edilecek, yama icin alanlari olmali.
#include "boot_sector_bin.h" // unsigned char boot_sector_data[]

// --- Sabitler ---
#define TARGET_DRIVE_ID 0x80 // Hedef Disk (İlk Hard Disk)
#define KERNEL_FILENAME "KERNEL.BIN" // Kurulum medyasındaki çekirdek dosyasının adı (8.3 format)

// --- Global Değişkenler ---
extern uint8_t boot_drive_id; // inst_head.S'ten boot edilen drive ID'si

// Disk G/Ç için dahili bufferlar (64KB limiti içinde olmalı) - inst_fat_read.c'de extern ediliyor.
uint8_t source_sector_buffer[SECTOR_SIZE]; // Kaynak diskten okuma için
uint8_t target_sector_buffer[SECTOR_SIZE]; // Hedef diske yazma için

// Hedef partition bilgisi
uint32_t target_partition_lba = 0; // Bulunan hedef partition'in LBA adresi


// --- Kernel LBA/Size Yamalama Fonksiyonu ---
// Bu fonksiyon, boot sektoru sablonunu cagirir ve kernelin diskteki
// konumu ve boyutu bilgilerini sablon uzerindeki belirlenmis offsetlere yazar.
// boot_sector_buffer: Yamalanacak boot sektorunun 512 byte'lik bufferi.
// kernel_start_lba: Cekirdegin hedef diskte baslayacagi LBA adresi.
// kernel_size_bytes: Cekirdek dosyasinin boyutu (byte).
// Not: BOOT_SECTOR_KERNEL_LBA_OFFSET ve BOOT_SECTOR_KERNEL_SIZE_OFFSET
// bootloader koduna (boot/boot.S) gore dogru tanimlanmalidir!
static void patch_boot_sector(uint8_t *boot_sector_buffer, uint32_t kernel_start_lba, uint32_t kernel_size_bytes) {
    if (!boot_sector_buffer) return;

    // Kernel başlangıç LBA'sını boot sektörüne yaz (4 byte)
    // Offset BOOT_SECTOR_KERNEL_LBA_OFFSET'e yazılacak. Little Endian varsayımı.
    *(uint32_t *)(boot_sector_buffer + BOOT_SECTOR_KERNEL_LBA_OFFSET) = kernel_start_lba;

    // Kernel boyutunu boot sektörüne yaz (4 byte)
    // Offset BOOT_SECTOR_KERNEL_SIZE_OFFSET'e yazılacak. Little Endian varsayımı.
    *(uint32_t *)(boot_sector_buffer + BOOT_SECTOR_KERNEL_SIZE_OFFSET) = kernel_size_bytes;

    inst_puts("Boot sektoru kernel LBA ve boyutu ile yamalandi.\r\n");
}


// --- Kurulum Ana Fonksiyonu ---
// inst_head.S tarafindan cagirilir.
void c_installer_main(void) {
    uint8_t error;
    uint32_t kernel_file_size;
    uint16_t kernel_first_cluster;
    uint32_t kernel_target_lba;
    uint32_t bytes_copied;
    uint16_t current_cluster;
    uint32_t current_source_lba;
    size_t sectors_to_copy;


    inst_puts("LI-DOS Kurulum Programi\r\n");
    inst_puts("========================\r\n");
    inst_puts("Hedef disk: BIOS drive 0x80 (ilk hard disk).\r\n");
    inst_puts("Kaynak surucu: BIOS drive 0x");
    // boot_drive_id'yi hex olarak yazdirmak icin basit bir sayidan stringe cevirme gerekir
    inst_puts("\r\n");

    inst_puts("Kuruluma baslamak icin bir tusa basin...");
    inst_getc();
    inst_puts("\r\n");

    // --- Adim 0: Disk Geometrilerini Oku ---
    inst_puts("Hedef disk geometrisi okunuyor...\r\n");
    error = inst_get_drive_params(TARGET_DRIVE_ID);
    if (error) {
         inst_puts("HATA: Hedef disk geometrisi okunamadi! Kurulum iptal.\r\n"); // Hata kodu yazdirilabilir
         goto end_install;
    }
    // Geometri bilgileri (SPT, Heads) inst_disk.S'teki global değişkenlere yazıldı.

    inst_puts("Kaynak disk geometrisi okunuyor...\r\n");
     error = inst_get_drive_params(boot_drive_id); // Kaynak sürücü ID'si inst_head.S'ten gelir
     if (error) {
         inst_puts("HATA: Kaynak disk geometrisi okunamadi! Kurulum iptal.\r\n"); // Hata kodu yazdirilabilir
         goto end_install;
    }
    // Kaynak geometri bilgileri de inst_disk.S'teki global değişkenlere yazıldı.

    // --- Adim 1: Kaynak Disk FAT Bilgilerini Oku ---
    inst_puts("Kaynak disk FAT bilgileri okunuyor...\r\n");
    error = inst_read_sectors(boot_drive_id, 0, 1, seg(source_sector_buffer), offset(source_sector_buffer));
    if (error) {
         inst_puts("HATA: Kaynak VBPB sektoru okunamadi! Kurulum iptal.\r\n"); // Hata kodu yazdirilabilir
         goto end_install;
    }
    if (inst_fat_read_vbpb(source_sector_buffer) != 0) {
         inst_puts("HATA: Kaynak VBPB gecerli degil! Kurulum iptal.\r\n");
         goto end_install;
    }
    inst_puts("Kaynak VBPB okundu ve parse edildi.\r\n");

    // --- Adim 2: Kaynak Diskten Çekirdek Dosyasini Bul ---
    inst_puts("Cekirdek dosyasi (");
    inst_puts(KERNEL_FILENAME);
    inst_puts(") kaynak diskte aranıyor...\r\n");

    kernel_first_cluster = inst_fat_read_find_file(boot_drive_id, KERNEL_FILENAME, &kernel_file_size);

    if (kernel_first_cluster == 0 || kernel_file_size == 0) {
         inst_puts("HATA: Cekirdek dosyasi kaynak diskte bulunamadi veya boyutu 0! Kurulum iptal.\r\n");
         goto end_install;
    }
    inst_puts("Cekirdek dosyasi bulundu. Boyut: ");
    // kernel_file_size'i ondalik olarak yazdırma kodu burada.
    inst_puts(" bytes.\r\n");

    // --- Adim 3: Hedef Disk MBR'sini Parse Et ve Partition Bul ---
    inst_puts("Hedef disk MBR ve partition tablosu okunuyor...\r\n");
    error = inst_fat_read_parse_mbr(TARGET_DRIVE_ID, &target_partition_lba);
    if (error) {
         inst_puts("HATA: Boot edilebilir FAT partition bulunamadi! Kurulum iptal.\r\n");
         goto end_install;
    }
    inst_puts("Hedef partition bulundu. Baslangic LBA: 0x");
    // target_partition_lba hex yazdırma
    inst_puts("\r\n");

    // --- Adim 4: Hedef Partition Boot Sector'dan VBPB Bilgilerini Oku ---
    inst_puts("Hedef partition VBPB bilgileri okunuyor...\r\n");
    error = inst_fat_read_vbpb_target(TARGET_DRIVE_ID, target_partition_lba);
     if (error) {
         inst_puts("HATA: Hedef partition VBPB okunamadi! Kurulum iptal.\r\n");
         goto end_install;
    }
    inst_puts("Hedef VBPB okundu ve parse edildi. FAT Tipi: FAT");
    // inst_fat_read_get_fs_type_target() ondalik yazdirma
    inst_puts("\r\n");


    // --- Adim 5: Çekirdeğin Hedefteki Konumunu Hesapla ---
    // Çekirdek, hedef partitiondaki Boot Sektörü, FAT'lar ve Root Dir bittikten sonra başlar.
    // Bu hesaplama hedef partition'ın VBPB parametrelerine göre yapılır.
    uint32_t target_root_dir_sectors = inst_fat_read_get_root_dir_sectors_target();
    uint32_t target_sectors_per_fat = inst_fat_read_get_sectors_per_fat_target();
    uint32_t target_reserved_sectors = inst_fat_read_get_reserved_sectors_target();
    uint32_t target_num_fats = inst_fat_read_get_num_fats_target();

    uint32_t kernel_start_lba_in_partition =
        target_reserved_sectors +
        target_num_fats * target_sectors_per_fat +
        target_root_dir_sectors;

    // Çekirdeğin mutlak LBA adresi = Partition Başlangıç LBA + Partition içi LBA
    kernel_target_lba = target_partition_lba + kernel_start_lba_in_partition;

    inst_puts("Cekirdek imaji hedef diskte LBA 0x");
    // kernel_target_lba hex yazdırma
    inst_puts(" adresine yazilacak.\r\n");


    // --- Adim 6: Boot Sektör Şablonunu Yamala ve Hedef Partition'a Yaz ---
    inst_puts("Boot sektoru yamalaniyor ve yaziliyor...\r\n");

    // Boot sektörü şablonunun bir kopyasını al (çünkü yama yapacağız)
    memcpy(target_sector_buffer, boot_sector_data, SECTOR_SIZE);

    // Boot sektörü şablonunu kernel LBA ve boyutu ile yama
    // BOOT_SECTOR_KERNEL_LBA_OFFSET ve BOOT_SECTOR_KERNEL_SIZE_OFFSET sabitleri
    // bootloader kodunuza göre doğru ayarlanmış olmalıdır!
    if (BOOT_SECTOR_KERNEL_LBA_OFFSET + 4 > SECTOR_SIZE || BOOT_SECTOR_KERNEL_SIZE_OFFSET + 4 > SECTOR_SIZE) {
        inst_puts("HATA: Boot sektor yama offsetleri gecersiz! Kurulum iptal.\r\n");
        goto end_install;
    }
    patch_boot_sector(target_sector_buffer, kernel_start_lba_in_partition, kernel_file_size); // Yamalama fonksiyonu

    // Yamalanmış boot sektorünü hedef partition'ın boot sektörüne yaz (PBS)
    uint32_t target_pbs_lba = target_partition_lba; // Partition'in baslangici PBS'tir.

    error = inst_write_sectors(TARGET_DRIVE_ID, target_pbs_lba, 1, seg(target_sector_buffer), offset(target_sector_buffer));
     if (error) {
        inst_puts("HATA: Yamalanmis boot sektoru yazilamadi! (Kod: 0x"); // Hata kodu yazdirma
        inst_puts(")\r\n");
        inst_puts("Kurulum basarisiz.\r\n");
        goto end_install;
    }
     inst_puts("Yamalanmis boot sektoru hedef partition'a yazildi.\r\n");


    // --- Adim 7: Çekirdek İmajını Kaynaktan Oku ve Hedefe Yaz ---
    inst_puts("Cekirdek imaji kaynak diskten okunuyor ve hedef diske yaziliyor...\r\n");

    bytes_copied = 0;
    current_cluster = kernel_first_cluster; // Kaynak dosyanin ilk clusteri
    // kernel_target_lba zaten Adim 5'te hesaplandi

    inst_puts("Veri kopyalaniyor...");

    while (bytes_copied < kernel_file_size) {
         // Kaynak clusterin LBA adresini bul (kaynak disk parametreleriyle)
         current_source_lba = inst_fat_read_cluster_to_lba(current_cluster);

         // Bu clusterdaki sektor sayisini kopyala (kaynak disk cluster boyutu)
         sectors_to_copy = inst_fat_read_get_sectors_per_cluster(); // Kaynak cluster boyutu

         // Kaynak diskten cluster kadar sektoru oku
         error = inst_read_sectors(boot_drive_id, current_source_lba, (uint8_t)sectors_to_copy, seg(source_sector_buffer), offset(source_sector_buffer));
         if (error) {
              inst_puts("\r\nHATA: Cekirdek verisi kaynak diskten okunamadi! (Kaynak LBA 0x"); // LBA yazdirma
              inst_puts(")\r\n");
              goto end_install;
         }

         // Hedef diske sektorleri yaz (Hedef LBA = Çekirdek Başlangıç LBA + Kopyalanan Byte / SECTOR_SIZE)
         write_error = inst_write_sectors(TARGET_DRIVE_ID, kernel_target_lba + (bytes_copied / SECTOR_SIZE), (uint8_t)sectors_to_copy, seg(source_sector_buffer), offset(source_sector_buffer));
         if (write_error) {
             inst_puts("\r\nHATA: Cekirdek verisi hedef diske yazilamadi! (Hedef LBA 0x"); // LBA yazdirma
             inst_puts(")\r\n");
             goto end_install;
         }

         bytes_copied += sectors_to_copy * SECTOR_SIZE; // Kopyalanan byte sayisini artir

         // FAT'tan bir sonraki clusteri bul (kaynak disk FAT)
         uint32_t next_c = inst_fat_read_get_fat_entry(current_cluster);
         if (next_c >= 0xFFF8) { // EOF veya Bad Cluster
             if (bytes_copied < kernel_file_size) {
                  // Dosya sonuna ulasildi ancak dosya boyutu tam cluster/sektor sınırında değil
                  // Bu durumda son eksik kalan byte'ları içeren son sektör parçası kopyalanmış demektir.
                  // Gerekirse burada son eksik byte'ları kopyalama mantığı eklenebilir, ama sektör bazında kopyaladık.
             }
             break; // Kopyalama bitti veya hata oldu
         }
         current_cluster = (uint16_t)next_c;

         // Progress goster (opsiyonel)
          inst_putc('.');
    }

    inst_puts("\r\nCekirdek imaji basariyla yazildi.\r\n");

    // --- Kurulum Tamamlandı ---
    inst_puts("Kurulum tamamlandi! Sistemi yeniden baslatabilirsiniz.\r\n");


end_install:
    inst_puts("Kurulum programindan cikmak icin bir tusa basin...");
    inst_getc();

    cli();
    hlt();
}