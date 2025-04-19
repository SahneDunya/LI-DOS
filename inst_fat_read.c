// inst_fat_read.c
// Lİ-DOS Kurulum Programı Minimum FAT Okuma Yardımcıları Implementasyonu (Güncellenmiş)
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Kurulum medyasindaki FAT'i ve hedef MBR/Partition'i okuma/parse etme.

#include "inst_fat_read.h"
#include "inst_io.h"   // Debug cikti icin
#include "inst_disk.h" // Disk G/Ç fonksiyonlari (güncellenmiş)
// Temel string/bellek fonksiyonlari
extern void *memcpy(void *dest, const void *src, size_t n);
extern int strcmp(const char *s1, const char *s2);
extern void *memset(void *s, int c, size_t n);
extern size_t strlen(const char *s); // KERNEL_FILENAME boyutu icin

// --- Dahili Degiskenler (Parse Edilmis VBPB Bilgileri) ---
struct min_vbpb src_vbpb_info; // Kaynak VBPB bilgileri
uint32_t src_fat_start_sector;
uint32_t src_root_dir_start_sector;
uint32_t src_data_start_sector;
uint32_t src_sectors_per_fat;
uint8_t src_fs_type; // FAT12 veya FAT16

struct min_vbpb target_vbpb_info; // Hedef VBPB bilgileri
uint32_t target_fat_start_sector; // Partition Boot Sector'a göre offsetli
uint32_t target_root_dir_start_sector; // Partition Boot Sector'a göre offsetli
uint32_t target_data_start_sector; // Partition Boot Sector'a göre offsetli
uint32_t target_sectors_per_fat;
uint8_t target_fs_type; // FAT12 veya FAT16


// Disk G/Ç bufferlari (installer.c'de tanimli oldugu varsayiliyor, extern edilebilir)
extern uint8_t source_sector_buffer[SECTOR_SIZE];
extern uint8_t target_sector_buffer[SECTOR_SIZE];


// --- Dahili Yardimci Fonksiyonlar ---
// (Orijinal fs.c'deki get_fat_entry ve cluster_to_lba mantigi buraya tasinir)
// format_filename_8_3 ve compare_filenames_8_3 helperlari da buraya veya baska bir yere eklenmelidir.

// FAT'tan belirtilen cluster numarasinin degerini okur (belirtilen VBPB bilgilerini kullanarak).
// vbpb_info: Hangi diskin VBPB bilgileri kullanilacak (kaynak veya hedef)
// fat_start_sector_lba: FAT alaninin baslangic LBA'si (mutlak)
// sectors_per_fat_val: FAT boyutunun sektor cinsinden degeri
// fs_type_val: FAT tipi (FAT12/16)
// drive_id: Okuma yapilacak disk surucusu ID'si
// fat_buffer: FAT sektorlerini okumak icin buffer
// fat_buffer_lba_cache: FAT bufferinin LBA cache degeri
// cluster: Degeri okunacak cluster numarasi.
// Donus degeri: Sonraki cluster numarasi veya Ozel deger (EOF, Bad).
static uint32_t get_fat_entry_generic(
    const struct min_vbpb *vbpb_info,
    uint32_t fat_start_sector_lba,
    uint32_t sectors_per_fat_val,
    uint8_t fs_type_val,
    uint8_t drive_id,
    uint8_t *fat_buffer,
    uint32_t *fat_buffer_lba_cache,
    uint16_t cluster)
{
    // Mantık önceki get_fat_entry ile aynı, sadece VBPB bilgileri ve bufferlar parametre olarak geliyor.
    // FAT12/FAT16 okuma mantığı burada implemente edilir.
    // ... (onceki get_fat_entry implementasyonunun bu parametrelere uyarlanmış hali) ...
    uint32_t fat_entry_value = 0xFFF7; // Varsayilan hata
    uint32_t fat_offset;
    uint32_t fat_sector_offset_in_fat; // FAT alanindaki sektor offseti
    uint16_t fat_sector_offset_in_buffer; // FAT sektoru icindeki byte offseti
    uint8_t error;

    if (fs_type_val == FS_TYPE_FAT12) {
        fat_offset = cluster + (cluster / 2);
        fat_sector_offset_in_fat = fat_offset / SECTOR_SIZE;
        fat_sector_offset_in_buffer = fat_offset % SECTOR_SIZE;
        fat_sector_lba = fat_start_sector_lba + fat_sector_offset_in_fat;

        if (*fat_buffer_lba_cache != fat_sector_lba) {
             error = inst_read_sectors(drive_id, fat_sector_lba, 1, seg(fat_buffer), offset(fat_buffer));
             if (error) return 0xFFF7; // Hata
             *fat_buffer_lba_cache = fat_sector_lba;
        }
        fat_entry_value = *(uint16_t *)((uint8_t *)fat_buffer + fat_sector_offset_in_buffer);
        if (cluster & 0x01) fat_entry_value >>= 4;
        fat_entry_value &= 0xFFF;
        if (fat_entry_value >= 0xFF8) fat_entry_value |= 0xFFF00000;

    } else if (fs_type_val == FS_TYPE_FAT16) {
        fat_offset = cluster * 2;
        fat_sector_offset_in_fat = fat_offset / SECTOR_SIZE;
        fat_sector_offset_in_buffer = fat_offset % SECTOR_SIZE;
        fat_sector_lba = fat_start_sector_lba + fat_sector_offset_in_fat;

        if (*fat_buffer_lba_cache != fat_sector_lba) {
             error = inst_read_sectors(drive_id, fat_sector_lba, 1, seg(fat_buffer), offset(fat_buffer));
             if (error) return 0xFFF7; // Hata
             *fat_buffer_lba_cache = fat_sector_lba;
        }
        fat_entry_value = *(uint16_t *)((uint8_t *)fat_buffer + fat_sector_offset_in_buffer);
        if (fat_entry_value >= 0xFFF8) fat_entry_value |= 0xFFF00000;

    }
     // else Bilinmeyen FS tipi (error)

    return fat_entry_value;
}

