// fs.c
// Lİ-DOS Temel Dosya Sistemi Implementasyonu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Salt okunur FAT12/FAT16 dosya sistemi erisimi.

#include "fs.h" // Dosya sistemi arayuzu ve yapilari
#include "hd.h" // Düsük seviye disk erisimi
#include "printk.h" // Debug cikti icin
// Temel string ve bellek fonksiyonlari (string.h yerine kernel implementasyonu)
extern void *memcpy(void *dest, const void *src, size_t n);
extern void *memset(void *s, int c, size_t n);
extern int strcmp(const char *s1, const char *s2); // String karsilastirma
// extern int strncmp(const char *s1, const char *s2, size_t n); // n karakter karsilastirma
extern size_t strlen(const char *s); // String uzunlugu

// --- Dahili Degiskenler ---

static struct vbpb current_vbpb; // Monte edilen volümün VBPB bilgileri
static uint32_t fat_start_sector; // FAT alaninin baslangic LBA sektoru
static uint32_t root_dir_start_sector; // Kök Dizin alaninin baslangic LBA sektoru (FAT12/16)
static uint32_t data_start_sector; // Veri alaninin baslangic LBA sektoru
static uint32_t sectors_per_fat; // Tek bir FAT kopyasinin sektor sayisi
static uint8_t fs_drive_id = 0; // Dosya sisteminin monte edildigi disk surucusu ID'si (BIOS)
static uint8_t fs_type = 0; // Monte edilen dosya sistemi tipi (FAT12 veya FAT16)

// Acik dosya/dizin nesneleri dizisi
static struct file_object open_files[MAX_OPEN_FILES];

// Disk sektorlerini okumak icin dahili bufferlar (64KB limiti icinde olmali)
// Bunlar kernel veri segmentinde global olarak tanimlanmistir.
static uint8_t fat_sector_buffer[SECTOR_SIZE]; // FAT sektorlerini okumak icin
static uint8_t data_sector_buffer[SECTOR_SIZE]; // Veri/Dizin sektorlerini okumak icin

// --- Dahili Yardimci Fonksiyonlar (fat.c ayri olsaydi orada olabilirdi) ---

// Verilen cluster numarasinin LBA sektör adresini hesaplar
// Cluster 0 ve 1 reserved, Data Area clusterlari 2'den baslar.
static uint32_t cluster_to_lba(uint16_t cluster) {
    // Cluster 2, data_start_sector'da baslar. Cluster N, N-2 * sectors_per_cluster sonra baslar.
    return data_start_sector + (uint32_t)(cluster - 2) * current_vbpb.sectors_per_cluster;
}

