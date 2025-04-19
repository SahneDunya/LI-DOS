// fs.h
// Lİ-DOS Temel Dosya Sistemi Arayuzu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Salt okunur FAT12/FAT16 dosya sistemi erisimi.

#ifndef _FS_H
#define _FS_H

// Gerekli temel tureler
// Kernelinizde baska bir baslik dosyasinda (types.h gibi) olabilir.
#include "types.h" // uint8_t, uint16_t, uint32_t, size_t gibi

// FAT Dizin Girdisi Bayraklari (Attribute Byte)
#define FAT_ATTR_READ_ONLY 0x01
#define FAT_ATTR_HIDDEN    0x02
#define FAT_ATTR_SYSTEM    0x04
#define FAT_ATTR_VOLUME_ID 0x08
#define FAT_ATTR_DIRECTORY 0x10
#define FAT_ATTR_ARCHIVE   0x20
#define FAT_ATTR_LONG_NAME (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID) // Uzun dosya adı girişi

// Seek Modları (fs_seek fonksiyonu icin)
#define FS_SEEK_SET 0 // Dosya başlangıcından itibaren
#define FS_SEEK_CUR 1 // Mevcut pozisyondan itibaren
#define FS_SEEK_END 2 // Dosya sonundan itibaren

// FAT formatlari
#define FS_TYPE_FAT12 12
#define FS_TYPE_FAT16 16
// FAT32 daha karmasik ve burada implemente edilmez.

// VBPB (Volume Boot Record) yapısı (Disk imajinin ilk sektoru)
// Bu yapi, diskin FAT dosya sistemi parametrelerini icerir.
// C'de raw byte'lardan map edilmelidir. Packed olmasi gerekebilir.
// 8086+ uyumlulugu icin __attribute__((packed)) veya benzeri derleyici direktifleri
// veya manuel byte okuma kullanilabilir. C89'da __attribute__ olmayabilir.
// Manuel byte okuma veya hizalama sorunlarini dikkate alarak offsetlere dogrudan erisim.
struct __attribute__((packed)) vbpb { // __attribute__((packed)) non-standard C89, manuel offset okuma tercih edilebilir
    uint8_t jump_boot[3];          // Jump instruction
    uint8_t oem_name[8];           // OEM Name/Version
    uint16_t bytes_per_sector;     // Bytes per Sector
    uint8_t sectors_per_cluster;   // Sectors per Cluster
    uint16_t reserved_sectors;     // Reserved Sectors (usually 1 for boot sector)
    uint8_t num_fats;              // Number of FATs (usually 2)
    uint16_t root_entry_count;     // Root Directory Entry Count (FAT12/16 only)
    uint16_t total_sectors_16;     // Total Sectors (if less than 65536)
    uint8_t media_descriptor;      // Media Descriptor
    uint16_t sectors_per_fat_16;   // Sectors per FAT (FAT12/16 only)
    uint16_t sectors_per_track;    // Sectors per Track
    uint16_t num_heads;            // Number of Heads
    uint32_t hidden_sectors;       // Hidden Sectors
    uint32_t total_sectors_32;     // Total Sectors (if >= 65536)

    // Extended Boot Record (for FAT12/16)
    uint8_t drive_number;          // BIOS Drive Number
    uint8_t flags;                 // Flags
    uint8_t signature;             // Extended Boot Signature (0x28 or 0x29)
    uint32_t volume_id;            // Volume Serial Number
    uint8_t volume_label[11];      // Volume Label
    uint8_t fs_type_label[8];      // File System Type Label (e.g. "FAT12   ", "FAT16   ")

    // ... Boot code and other fields ...
    uint8_t boot_code[448];        // Boot Code
    uint16_t boot_signature;       // Boot Signature (0xAA55) at offset 510
};

// FAT Dizin Girdisi yapısı (32 byte)
struct __attribute__((packed)) fat_dir_entry { // __attribute__((packed)) non-standard C89
    uint8_t filename[8];      // 8.3 Filename (padded with spaces)
    uint8_t ext[3];           // Extension (padded with spaces)
    uint8_t attribute;        // File Attributes (FAT_ATTR_x)
    uint8_t reserved;         // Reserved
    uint8_t create_time_tenths; // Create Time (tenths of a second)
    uint16_t create_time;     // Create Time (hours, minutes, seconds)
    uint16_t create_date;     // Create Date
    uint16_t access_date;     // Last Access Date
    uint16_t first_cluster_high; // First Cluster High Word (for FAT32, 0 for FAT12/16)
    uint16_t write_time;      // Last Write Time
    uint16_t write_date;      // Last Write Date
    uint16_t first_cluster_low;  // First Cluster Low Word (first cluster of file/dir)
    uint32_t file_size;       // File Size in bytes
};

