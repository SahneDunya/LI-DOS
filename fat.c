// fat.c (Opsiyonel - get_fat_entry baska dosyaya tasinirsa)
// Lİ-DOS FAT Yardımcı Fonksiyonlari
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: FAT'tan bilgi okuma (cluster zinciri takibi).

#include "fs.h" // VBPB, SECTOR_SIZE, FS_TYPE_x vb. tanimlar icin

// fs.c'deki global degiskenlere erisim gerekli (current_vbpb, fat_start_sector, fs_type, fs_drive_id, fat_sector_buffer)
// Bu global degiskenleri bu dosyada extern olarak bildirmek veya fs.h'da tanimlamak gerekir.
// Global degiskenleri ayri bir .c dosyasinda tanimlayip fs.h'da extern etmek yaygin yaklasimdir.
 struct vbpb current_vbpb;

// FAT'tan belirtilen cluster numarasinin degerini okur.
// Donus degeri: Sonraki cluster numarasi, veya Ozel degerler (EOF, Bad Cluster vb.)
 uint32_t get_fat_entry(uint16_t cluster)