// FAT'tan belirtilen cluster numarasinin degerini okur.
// Donus degeri: Sonraki cluster numarasi, veya Ozel degerler (EOF, Bad Cluster vb.)
static uint32_t get_fat_entry(uint16_t cluster) {
    // FAT12 ve FAT16 icin farkli okuma mantigi
    uint32_t fat_entry_value = 0;
    uint32_t fat_offset; // FAT icindeki byte offseti
    uint32_t fat_sector; // FAT alanindaki sektor offseti
    uint16_t fat_sector_offset; // FAT sektoru icindeki byte offseti
    uint8_t error;

    if (fs_type == FS_TYPE_FAT12) {
        // FAT12: Her girdi 1.5 byte (12 bit)
        fat_offset = cluster + (cluster / 2); // cluster * 1.5
        fat_sector = fat_start_sector + (fat_offset / SECTOR_SIZE);
        fat_sector_offset = fat_offset % SECTOR_SIZE;

        // FAT sektorunu oku (eger bufferda degilse veya degismisse)
        // Basit ornek: Her zaman oku. Gelişmişte cache kullanilir.
        error = hd_read_sectors_chs(fs_drive_id, 0, 0, fat_sector, 1, seg(fat_sector_buffer), offset(fat_sector_buffer));
        if (error) {
             printk("FS Error: Reading FAT12 sector 0x%lx failed (cluster %u)\n", fat_sector, cluster);
             return 0xFFF7; // Bad cluster gibi bir hata degeri
        }

        // FAT12 degerini bufferdan al
        fat_entry_value = *(uint16_t *)((uint8_t *)fat_sector_buffer + fat_sector_offset);

        // Cluster numarasi cift mi tek mi?
        if (cluster & 0x01) { // Tek cluster (örn: cluster 3, byte 4 ve 5'in yuksek 4 biti ve byte 6'nın tamamı)
            fat_entry_value >>= 4; // Yuksek 12 biti al
        } else { // Cift cluster (örn: cluster 2, byte 3'un tamamı ve byte 4 ve 5'in dusuk 4 biti)
             fat_entry_value &= 0xFFF; // Dusuk 12 biti al
        }
        fat_entry_value &= 0xFFF; // Sadece 12 bit al

        // FAT12 son cluster/EOF işaretleri 0xFF8 - 0xFFF
        if (fat_entry_value >= 0xFF8) fat_entry_value |= 0xFFF00000; // EOF/Bad Cluster bayraklarini 32-bit'e yay
                                                                    // FAT12 sonu 0xFFF

    } else if (fs_type == FS_TYPE_FAT16) {
        // FAT16: Her girdi 2 byte (16 bit)
        fat_offset = cluster * 2;
        fat_sector = fat_start_sector + (fat_offset / SECTOR_SIZE);
        fat_sector_offset = fat_offset % SECTOR_SIZE;

        // FAT sektorunu oku
        error = hd_read_sectors_chs(fs_drive_id, 0, 0, fat_sector, 1, seg(fat_sector_buffer), offset(fat_sector_buffer));
        if (error) {
             printk("FS Error: Reading FAT16 sector 0x%lx failed (cluster %u)\n", fat_sector, cluster);
             return 0xFFF7; // Bad cluster gibi bir hata degeri
        }

        // FAT16 degerini bufferdan al (Little Endian varsayim)
        fat_entry_value = *(uint16_t *)((uint8_t *)fat_sector_buffer + fat_sector_offset);

        // FAT16 son cluster/EOF işaretleri 0xFFF8 - 0xFFFF
        if (fat_entry_value >= 0xFFF8) fat_entry_value |= 0xFFF00000; // EOF/Bad Cluster bayraklarini 32-bit'e yay

    } else {
         // Bilinmeyen FS tipi
         return 0xFFF7; // Hata
    }

    return fat_entry_value;
}


// Dosya veya dizin ismini 8.3 formatına cevirir (basit)
// Kaynak stringdeki karakterleri alir, 8+3 formatina yerlestirir, boslukla doldurur.
// "." ve ".." isaretlenmemis. Kucuk harfi buyuk harfe cevirebilir.
// Donus: Hedef buffer (8+3+null). Hata durumunda bos string?
static void format_filename_8_3(const char *filename, char *output_8_3) {
    int i, j;
    // 8 byte filename + 3 byte extension + null terminator = 12 byte
    memset(output_8_3, ' ', 11); // 11 byte'i boslukla doldur
    output_8_3[11] = '\0';

    // Filename kismini isle (ilk 8 karakter veya '.' oncesi)
    i = 0; // output_8_3 index
    j = 0; // filename index
    while (j < 8 && filename[j] != '\0' && filename[j] != '.') {
        char c = filename[j];
        // Kucuk harfi buyuk harfe cevir (A-Z = a-z - 32)
        if (c >= 'a' && c <= 'z') c -= 32;
        output_8_3[i++] = c;
        j++;
    }

    // '.' karakterini bul ve uzantiya gec
    while (filename[j] != '\0' && filename[j] != '.') {
        j++; // '.' veya null terminator'a kadar atla
    }

    if (filename[j] == '.') {
        j++; // '.' karakterini atla
        i = 8; // Uzanti kismina gec (output_8_3 index 8'den baslar)
        while (i < 11 && filename[j] != '\0') {
             char c = filename[j];
             if (c >= 'a' && c <= 'z') c -= 32;
             output_8_3[i++] = c;
             j++;
        }
    }
    // Kalan 11 byte boslukla doludur
}

// Iki 8.3 formatli dosya ismini karsilastirir (bosluk doldurma ve case-insensitive dikkate alinarak)
// entry_filename_8_3: Dizin girdisindeki 8.3 formatli isim (11 byte + null)
// search_filename_8_3: Aranan ismin 8.3 formatli hali (11 byte + null)
// Donus: 0 eslesirse, degilse 0 disi.
static int compare_filenames_8_3(const uint8_t *entry_filename_11, const char *search_filename_11) {
    int i;
    for (i = 0; i < 11; i++) {
        // Hem dizin girdisi ismindeki karakter hem de aranan isimdeki karakter (ki bu buyuk harfe cevrilmis ve bosluk doldurulmustur) karsilastirilir.
        // Dizin girdisindeki isim zaten buyuk harf ve bosluk doldurulmus kabul edilebilir (LFN haric).
        if (entry_filename_11[i] != (uint8_t)search_filename_11[i]) {
             return 1; // Eslesmedi
        }
    }
    return 0; // Eslesti
}


