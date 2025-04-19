// pkg.c
// Lİ-DOS Paket Yöneticisi Implementasyonu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Minimal yazilim paketi kurulumu ve listeleme.

#include "pkg.h"
// Temel string/bellek fonksiyonlari (string.h yerine)
extern void *memcpy(void *dest, const void *src, size_t n);
extern void *memset(void *s, int c, size_t n);
extern int strcmp(const char *s1, const char *s2);
extern size_t strlen(const char *s);
// printk yerine tty_io kullanimi (daha üst seviye)
#define pkg_printf(...) // printk veya tty_printf benzeri fonksiyon tanimlanmali
                       // Veya basit tty_puts kullanilabilir.
#define printk tty_puts // Basitlik icin tty_puts kullanimi

// --- Dahili Fonksiyon Bildirimleri (Opsiyonel) ---
// Paket dosyasindan dosya verisini okuyup hedef yola yazar.
// static int pkg_extract_file(struct file_object *pkg_file, const struct pkg_file_entry *file_entry);

// --- Paket Yöneticisi Implementasyonu ---

void pkg_init(void) {
    // Paket veritabanı dosyasını (PKG_DB_PATH) kontrol et.
    // Yoksa oluştur.
    struct file_object *db_file = fs_open(PKG_DB_PATH, "r"); // Okuma modunda dene

    if (!db_file) {
        // Dosya yok veya açılamadı, yazma modunda oluşturmayı dene
        db_file = fs_open(PKG_DB_PATH, "w"); // Yazma modu (fs modülünde implemente edilmeli)
        if (!db_file) {
             printk("PKG Init Error: Veritabanı dosyası oluşturulamadı!\r\n");
        } else {
             printk("PKG Init: Veritabanı dosyası oluşturuldu.\r\n");
             fs_close(db_file);
        }
    } else {
        printk("PKG Init: Veritabanı dosyası bulundu.\r\n");
        fs_close(db_file);
    }

    // Kurulu paket listesini RAM'e yüklemek (minimalde yapılmaz) veya sadece kontrol için açık bırakmak
}