// Cluster numarasini LBA sektör adresine çevirir (belirtilen VBPB bilgilerini kullanarak).
// vbpb_info: Hangi diskin VBPB bilgileri kullanilacak
// data_start_sector_lba: Veri alaninin baslangic LBA'si (mutlak)
// cluster: Cevrilecek cluster numarasi
// Donus: LBA sektor adresi
static uint32_t cluster_to_lba_generic(
    const struct min_vbpb *vbpb_info,
    uint32_t data_start_sector_lba,
    uint16_t cluster)
{
    // Cluster 2, data_start_sector'da baslar. Cluster N, N-2 * sectors_per_cluster sonra baslar.
    if (cluster < 2) return 0; // Gecersiz data clusteri

    return data_start_sector_lba + (uint32_t)(cluster - 2) * vbpb_info->sectors_per_cluster;
}


// --- Global FAT Buffer Cache ---
static uint8_t src_fat_sector_buffer_cache[SECTOR_SIZE];
static uint32_t src_fat_sector_buffer_lba_cache = 0xFFFFFFFF;

static uint8_t target_fat_sector_buffer_cache[SECTOR_SIZE];
static uint32_t target_fat_sector_buffer_lba_cache = 0xFFFFFFFF;


// --- Implementasyonlar (inst_fat_read.h'taki fonksiyonlar) ---

// Kurulum medyasinin VBPB'sini parse eder.
int inst_fat_read_vbpb(const uint8_t *boot_sector_buffer) {
    const struct __attribute__((packed)) min_vbpb *vbpb_ptr = (const struct __attribute__((packed)) min_vbpb *)boot_sector_buffer;

    if (!boot_sector_buffer) return -1;
    if (vbpb_ptr->boot_signature != 0xAA55) return -1; // Gecersiz imza

    memcpy(&src_vbpb_info, vbpb_ptr, sizeof(struct min_vbpb));

    if (src_vbpb_info.bytes_per_sector != SECTOR_SIZE || src_vbpb_info.num_fats == 0 || src_vbpb_info.sectors_per_fat_16 == 0) {
         return -1; // Gecersiz temel degerler
    }

    src_sectors_per_fat = src_vbpb_info.sectors_per_fat_16; // FAT12/16 ayni alan

    // FAT tipini belirle (cluster sayisi hesaplama)
    uint32_t root_dir_sectors = (uint32_t)(src_vbpb_info.root_entry_count * 32 + SECTOR_SIZE - 1) / SECTOR_SIZE;
    uint32_t total_sectors = (vbpb_ptr->total_sectors_16 == 0) ? vbpb_ptr->total_sectors_32 : vbpb_ptr->total_sectors_16; // Tam VBPB'den total sector al
    uint32_t data_sectors = total_sectors - src_vbpb_info.reserved_sectors - (uint32_t)src_vbpb_info.num_fats * src_sectors_per_fat - root_dir_sectors;
    uint32_t num_clusters = data_sectors / src_vbpb_info.sectors_per_cluster;

    if (num_clusters < 4085) {
        src_fs_type = FS_TYPE_FAT12;
    } else {
        src_fs_type = FS_TYPE_FAT16;
    }

    // Alanlarin baslangic sektorlerini hesapla (kaynak)
    src_fat_start_sector = src_vbpb_info.reserved_sectors;
    src_root_dir_start_sector = src_fat_start_sector + (uint32_t)src_vbpb_info.num_fats * src_sectors_per_fat;
    src_data_start_sector = src_root_dir_start_sector + root_dir_sectors;

    src_fat_sector_buffer_lba_cache = 0xFFFFFFFF; // Cache'i sifirla

    return 0;
}