// Dizin girdilerini okumak ve isim eslestirmek icin yardimci.
// current_dir_cluster: Su anki dizinin ilk clusteri (Root icin 0 veya ozel deger)
// path_component_8_3: Aranan dosya/dizin isminin 8.3 formatli hali (11 byte)
// bulunan_entry: Bulunan dizin girdisinin kopyalanacagi buffer (32 byte)
// Donus: Bulunan girdinin ilk clusteri, bulunamazsa 0. Hata durumunda ozel deger?
// Basitlik icin sadece Root Directory (current_dir_cluster = 0) arama yapar.
static uint16_t find_entry_in_root_dir(const char *path_component_8_3, struct fat_dir_entry *found_entry) {
    uint32_t sector_lba;
    uint16_t i, j;
    uint8_t error;

    uint32_t root_dir_sector_count = (uint32_t)(current_vbpb.root_entry_count * 32 + SECTOR_SIZE - 1) / SECTOR_SIZE;

    // Kök Dizin sektorlerini dolas
    for (i = 0; i < root_dir_sector_count; i++) {
        sector_lba = root_dir_start_sector + i;

        // Sektoru oku
        error = hd_read_sectors_chs(fs_drive_id, 0, 0, sector_lba, 1, seg(data_sector_buffer), offset(data_sector_buffer));
        if (error) {
             printk("FS Error: Reading Root Dir sector 0x%lx failed.\n", sector_lba);
             return 0; // Hata
        }

        // Sektordeki dizin girdilerini dolas (her sektor 16 girdi icerir)
        for (j = 0; j < 16; j++) {
            struct fat_dir_entry *entry = (struct fat_dir_entry *)((uint8_t *)data_sector_buffer + j * sizeof(struct fat_dir_entry));

            // Gecersiz girdileri veya bos yerleri atla
            if (entry->filename[0] == 0x00) {
                 // Bu noktadan sonraki girdiler de bos (dizin sonu) - Root Dir icin gecerli
                 return 0; // Bulunamadi
            }
            if (entry->filename[0] == 0xE5) {
                 continue; // Silinmis girdi
            }
            // LFN (Uzun Dosya Adi) girdilerini atla (Salt okunur basit implementasyonda desteklenmez)
            if (entry->attribute & FAT_ATTR_LONG_NAME) {
                continue;
            }

            // Dosya ismini karsilastir (8.3 formatinda ve buyuk harfe cevrilmis olmali)
            if (compare_filenames_8_3(entry->filename, path_component_8_3) == 0) {
                 // İsimler eşleşti
                 // Bulunan girdiyi buffer'a kopyala
                 memcpy(found_entry, entry, sizeof(struct fat_dir_entry));
                 // Ilk cluster numarasini dondur
                 return entry->first_cluster_low; // FAT12/16'da high word 0'dır.
            }
        }
    }

    // Kök dizinde bulunamadi
    return 0; // Bulunamadi
}

// --- Ana Dosya Sistemi Fonksiyonlari ---