// Belirtilen paketi (dosya yolu) kurar.
int pkg_install(const char *package_filepath) {
    struct file_object *pkg_file = (struct file_object *)0;
    struct pkg_header pkg_hdr;
    struct pkg_file_entry file_entry;
    uint32_t bytes_read;
    int i;
    int status = PKG_STATUS_ERROR; // Varsayılan olarak hata

    printk("PKG: Paket '%s' kuruluyor...\r\n", package_filepath); // pkg_printf kullanilabilir

    // Paket dosyasını aç
    pkg_file = fs_open(package_filepath, "r");
    if (!pkg_file) {
        printk("PKG Error: Paket dosyası açılamadı!\r\n");
        return PKG_STATUS_FILE_ERROR;
    }

    // Paket Header'ını oku
    bytes_read = fs_read(pkg_file, &pkg_hdr, sizeof(struct pkg_header));
    if (bytes_read != sizeof(struct pkg_header) || pkg_hdr.magic != PKG_MAGIC) {
        printk("PKG Error: Gecersiz paket başlığı veya sihirli sayı!\r\n");
        fs_close(pkg_file);
        return PKG_STATUS_INVALID_PACKAGE;
    }

    // Paket adı ve versiyonunu göster
    printk("  Paket Adı: %s, Versiyon: %s\r\n", pkg_hdr.pkg_name, pkg_hdr.pkg_version);

    // Paketin zaten kurulu olup olmadığını kontrol et
    if (pkg_is_installed(pkg_hdr.pkg_name) == 1) {
         printk("PKG Error: Paket '%s' zaten kurulu!\r\n", pkg_hdr.pkg_name);
         fs_close(pkg_file);
         return PKG_STATUS_ALREADY_INSTALLED;
    }

    // Dosyaları paketten çıkar ve diske yaz
    printk("  %u dosya çıkarılıyor...\r\n", pkg_hdr.num_files); // pkg_printf kullanilabilir

    // Her dosya girdisini oku ve ilgili dosyayı çıkar
    for (i = 0; i < pkg_hdr.num_files; i++) {
        // Dosya girdisi header'dan hemen sonra başlar. Dosya girdilerini okumak için seek yap
        // Veya header'dan sonra peş peşe okuyabiliriz.
        // Basitlik icin seek yapalim: i * sizeof(struct pkg_file_entry) kadar offset.
        uint32_t entry_offset = sizeof(struct pkg_header) + i * sizeof(struct pkg_file_entry);
        fs_seek(pkg_file, entry_offset, FS_SEEK_SET); // Dosya girdisine seek yap

        // Dosya girdisini oku
        bytes_read = fs_read(pkg_file, &file_entry, sizeof(struct pkg_file_entry));
        if (bytes_read != sizeof(struct pkg_file_entry)) {
            printk("PKG Error: Dosya girdisi okunamadı (index %d)!\r\n", i);
            status = PKG_STATUS_INVALID_PACKAGE; // Gecersiz paket formatı
            goto cleanup; // Temizliğe git
        }

        // Dosya adını ve boyutunu göster
        printk("    - %s (%lu bytes)... ", file_entry.filename, file_entry.file_size);

        // Dosya verisini paketten oku ve hedef yola yaz.
        // Bu en karmasik kisimdir. fs modülünün yazma desteği ve dizin oluşturma
        // yeteneği olmalıdır. pkg_extract_file yardimcisi kullanilabilir.
        // int extract_status = pkg_extract_file(pkg_file, &file_entry); // Bu fonksiyon implemente edilmeli

        // --- Basit Dosya Çıkarma Mantığı (Gerekli FS Yazma Fonksiyonları Varsayılarak) ---
        struct file_object *dest_file = (struct file_object *)0;
        char file_data_buffer[SECTOR_SIZE]; // Dosya verisini okumak icin buffer

        // Hedef dosyayi yazma modunda ac/olustur
        // FS modülünde dosya oluşturma ve yazma desteği OLMALIDIR.
        dest_file = fs_open(file_entry.filename, "w"); // "w" modu (yazma + olusturma)
        if (!dest_file) {
            // Dizin yoksa olusturmayı dene (daha da karmasik)
            // fs_create_dir fonksiyonu gereklidir.
            // Ornek: Eger yolda '/' varsa, ust dizini olusturmayi dene.
            printk("HATA: Hedef dosya '%s' yazılamadı (FS yazma hatası).\r\n", file_entry.filename);
            status = PKG_STATUS_FILE_ERROR;
            goto cleanup;
        }

        // Paket icindeki dosya verisinin başlangıcına seek yap
        fs_seek(pkg_file, file_entry.data_offset, FS_SEEK_SET);

        // Dosya verisini parcaciklar halinde oku ve hedef dosyaya yaz
        uint32_t bytes_to_copy = file_entry.file_size;
        uint32_t bytes_copied_in_file = 0;
        while (bytes_copied_in_file < bytes_to_copy) {
            size_t read_len = (bytes_to_copy - bytes_copied_in_file < SECTOR_SIZE) ? (size_t)(bytes_to_copy - bytes_copied_in_file) : SECTOR_SIZE;

            // Paket dosyasindan veriyi oku
            bytes_read = fs_read(pkg_file, file_data_buffer, read_len);
            if (bytes_read == 0) {
                 printk("HATA: Dosya verisi okunamadı (Paket içinde)!\r\n");
                 fs_close(dest_file);
                 status = PKG_STATUS_INVALID_PACKAGE; // Paket bozuk
                 goto cleanup;
            }

            // Hedef dosyaya yaz
            size_t bytes_written = fs_write(dest_file, file_data_buffer, bytes_read); // fs_write implemente edilmeli
            if (bytes_written != bytes_read) {
                printk("HATA: Dosya verisi yazılamadı (Diske yazma hatası)!\r\n");
                 fs_close(dest_file);
                 status = PKG_STATUS_FILE_ERROR;
                 goto cleanup;
            }

            bytes_copied_in_file += bytes_read;
        }

        fs_close(dest_file); // Hedef dosyayi kapat
        printk("TAMAM.\r\n");
    }

    // --- Kurulum Başarılı, Veritabanını Güncelle ---
    printk("  Veritabanı güncelleniyor...\r\n");
    struct file_object *db_file = fs_open(PKG_DB_PATH, "a"); // Ekleme (append) modunda ac
    if (!db_file) {
        printk("PKG Error: Veritabanı dosyası açılamadı (ekleme)! Kurulum tamamlandı ama DB güncellenmedi.\r\n");
        // Kurulumu basarili kabul edelim ama DB hatasi verelim.
        status = PKG_STATUS_DB_ERROR;
        goto cleanup;
    }

    // Veritabanına paket adını ve versiyonu yaz
    fs_write(db_file, pkg_hdr.pkg_name, strlen(pkg_hdr.pkg_name));
    fs_putc(db_file->id, ' '); // Boşluk yaz
    fs_write(db_file, pkg_hdr.pkg_version, strlen(pkg_hdr.pkg_version));
    fs_putc(db_file->id, '\r'); // Satır sonu (CRLF)
    fs_putc(db_file->id, '\n');

    fs_close(db_file); // Veritabanı dosyasını kapat
    printk("  Veritabanı güncellendi.\r\n");


    status = PKG_STATUS_OK; // Her şey yolunda

cleanup:
    fs_close(pkg_file); // Paket dosyasını kapat
    if (status == PKG_STATUS_OK) {
        printk("PKG: Paket '%s' başarıyla kuruldu.\r\n", pkg_hdr.pkg_name);
    } else {
        printk("PKG: Paket '%s' kurulumu HATA ile sonlandı.\r\n", package_filepath);
    }
    return status;
}