// Dahili dosya veya dizin nesnesi (Açık dosyaları temsil eder)
// Birden fazla dosya ayni anda acilabilir, bu yapi her biri icin durum tutar.
#define MAX_OPEN_FILES 8 // Ayni anda acik olabilecek maks dosya sayisi

struct file_object {
    uint8_t state; // Nesnenin durumu (kullanımda, boş)
    uint8_t attributes; // Dosya veya dizin mi? (FAT_ATTR_DIRECTORY)
    uint32_t size;     // Dosya boyutu (byte)
    uint32_t current_offset; // Okuma/Yazma pozisyonu (byte, dosya başından itibaren)
    uint16_t first_cluster; // Dosya/Dizinin basladigi ilk cluster
    uint16_t current_cluster; // Su an uzerinde bulunulan cluster (okuma sirasinda)
    uint16_t offset_in_cluster; // Su anki cluster icindeki offset (byte)
    // Dizin okuma icin:
    uint32_t dir_entry_sector; // Dizin girdisinin bulundugu sektor (Root Dir veya Data Area)
    uint16_t dir_entry_offset; // Dizin girdisinin sektor icindeki offseti (byte)
    uint16_t current_dir_entry_index; // Dizin okunurken siradaki girdi indexi
};

// Dosya nesnesi durumu
#define FILE_STATE_UNUSED 0
#define FILE_STATE_OPEN   1
#define DIR_STATE_OPEN    2 // Dizin de bir tür açık dosya gibi ele alınabilir.

// Dosya sistemini belirtilen disk sürücüsünde başlatir (mount eder).
// drive_id: BIOS disk sürücüsü numarası (örn. 0x80).
// Donus degeri: 0 basari, -1 hata.
int fs_init(uint8_t drive_id);

// Belirtilen yoldaki (path) dosyayi veya dizini acar.
// path: Açılacak dosyanın veya dizinin yolu (örn. "\\DIR\\FILE.EXT").
// mode: Açma modu (salt okunur için "r"). Yazma modları ("w", "a") implemente edilmeyecek.
// Donus degeri: Açılan dosya/dizin nesnesine pointer veya hata durumunda NULL.
struct file_object *fs_open(const char *path, const char *mode);

// Açık dosyadan veri okur.
// file: Okuma yapılacak dosya nesnesine pointer.
// buffer: Okunan verinin yazilacagi buffer.
// count: Okunacak maksimum byte sayisi.
// Donus degeri: Gercekten okunan byte sayisi (0 EOF veya hata).
size_t fs_read(struct file_object *file, void *buffer, size_t count);

// Açık dosyada okuma/yazma pozisyonunu ayarlar (seek).
// file: Seek yapılacak dosya nesnesine pointer.
// offset: Hareket edilecek offset degeri.
// origin: Hareketin başlangic noktasi (FS_SEEK_SET, FS_SEEK_CUR, FS_SEEK_END).
// Donus degeri: 0 basari, -1 hata.
int fs_seek(struct file_object *file, long offset, int origin);

// Açık dosyayi veya dizini kapatir.
// file: Kapatılacak dosya nesnesine pointer.
// Donus degeri: 0 basari, -1 hata.
int fs_close(struct file_object *file);

// Açık dizinden siradaki dizin girdisini okur.
// dir_object: Okuma yapilacak dizin nesnesine pointer (fs_open ile acilmis bir dizin olmali).
// entry_buffer: Okunan dizin girdisinin kopyalanacagi 32 byte'lik buffer.
// Donus degeri: Okunan girdi bufferina pointer (entry_buffer) veya tum girdiler okunduysa/hata olursa NULL.
struct fat_dir_entry *fs_read_dir(struct file_object *dir_object, struct fat_dir_entry *entry_buffer);

// Yardımcı fonksiyonlar (fat.c veya fs.c icinde olabilir)
 uint32_t get_fat_entry(uint32_t cluster); // FAT'tan cluster degeri okur
 uint32_t cluster_to_lba(uint36_t cluster); // Cluster numarasini LBA sektör adresine çevirir

#endif // _FS_H