// Dosya sistemini belirtilen disk sürücüsünde başlatir (mount eder).
int fs_init(uint8_t drive_id) {
    uint8_t error;
    struct __attribute__((packed)) vbpb *vbpb_ptr = (struct __attribute__((packed)) vbpb *)data_sector_buffer; // VBPB'yi okumak icin buffer uzerinde pointer

    fs_drive_id = drive_id;

    // Boot sektoru (sektor 0) oku
    error = hd_read_sectors_chs(fs_drive_id, 0, 0, 0, 1, seg(data_sector_buffer), offset(data_sector_buffer));
    if (error) {
        printk("FS Init Error: Reading boot sector failed (drive 0x%x, error 0x%x)\n", fs_drive_id, error);
        return -1;
    }

    // Boot imzasi (0xAA55) kontrolu
    if (vbpb_ptr->boot_signature != 0xAA55) {
        printk("FS Init Error: Invalid boot sector signature (0x%x) on drive 0x%x.\n", vbpb_ptr->boot_signature, fs_drive_id);
        return -1;
    }

    // VBPB bilgilerini kopyala/parse et (packed yapi uzerinden erisim)
    // Struct copy yeterli olmali eger packed dogru calisiyorsa
    memcpy(&current_vbpb, vbpb_ptr, sizeof(struct vbpb));

    // Temel VBPB degeri kontrolleri (ornektir)
    if (current_vbpb.bytes_per_sector != SECTOR_SIZE || current_vbpb.num_fats == 0) {
         printk("FS Init Error: Invalid VBPB values on drive 0x%x.\n", fs_drive_id);
         return -1;
    }

    // FAT tipini belirle (Basit yöntemler: Root Dir boyutu ve Toplam Sektor sayisi)
    // FAT12: Root Entry Count > 0, Sectors Per FAT 16 > 0, Total Sectors 16 > 0
    // FAT16: Root Entry Count > 0, Sectors Per FAT 16 > 0, Total Sectors 16 > 0
    // Gerçek FAT tipi belirleme, Data Area'daki cluster sayisina bakar.
    // TotalDataSectors = TotalSectors - ReservedSectors - (NumFATs * SectorsPerFAT) - RootDirSectors
    // NumClusters = TotalDataSectors / SectorsPerCluster
    // FAT12: NumClusters < 4085
    // FAT16: 4085 <= NumClusters < 65525
    // Basit ornekte SectorsPerFAT16'ya bakarak belirleyelim (tam dogru degil ama fikir verir)
    if (current_vbpb.sectors_per_fat_16 == 0) {
        // FAT32 olabilir veya hata. Basit implementasyonda FAT32 desteklenmez.
         printk("FS Init Error: Probably FAT32 or invalid FAT type on drive 0x%x.\n", fs_drive_id);
         return -1;
    }
    sectors_per_fat = current_vbpb.sectors_per_fat_16; // FAT12/16 icin ayni alan kullanilir.

    // Bu basit ornek SectorsPerFAT16 > 0 ise FAT16 oldugunu varsayacak (Yanlis olabilir!)
    // Doğrusu cluster sayisini hesaplamak:
    uint32_t root_dir_sectors = (uint32_t)(current_vbpb.root_entry_count * 32 + SECTOR_SIZE - 1) / SECTOR_SIZE;
    uint32_t total_sectors = (current_vbpb.total_sectors_16 == 0) ? current_vbpb.total_sectors_32 : current_vbpb.total_sectors_16;
    uint32_t data_sectors = total_sectors - current_vbpb.reserved_sectors - (uint32_t)current_vbpb.num_fats * sectors_per_fat - root_dir_sectors;
    uint32_t num_clusters = data_sectors / current_vbpb.sectors_per_cluster;

    if (num_clusters < 4085) {
        fs_type = FS_TYPE_FAT12;
        // printk("Detected FAT12 file system.\n");
    } else { // >= 4085 ve < 65525 (idealde)
        fs_type = FS_TYPE_FAT16;
        // printk("Detected FAT16 file system.\n");
    }
    // Cluster sayisi cok buyukse FAT32'dir (bu ornek desteklemez).

    // Alanlarin baslangic sektorlerini hesapla
    fat_start_sector = current_vbpb.reserved_sectors;
    root_dir_start_sector = fat_start_sector + (uint32_t)current_vbpb.num_fats * sectors_per_fat;
    data_start_sector = root_dir_start_sector + root_dir_sectors;

    // Acik dosya nesneleri dizisini baslat
    for (i = 0; i < MAX_OPEN_FILES; i++) {
        open_files[i].state = FILE_STATE_UNUSED;
    }

    printk("FS Initialized: Drive 0x%x, Type FAT%u, Root Dir @ 0x%lx, Data @ 0x%lx\n", fs_drive_id, fs_type, root_dir_start_sector, data_start_sector);
    return 0;
}

// Belirtilen yoldaki (path) dosyayi veya dizini acar.
struct file_object *fs_open(const char *path, const char *mode) {
    int i;
    struct file_object *file = (struct file_object *)0;
    uint16_t current_dir_cluster = 0; // 0 Root Directory'i temsil etsin
    const char *path_ptr = path;
    char path_component_8_3[12]; // 8.3 format + null