// Kurulu paketlerin bir listesini ekrana yazar.
int pkg_list_installed(void) {
    struct file_object *db_file = (struct file_object *)0;
    char db_buffer[256]; // Veritabanını okumak icin buffer
    uint32_t bytes_read;
    int status = PKG_STATUS_ERROR;

    printk("PKG: Kurulu Paketler:\r\n");

    // Veritabanı dosyasını aç
    db_file = fs_open(PKG_DB_PATH, "r");
    if (!db_file) {
        printk("  Veritabanı dosyası bulunamadı veya açılamadı!\r\n");
        return PKG_STATUS_DB_ERROR;
    }

    // Dosya içeriğini okuyup ekrana yaz (basit: tüm dosyayı tek seferde oku veya parcali)
    // Buffer boyutu yeterli degilse parcali okuma/isleme yapilmalidir.
    bytes_read = fs_read(db_file, db_buffer, sizeof(db_buffer) - 1); // Buffer tasmamasi icin 1 eksik oku

    if (bytes_read > 0) {
        db_buffer[bytes_read] = '\0'; // Null terminator ekle
        // Buffer icerigini parse et ve her satiri (paket adi + versiyonu) ekrana yaz.
        // printk("%s\r\n", db_buffer); // Basit: Tüm bufferi yazdir
        // Gercek parsing: satır satır ayır, # yorum satırlarını atla vb.

        char *line_start = db_buffer;
        char *line_end;

        while ((size_t)(line_start - db_buffer) < bytes_read) {
            line_end = line_start;
            // Satir sonunu bul (\r veya \n)
            while ((size_t)(line_end - db_buffer) < bytes_read && *line_end != '\r' && *line_end != '\n') {
                line_end++;
            }

            // Satir sonunu null terminator ile gecici olarak degistir
            char original_char = *line_end;
            *line_end = '\0';

            // Satiri yazdir (paket adi ve versiyonu)
            printk("  - %s\r\n", line_start);

            // Orijinal karakteri geri koy ve bir sonraki satira atla
            *line_end = original_char;
            line_start = line_end;
            // Satir sonu karakterlerini (CR ve LF) atla
            while ((size_t)(line_start - db_buffer) < bytes_read && (*line_start == '\r' || *line_start == '\n')) {
                line_start++;
            }
        }

    } else {
        printk("  (Veritabanı boş)\r\n");
    }

    fs_close(db_file);
    status = PKG_STATUS_OK;

    return status;
}

// Belirtilen isme sahip paketin kurulu olup olmadığını kontrol eder.
int pkg_is_installed(const char *package_name) {
    struct file_object *db_file = (struct file_object *)0;
    char db_buffer[256]; // Veritabanını okumak icin buffer
    uint32_t bytes_read;

    if (!package_name || package_name[0] == '\0') return 0;

    db_file = fs_open(PKG_DB_PATH, "r");
    if (!db_file) {
        // printk("PKG Is Installed Error: Veritabanı dosyası bulunamadı.\r\n");
        return PKG_STATUS_DB_ERROR; // Veritabanı yoksa kurulu degildir (veya hata)
    }

    // Veritabanı içeriğini oku
    bytes_read = fs_read(db_file, db_buffer, sizeof(db_buffer) - 1);
    fs_close(db_file); // Dosyayi kapat

    if (bytes_read == 0) {
        return 0; // Veritabanı boş, kurulu degil
    }
    db_buffer[bytes_read] = '\0'; // Null terminator ekle

    // Buffer icerigini parse et ve paket isimlerini karsilastir
    char *line_start = db_buffer;
    char *line_end;

    while ((size_t)(line_start - db_buffer) < bytes_read) {
        line_end = line_start;
        // Satir sonunu bul
         while ((size_t)(line_end - db_buffer) < bytes_read && *line_end != '\r' && *line_end != '\n') {
             line_end++;
         }

        // Paket adi kisminin sonunu bul (ilk bosluk)
        char *name_end = line_start;
         while (name_end < line_end && *name_end != ' ') {
             name_end++;
         }

        // Paket adi kismini gecici olarak null terminate et
        char original_char = *name_end;
        *name_end = '\0';

        // Paket adını karşılaştır
        int names_match = strcmp(line_start, package_name);

        // Orijinal karakteri geri koy
        *name_end = original_char;

        if (names_match == 0) {
             // İsim eşleşti, paket kurulu
             return 1;
        }

        // Bir sonraki satira atla
        line_start = line_end;
         while ((size_t)(line_start - db_buffer) < bytes_read && (*line_start == '\r' || *line_start == '\n')) {
             line_start++;
         }
    }

    // Veritabanında bulunamadı
    return 0;
}


// pkg.c sonu