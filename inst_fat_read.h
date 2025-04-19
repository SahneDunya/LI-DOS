// inst_fat_read.h
// Lİ-DOS Kurulum Programı Minimum FAT Okuma Yardımcıları (Güncellenmiş)
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Kurulum medyasindaki FAT'i ve hedef MBR/Partition'i okuma/parse etme.

#ifndef _INST_FAT_READ_H
#define _INST_FAT_READ_H

#include "inst.h" // Temel tipler, SECTOR_SIZE, Partition yapilari

// Minimum VBPB bilgisi (kurulum medyasindaki veya hedefteki FAT'i okumak icin)
struct __attribute__((packed)) min_vbpb { // packed non-standard C89
    // Offset 11
    uint16_t bytes_per_sector;
    // Offset 13
    uint8_t sectors_per_cluster;
    // Offset 14
    uint16_t reserved_sectors;
    // Offset 16
    uint8_t num_fats;
    // Offset 17
    uint16_t root_entry_count;
    // Offset 22
    uint16_t sectors_per_fat_16;
    // Offset 24
    uint16_t sectors_per_track; // LBA->CHS icin (eğer kullanılacaksa)
    // Offset 26
    uint16_t num_heads;       // LBA->CHS icin (eğer kullanılacaksa)
    // Offset 32 (FAT32 değilse bu alanlar total_sectors_32 vs. içerir)
    uint32_t hidden_sectors; // Partition başlangıcı (Partition Boot Sector LBA)
    uint32_t total_sectors_32; // 32 bit Total Sectors

    // Offset 54
    uint8_t fs_type_label[8]; // "FAT12   " veya "FAT16   " gibi
    // Offset 510
    uint16_t boot_signature;
    // NOT: Tam VBPB yapısı değil, sadece parse ettiğimiz alanlar.
};

// --- Kaynak Disk FAT Bilgileri ---
// inst_fat_read_vbpb tarafindan doldurulacak
extern struct min_vbpb src_vbpb_info;
extern uint32_t src_fat_start_sector;
extern uint32_t src_root_dir_start_sector;
extern uint32_t src_data_start_sector;
extern uint32_t src_sectors_per_fat;
extern uint8_t src_fs_type; // FAT12 veya FAT16

// --- Hedef Disk FAT Bilgileri ---
// inst_fat_read_vbpb_target tarafindan doldurulacak (Partition Boot Sector'dan)
extern struct min_vbpb target_vbpb_info;
extern uint32_t target_fat_start_sector; // Partition Boot Sector'a göre offsetli
extern uint32_t target_root_dir_start_sector; // Partition Boot Sector'a göre offsetli
extern uint32_t target_data_start_sector; // Partition Boot Sector'a göre offsetli
extern uint32_t target_sectors_per_fat;
extern uint8_t target_fs_type; // FAT12 veya FAT16


// Kurulum medyasinin VBPB'sini okur ve temel bilgileri parse eder.
// boot_sector_buffer: Okunmus boot sektoru verisini iceren 512 byte'lik buffer.
// Donus degeri: 0 basari, -1 hata (gecersiz VBPB veya imza).
int inst_fat_read_vbpb(const uint8_t *boot_sector_buffer);

// Hedef diskin MBR'sini okur, Partition Table'i parse eder, ilk boot edilebilir primary FAT partition'i bulur.
// drive_id: Hedef disk sürücüsü ID'si.
// partition_lba: Bulunan partition'in başlangıç LBA adresi buraya yazılır.
// Donus degeri: 0 basari, -1 hata.
int inst_fat_read_parse_mbr(uint8_t drive_id, uint32_t *partition_lba);

// Hedef Partition Boot Sector'dan VBPB'yi okur ve parametrelerini doldurur (target_vbpb_info vb.)
// drive_id: Hedef disk sürücüsü ID'si.
// pbs_lba: Partition Boot Sector'un LBA adresi.
// Donus degeri: 0 basari, -1 hata.
int inst_fat_read_vbpb_target(uint8_t drive_id, uint32_t pbs_lba);


// Kaynak medyasindaki KERNEL_FILENAME dosyasini arar (Root Directory'de).
// drive_id: Kaynak disk sürücüsü ID'si.
// filename_8_3: Aranan dosya adı (8.3 formatında).
// file_size: Bulunan dosyanin boyutu buraya yazilir.
// Donus degeri: Dosyanin basladigi ilk cluster numarasi, bulunamazsa 0.
uint16_t inst_fat_read_find_file(uint8_t drive_id, const char *filename_8_3, uint32_t *file_size);

// FAT'tan belirtilen cluster numarasinin degerini okur (kaynak disk icin).
// cluster: Degeri okunacak cluster numarasi.
// Donus degeri: Sonraki cluster numarasi veya Ozel deger (EOF, Bad).
uint32_t inst_fat_read_get_fat_entry(uint16_t cluster);


// Kaynak disk parametrelerine göre cluster numarasinin LBA sektör adresini hesaplar.
extern uint32_t inst_fat_read_cluster_to_lba(uint16_t cluster);

// Hedef disk parametrelerine göre cluster numarasinin LBA sektör adresini hesaplar.
extern uint32_t inst_fat_read_cluster_to_lba_target(uint16_t cluster);


#endif // _INST_FAT_READ_H