    // Sadece salt okunur modu ("r") destekle
    if (!mode || mode[0] != 'r' || mode[1] != '\0') {
         printk("FS Open Error: Unsupported mode '%s'\n", mode);
         return (struct file_object *)0;
    }

    // Bos bir acik dosya yuvasi bul
    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].state == FILE_STATE_UNUSED) {
            file = &open_files[i];
            break;
        }
    }

    if (!file) {
        printk("FS Open Error: Too many open files.\n");
        return (struct file_object *)0; // Acik yuva yok
    }

    // Yolun basindaki "\\" isaretini atla
    if (path_ptr[0] == '\\') {
        path_ptr++;
    }

    // Yolun her bir bilesenini isle
    // Bu implementasyon sadece kok dizindeki dosyalari acar (tek seviye).
    // Alt dizinleri islemek daha karmasik.
    // while (*path_ptr != '\0') { ... } // Gercek implementasyon burada dongu icerir.

    // Basit ornek: Yolun tamamini kok dizinde aranan dosya olarak isle.
    // Path bileseni ayrıştırması yapılmaz, tüm yol son bilesen kabul edilir.
    // Yani "\\FILE.EXT" veya "FILE.EXT" kok dizinde aranir.
    // Alt dizinler desteklenmez.

    // Yolun tamamini aranan dosya adi olarak al ve 8.3 formatina cevir.
    format_filename_8_3(path_ptr, path_component_8_3);

    // Kok dizinde girdiyi ara
    struct fat_dir_entry found_entry_buffer;
    uint16_t first_cluster = find_entry_in_root_dir(path_component_8_3, &found_entry_buffer);

    if (first_cluster == 0 && (found_entry_buffer.filename[0] == 0 || found_entry_buffer.filename[0] == 0xE5)) {
         // find_entry_in_root_dir 0 dondurduyse ve buffer bos kaldiysa bulunamadi demektir.
         printk("FS Open Error: File or directory '%s' not found.\n", path_ptr);
         file->state = FILE_STATE_UNUSED; // Kullanilmayan duruma geri al
         return (struct file_object *)0; // Bulunamadi
    }
     if (first_cluster == 0 && !(found_entry_buffer.attribute & FAT_ATTR_DIRECTORY)) {
         // find_entry_in_root_dir 0 dondurdu ama bulunan sey bir dosyaydi (Root dirde ilk cluster 0 olmaz)
         // Bu durum find_entry_in_root_dir'in 0 dondurme mantigina baglidir.
         // find_entry_in_root_dir'in buldugu entrynin ilk clusterini veya 0 (bulunamadi) dondurmesi daha dogru.
         // Yukaridaki find_entry_in_root_dir, bulunan girdinin clusterini veya 0 (bulunamadi/hata) donduruyor.
         // Eger 0 dondurduyse ve buffer bos degilse bu Root Dirin kendisi olabilir (Root dir clusteri 0 olarak ele aliniyorsa)
         // Veya hata olmuştur. find_entry_in_root_dir'in donus mantigina gore islem yapilmali.
         // Varsayalim ki find_entry_in_root_dir bulunamazsa veya hata olursa 0 dondurur.
         // Ozel olarak Root Directory'i acmak icin "/" veya "\\" yolu kullanilabilir.
         // "FILE.EXT" aramasinda 0 gelirse bulunamadi demektir.
          if (strlen(path_ptr) == 0 || (strlen(path_ptr) == 1 && path_ptr[0] == '\\')) {
              // Kok dizini acma istegi
              // Kok dizin ozel bir dosya nesnesi olarak ele alinir.
              file->state = DIR_STATE_OPEN;
              file->attributes = FAT_ATTR_DIRECTORY;
              file->size = current_vbpb.root_entry_count * 32; // Root Dir boyutu
              file->first_cluster = 0; // Root Dir icin ozel cluster degeri
              file->current_offset = 0;
              file->current_cluster = 0;
              file->offset_in_cluster = 0;
              file->current_dir_entry_index = 0; // Dizin okuma icin
              printk("FS Open: Root Directory opened.\n");
              return file;
          } else {
              // Normal dosya/dizin bulunamadi
              printk("FS Open Error: File or directory '%s' not found.\n", path_ptr);
              file->state = FILE_STATE_UNUSED; // Kullanilmayan duruma geri al
              return (struct file_object *)0; // Bulunamadi
          }
    }


    // Girdi bulundu. Dosya nesnesini doldur.
    if (found_entry_buffer.attribute & FAT_ATTR_DIRECTORY) {
         // Dizin aciliyor
         file->state = DIR_STATE_OPEN;
         file->attributes = FAT_ATTR_DIRECTORY;
         file->size = 0xFFFFFFFF; // Dizin boyutu FAT'ta 0'dir, sonsuz gibi dusunulebilir veya sektor/cluster bazinda hesaplanir
         file->first_cluster = first_cluster;
         file->current_offset = 0;
         file->current_cluster = first_cluster; // Ilk cluster ile basla
         file->offset_in_cluster = 0;
         file->current_dir_entry_index = 0; // Dizin okuma icin
         printk("FS Open: Directory '%s' opened (cluster %u).\n", path_ptr, first_cluster);
    } else {
         // Dosya aciliyor
         file->state = FILE_STATE_OPEN;
         file->attributes = found_entry_buffer.attribute; // Dosya ozellikleri
         file->size = found_entry_buffer.file_size; // Dosya boyutu
         file->first_cluster = first_cluster;
         file->current_offset = 0;
         file->current_cluster = first_cluster; // Ilk cluster ile basla
         file->offset_in_cluster = 0;
         printk("FS Open: File '%s' opened (cluster %u, size %lu).\n", path_ptr, first_cluster, file->size);
    }


    return file; // Açılan dosya nesnesine pointer döndür
}