// Hedef diskin MBR'sini okur, Partition Table'i parse eder, ilk boot edilebilir primary FAT partition'i bulur.
int inst_fat_read_parse_mbr(uint8_t drive_id, uint32_t *partition_lba) {
    uint8_t error;
    struct __attribute__((packed)) partition_entry *pt_entry;
    int i;

    if (!partition_lba) return -1;

    // Hedef diskin MBR'sini oku (LBA 0)
    error = inst_read_sectors(drive_id, 0, 1, seg(target_sector_buffer), offset(target_sector_buffer)); // target_sector_buffer global
    if (error) {
        inst_puts("FAT Read Error: Hedef MBR okunamadi!.\r\n");
        return -1;
    }

    // MBR imzasini kontrol et
    uint16_t mbr_signature = *(uint16_t *)((uint8_t *)target_sector_buffer + MBR_BOOT_SIGNATURE_OFFSET);
    if (mbr_signature != 0xAA55) {
        inst_puts("FAT Read Error: Hedef MBR imzasi gecersiz!.\r\n");
        return -1;
    }

    // Partition Table'i tara
    pt_entry = (struct __attribute__((packed)) partition_entry *)((uint8_t *)target_sector_buffer + MBR_PARTITION_TABLE_OFFSET);

    for (i = 0; i < 4; i++) { // 4 adet primary partition girdisi
        // Boot edilebilir mi? (0x80) ve FAT tipi mi?
        // Yaygin FAT tipleri: 0x01 (FAT12), 0x04 (FAT16 <32MB), 0x06 (FAT16 >=32MB)
        if (pt_entry[i].boot_indicator == 0x80 &&
           (pt_entry[i].system_id == 0x01 || pt_entry[i].system_id == 0x04 || pt_entry[i].system_id == 0x06))
        {
            // Boot edilebilir FAT partition bulundu
            *partition_lba = pt_entry[i].start_lba; // Partition Boot Sector'un LBA adresi
            inst_puts("FAT Read: Boot edilebilir FAT partition bulundu. LBA: 0x"); // Hex yazdırma *partition_lba
            inst_puts("\r\n");
            return 0; // Basari
        }
    }

    // Boot edilebilir FAT partition bulunamadi
    inst_puts("FAT Read Error: Boot edilebilir FAT partition bulunamadi.\r\n");
    return -1;
}