// Açık dosyadan veri okur.
size_t fs_read(struct file_object *file, void *buffer, size_t count) {
    uint32_t bytes_to_read = (uint32_t)count;
    uint32_t bytes_read_total = 0;
    uint32_t remaining_in_file;
    uint32_t current_sector_lba;
    uint16_t sector_in_cluster;
    uint16_t offset_in_sector;
    uint16_t bytes_in_sector;
    uint8_t error;

    // Gecerlilik kontrolu
    if (!file || file->state != FILE_STATE_OPEN || !buffer || count == 0) {
        // printk("FS Read Error: Invalid file object or parameters.\n");
        return 0; // Gecersiz nesne veya parametre
    }
    if (file->attributes & FAT_ATTR_DIRECTORY) {
         // printk("FS Read Error: Cannot read file data from a directory object.\n");
         return 0; // Dizin objesinden dosya verisi okunamaz
    }


    // Dosya sonuna kadar kalan byte sayisi
    if (file->current_offset >= file->size) {
         // printk("FS Read: EOF reached.\n");
         return 0; // Dosya sonuna ulasildi
    }
    remaining_in_file = file->size - file->current_offset;

    // Gerçekten okunacak byte sayisi (istenen miktar veya dosya sonuna kalan)
    if (bytes_to_read > remaining_in_file) {
        bytes_to_read = remaining_in_file;
    }
    if (bytes_to_read == 0) return 0;

    // Okuma döngüsü
    while (bytes_read_total < bytes_to_read) {
        // Hali hazirda uzerinde bulundugumuz cluster veya bir sonraki cluster hesaplanmali
        // current_cluster ve offset_in_cluster dosya nesnesinde tutuluyor.

        // Eger mevcut cluster gecerli degilse (ilk okuma veya cluster sonu)
        if (file->current_cluster == 0 || file->offset_in_cluster >= current_vbpb.sectors_per_cluster * SECTOR_SIZE) {
             // Bir sonraki clustera gec
             if (file->current_cluster == 0) {
                  // Dosyanin ilk clusteri (Root dir icin 0 olmayacak, dosya icin >1)
                  file->current_cluster = file->first_cluster;
             } else {
                  // FAT'tan bir sonraki cluster numarasini al
                  uint32_t next_c = get_fat_entry(file->current_cluster);
                  if (next_c >= 0xFFF8) { // FAT16 EOF veya Bad Cluster (FAT12 icin 0xFF8)
                       // Dosya sonu veya Bad cluster
                       // printk("FS Read: Reached EOF marker or bad cluster in FAT chain.\n");
                       break; // Donguyu bitir
                  }
                  file->current_cluster = (uint16_t)next_c;
             }
             file->offset_in_cluster = 0; // Yeni clusterin basina git

             if (file->current_cluster == 0) {
                  // printk("FS Read Error: File has 0 as first cluster (invalid).\n");
                  break; // Gecersiz cluster
             }
        }

        // Hali hazirda uzerinde bulunulan clusterin LBA adresini hesapla
        current_sector_lba = cluster_to_lba(file->current_cluster);

        // Cluster icindeki offsete gore okunacak sektor ve sektor ici offseti hesapla
        sector_in_cluster = file->offset_in_cluster / SECTOR_SIZE;
        offset_in_sector = file->offset_in_cluster % SECTOR_SIZE;

        // Okuma yapilacak sektorun LBA adresini hesapla
        uint32_t sector_to_read = current_sector_lba + sector_in_cluster;

        // Sektorden okunacak byte sayisi (ya sektor sonuna kadar, ya istenen miktar kadar, ya da cluster sonuna kadar kalan)
        bytes_in_sector = SECTOR_SIZE - offset_in_sector;
        uint32_t remaining_in_cluster = (current_vbpb.sectors_per_cluster * SECTOR_SIZE) - file->offset_in_cluster;

        uint16_t read_len = bytes_in_sector;
        if (read_len > (bytes_to_read - bytes_read_total)) read_len = (uint16_t)(bytes_to_read - bytes_read_total); // Kalan okunacak miktar
        if (read_len > remaining_in_cluster) read_len = (uint16_t)remaining_in_cluster; // Cluster sonu

        if (read_len == 0) break; // Okunacak bir sey kalmadi

        // Sektoru dahili buffera oku
        error = hd_read_sectors_chs(fs_drive_id, 0, 0, sector_to_read, 1, seg(data_sector_buffer), offset(data_sector_buffer));
        if (error) {
             printk("FS Read Error: Reading data sector 0x%lx failed (error 0x%x).\n", sector_to_read, error);
             break; // Hata durumunda donguyu bitir
        }

        // Dahili bufferdan kullanici bufferina kopyala
        memcpy((uint8_t *)buffer + bytes_read_total, data_sector_buffer + offset_in_sector, read_len);

        // Okuma pozisyonlarini guncelle
        bytes_read_total += read_len;
        file->current_offset += read_len;
        file->offset_in_cluster += read_len;
        // file->current_cluster, offset_in_cluster >= cluster_size ise bir sonraki iterasyonda guncellenecek.
    }

    return (size_t)bytes_read_total; // Toplam okunan byte sayisini dondur
}

// Açık dosyada okuma/yazma pozisyonunu ayarlar (seek).
int fs_seek(struct file_object *file, long offset, int origin) {
    uint32_t new_offset;

    // Gecerlilik kontrolu
    if (!file || (file->state != FILE_STATE_OPEN && file->state != DIR_STATE_OPEN)) {
        // printk("FS Seek Error: Invalid file object.\n");
        return -1;
    }

    // Yeni offseti hesapla
    switch (origin) {
        case FS_SEEK_SET:
            new_offset = (uint32_t)offset;
            break;
        case FS_SEEK_CUR:
            new_offset = file->current_offset + (uint32_t)offset; // Isaretli offset olabilir long
            break;
        case FS_SEEK_END:
            if (file->attributes & FAT_ATTR_DIRECTORY) {
                 // printk("FS Seek Error: Cannot seek from end on directory.\n");
                 return -1; // Dizinler icin sonundan seek mantikli degil
            }
            new_offset = file->size + (uint32_t)offset; // Dosya boyutu + offset
            break;
        default:
            // printk("FS Seek Error: Invalid origin %d.\n", origin);
            return -1; // Gecersiz origin
    }

    // Gecerlilik kontrolu: Yeni offset negatif olmamali ve dosya boyutunu (genellikle) asmamali
    // FAT'ta dosya sonundan sonraya seek yapmak (lseek gibi) mumkundur ama yazma gerektirir.
    // Salt okunurda sadece dosya boyutu icinde seek yapilabilir.
    if ((long)new_offset < 0) { // Offset negatif olduysa (current_offset + negatif offset)
         // printk("FS Seek Error: Resulting offset is negative.\n");
         return -1;
    }
    if (!(file->attributes & FAT_ATTR_DIRECTORY) && new_offset > file->size) {
        // Dosya ise, boyuttan sonraya seek yapma
         // printk("FS Seek Warning: Seeking past end of file.\n");
         // Izin verelim ama okuma 0 byte dondurur. Veya -1 dondurulebilir.
         // Standarda yakin olmak icin izin verip okumada 0 dondurmek daha iyi.
    }

    // Yeni offseti ayarla
    file->current_offset = new_offset;
    // Cluster ve offset_in_cluster degerlerini sifirla, bir sonraki okumada yeniden hesaplanacaklar.
    // Daha gelismis implementasyonda bu seek aninda hesaplanabilir.
    file->current_cluster = 0; // Gecersiz hale getir, okuma basinda bulunacak
    file->offset_in_cluster = 0;

    // Dizin okuma pozisyonunu da sifirla (eger dizin ise)
    if (file->state == DIR_STATE_OPEN) {
        file->current_dir_entry_index = new_offset / 32; // Offsetten girdi indexini hesapla
    }


    // printk("FS Seek: Device %d, new offset %lu.\n", file - open_files, file->current_offset);
    return 0; // Basari
}