// Hedef Partition Boot Sector'dan VBPB'yi okur ve parametrelerini doldurur (target_vbpb_info vb.)
int inst_fat_read_vbpb_target(uint8_t drive_id, uint32_t pbs_lba) {
    uint8_t error;
     const struct __attribute__((packed)) min_vbpb *vbpb_ptr = (const struct __attribute__((packed)) min_vbpb *)target_sector_buffer;

    // Partition Boot Sector'u oku
    error = inst_read_sectors(drive_id, pbs_lba, 1, seg(target_sector_buffer), offset(target_sector_buffer)); // target_sector_buffer global
    if (error) {
        inst_puts("FAT Read Error: Hedef PBS okunamadi!.\r\n");
        return -1;
    }

    // PBS imzasini kontrol et (MBR imzasiyla ayni offset)
     if (vbpb_ptr->boot_signature != 0xAA55) {
        inst_puts("FAT Read Error: Hedef PBS imzasi gecersiz!.\r\n");
        return -1;
    }

    // VBPB bilgilerini parse et
    memcpy(&target_vbpb_info, vbpb_ptr, sizeof(struct min_vbpb));

     if (target_vbpb_info.bytes_per_sector != SECTOR_SIZE || target_vbpb_info.num_fats == 0 || target_vbpb_info.sectors_per_fat_16 == 0) {
         inst_puts("FAT Read Error: Hedef VBPB degerleri gecersiz.\r\n");
         return -1;
    }

    target_sectors_per_fat = target_vbpb_info.sectors_per_fat_16;

    // FAT tipini belirle (cluster sayisi hesaplama)
    uint32_t root_dir_sectors = (uint32_t)(target_vbpb_info.root_entry_count * 32 + SECTOR_SIZE - 1) / SECTOR_SIZE;
    uint32_t total_sectors = (vbpb_ptr->total_sectors_16 == 0) ? vbpb_ptr->total_sectors_32 : vbpb_ptr->total_sectors_16; // Tam VBPB'den total sector al
    uint32_t data_sectors = total_sectors - target_vbpb_info.reserved_sectors - (uint32_t)target_vbpb_info.num_fats * target_sectors_per_fat - root_dir_sectors;
    uint32_t num_clusters = data_sectors / target_vbpb_info.sectors_per_cluster;

    if (num_clusters < 4085) {
        target_fs_type = FS_TYPE_FAT12;
    } else {
        target_fs_type = FS_TYPE_FAT16;
    }

    // Alanlarin baslangic sektorlerini hesapla (hedef partition icindeki offsetler)
    // Partition'in mutlak LBA'sina eklenmelidir kullanilirken.
    target_fat_start_sector = target_vbpb_info.reserved_sectors;
    target_root_dir_start_sector = target_fat_start_sector + (uint32_t)target_vbpb_info.num_fats * target_sectors_per_fat;
    target_data_start_sector = target_root_dir_start_sector + root_dir_sectors;

    target_fat_sector_buffer_lba_cache = 0xFFFFFFFF; // Cache'i sifirla

    inst_puts("FAT Read: Hedef VBPB parsed. Type FAT"); // target_fs_type yazdirma
    inst_puts(".\r\n");

    return 0;
}


// Kaynak medyasindaki KERNEL_FILENAME dosyasini arar (Root Directory'de).
uint16_t inst_fat_read_find_file(uint8_t drive_id, const char *filename_8_3, uint32_t *file_size) {
    // Mantık önceki find_entry_in_root_dir ile aynı, sadece kaynak VBPB bilgilerini kullanır.
    // ... Önceki implementasyon ...
    uint32_t sector_lba;
    uint16_t i, j;
    uint8_t error;
    char search_name_8_3[12]; // Arama icin 8.3 format + null
    struct fat_dir_entry *entry; // Dizin girdisi yapisi inst.h'ten alinir


    if (!filename_8_3 || !file_size) return 0;

    // Aranan dosya ismini 8.3 formatina cevir ve boslukla doldur
    // format_filename_8_3(filename_8_3, search_name_8_3); // Implement this helper

    // Ornek: Sadece KERNEL.BIN dosyasini arayacaksek, 8.3 formatini manuel olusturabiliriz.
    memcpy(search_name_8_3, "KERNEL  BIN", 11); // 8+3 karakter, bosluk doldurulmus
    search_name_8_3[11] = '\0';


    // Kök Dizin sektorlerini dolas (kaynak)
    uint32_t root_dir_sector_count = (uint32_t)(src_vbpb_info.root_entry_count * 32 + SECTOR_SIZE - 1) / SECTOR_SIZE;

    for (i = 0; i < root_dir_sector_count; i++) {
        sector_lba = src_root_dir_start_sector + i;

        // Sektoru oku (kaynak)
        error = inst_read_sectors(drive_id, sector_lba, 1, seg(source_sector_buffer), offset(source_sector_buffer)); // source_sector_buffer global
        if (error) {
             inst_puts("FAT Read Error: Kaynak Root Dir sector okunamadi.\r\n");
             return 0; // Hata
        }

        // Sektordeki dizin girdilerini dolas
        for (j = 0; j < 16; j++) {
            entry = (struct fat_dir_entry *)((uint8_t *)source_sector_buffer + j * sizeof(struct fat_dir_entry));

            if (entry->filename[0] == 0x00) return 0; // Dizin sonu
            if (entry->filename[0] == 0xE5) continue; // Silinmis
            if (entry->attribute & FAT_ATTR_LONG_NAME) continue; // LFN atla
            if (entry->attribute & FAT_ATTR_DIRECTORY) continue; // Dizinleri atla


            // Dosya ismini karsilastir (compare_filenames_8_3 helper gerekli)
            int names_match = 1;
             for(int k=0; k<11; k++) {
                if (entry->filename[k] != (uint8_t)search_name_8_3[k]) {
                    names_match = 0;
                    break;
                }
            }

            if (names_match) {
                 *file_size = entry->file_size;
                 return entry->first_cluster_low; // Ilk cluster numarasini dondur
            }
        }
    }

    return 0; // Bulunamadi
}