// Açık dosyayi veya dizini kapatir.
int fs_close(struct file_object *file) {
    // Gecerlilik kontrolu
    if (!file || (file->state != FILE_STATE_OPEN && file->state != DIR_STATE_OPEN)) {
        // printk("FS Close Error: Invalid file object.\n");
        return -1;
    }

    // Nesnenin durumunu bos (UNUSED) olarak ayarla
    file->state = FILE_STATE_UNUSED;

    // printk("FS Close: Device %d closed.\n", file - open_files);
    return 0; // Basari
}

// Açık dizinden siradaki dizin girdisini okur.
struct fat_dir_entry *fs_read_dir(struct file_object *dir_object, struct fat_dir_entry *entry_buffer) {
     uint32_t current_lba;
     uint16_t entry_sector_offset;
     uint16_t entry_sector_idx;
     uint16_t entry_in_sector_idx;
     uint8_t error;

     // Gecerlilik kontrolu
     if (!dir_object || dir_object->state != DIR_STATE_OPEN || !entry_buffer) {
         // printk("FS Read Dir Error: Invalid directory object or buffer.\n");
         return (struct fat_dir_entry *)0;
     }

     // Sadece Root Directory okuma desteklenir (simple ornek).
     if (dir_object->first_cluster != 0) {
          // printk("FS Read Dir Error: Reading subdirectories not supported in this example.\n");
          return (struct fat_dir_entry *)0;
     }

     // Root Directory girdilerini oku
     uint32_t root_dir_sector_count = (uint32_t)(current_vbpb.root_entry_count * 32 + SECTOR_SIZE - 1) / SECTOR_SIZE;

     // Okunacak bir sonraki girdi indexi gecerli mi?
     if (dir_object->current_dir_entry_index >= current_vbpb.root_entry_count) {
          // printk("FS Read Dir: End of directory.\n");
          dir_object->current_dir_entry_index = 0; // Sonuna gelindiyse sifirla (opsiyonel)
          return (struct fat_dir_entry *)0; // Dizin sonu
     }

     // Girdinin bulundugu sektor ve sektor ici offseti hesapla
     entry_sector_offset = dir_object->current_dir_entry_index / (SECTOR_SIZE / sizeof(struct fat_dir_entry)); // Sektor icindeki siralama
     entry_in_sector_idx = dir_object->current_dir_entry_index % (SECTOR_SIZE / sizeof(struct fat_dir_entry)); // Sektor icindeki pozisyon

     // Girdinin LBA sektor adresini hesapla
     current_lba = root_dir_start_sector + entry_sector_offset;

     // Sektoru oku
     error = hd_read_sectors_chs(fs_drive_id, 0, 0, current_lba, 1, seg(data_sector_buffer), offset(data_sector_buffer));
     if (error) {
          printk("FS Read Dir Error: Reading dir sector 0x%lx failed (error 0x%x).\n", current_lba, error);
          return (struct fat_dir_entry *)0; // Hata
     }

     // Dizin girdisini buffer'a kopyala
     memcpy(entry_buffer, (uint8_t *)data_sector_buffer + entry_in_sector_idx * sizeof(struct fat_dir_entry), sizeof(struct fat_dir_entry));

     // Bir sonraki girdi indexini guncelle
     dir_object->current_dir_entry_index++;

     // printk("FS Read Dir: Read entry %u from device %d.\n", dir_object->current_dir_entry_index -1, dir_object - open_files);

     // Okunan girdi bufferinin adresini dondur
     return entry_buffer;
}


// fs.c sonu