// FAT'tan belirtilen cluster numarasinin degerini okur (kaynak disk icin).
uint32_t inst_fat_read_get_fat_entry(uint16_t cluster) {
     // get_fat_entry_generic fonksiyonunu kaynak bilgilerle cagir.
    return get_fat_entry_generic(
        &src_vbpb_info,
        src_fat_start_sector,
        src_sectors_per_fat,
        src_fs_type,
        boot_drive_id, // Kaynak drive ID'si (global olmali)
        src_fat_sector_buffer_cache,
        &src_fat_sector_buffer_lba_cache,
        cluster);
}


// Kaynak disk parametrelerine göre cluster numarasinin LBA sektör adresini hesaplar.
uint32_t inst_fat_read_cluster_to_lba(uint16_t cluster) {
    // cluster_to_lba_generic fonksiyonunu kaynak bilgilerle cagir.
    return cluster_to_lba_generic(
        &src_vbpb_info,
        src_data_start_sector,
        cluster);
}

// Hedef disk parametrelerine göre cluster numarasinin LBA sektör adresini hesaplar.
uint32_t inst_fat_read_cluster_to_lba_target(uint16_t cluster) {
     // cluster_to_lba_generic fonksiyonunu hedef bilgilerle cagir.
     // Dikkat! Hedef LBA = Partition LBA + Partition ici LBA
     // target_data_start_sector, PBS'e gore offsetlidir. PBS LBA'sini eklemek gerekir.
     extern uint32_t target_partition_lba; // installer.c'de bulunacak global
     return target_partition_lba + cluster_to_lba_generic(
         &target_vbpb_info,
         target_data_start_sector,
         cluster);
}

// --- Yardımcı Get Fonksiyonları ---
// Bu fonksiyonlar inst_fat_read.h'ta bildirildi ve global bilgileri dondurur.
uint32_t inst_fat_read_get_sectors_per_fat(void) { return src_sectors_per_fat; } // Kaynak bilgisi
uint32_t inst_fat_read_get_root_dir_sectors(void) { // Kaynak bilgisi
     return (uint32_t)(src_vbpb_info.root_entry_count * 32 + SECTOR_SIZE - 1) / SECTOR_SIZE;
}
uint32_t inst_fat_read_get_reserved_sectors(void) { return src_vbpb_info.reserved_sectors; } // Kaynak bilgisi
uint32_t inst_fat_read_get_num_fats(void) { return src_vbpb_info.num_fats; } // Kaynak bilgisi
uint16_t inst_fat_read_get_sectors_per_cluster(void) { return src_vbpb_info.sectors_per_cluster; } // Kaynak bilgisi

// --- Hedef Bilgileri Get Fonksiyonları ---
uint32_t inst_fat_read_get_sectors_per_fat_target(void) { return target_sectors_per_fat; }
uint32_t inst_fat_read_get_root_dir_sectors_target(void) {
     return (uint32_t)(target_vbpb_info.root_entry_count * 32 + SECTOR_SIZE - 1) / SECTOR_SIZE;
}
uint32_t inst_fat_read_get_reserved_sectors_target(void) { return target_vbpb_info.reserved_sectors; }
uint32_t inst_fat_read_get_num_fats_target(void) { return target_vbpb_info.num_fats; }
uint16_t inst_fat_read_get_sectors_per_cluster_target(void) { return target_vbpb_info.sectors_per_cluster; }
uint8_t inst_fat_read_get_fs_type_target(void) { return target_fs_type; }


// inst_fat_read.c sonu