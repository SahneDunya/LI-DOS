// shell.c
// Lİ-DOS Minimal Kabuk (Shell) Implementasyonu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Temel Komut Satiri Arayuzu saglamak.

#include "shell.h"
#include "tty_io.h" // TTY fonksiyonlari
#include "fs.h"    // Dosya sistemi fonksiyonlari
#include "panic.h" // panic fonksiyonu
 #include "sys.h"   // sys_shutdown icin (varsa)
#include "asm.h"   // cli, hlt icin (varsa)
// Temel string/bellek fonksiyonlari
extern int strcmp(const char *s1, const char *s2);
extern size_t strlen(const char *s);
extern void *memcpy(void *dest, const void *src, size_t n);
extern void *memset(void *s, int c, size_t n);

// --- Dahili Değişkenler ---
static char shell_cmd_buf[SHELL_CMD_BUF_SIZE]; // Komut satiri girdi bufferi
static char shell_current_dir[SHELL_CURRENT_DIR_MAX_LEN + 1]; // Mevcut çalışma dizini

// --- Dahili Fonksiyon Bildirimleri ---
static void display_prompt(void);
static int read_command(char *buffer, size_t size);
static int parse_command(char *line, struct command_line *cmd);
static int execute_command(const struct command_line *cmd);
// Dahili komut implementasyonlari
static int shell_cmd_help(const struct command_line *cmd);
static int shell_cmd_ls(const struct command_line *cmd);
static int shell_cmd_cd(const struct command_line *cmd);
static int shell_cmd_cat(const struct command_line *cmd);
static int shell_cmd_exit(const struct command_line *cmd); // Veya shutdown

// --- Kabuk Ana Döngüsü ---
void shell_main(void) {
    struct command_line cmd;

    // Kabuk baslangic ayarlari
    // Mevcut dizini kok dizin olarak ayarla
    shell_current_dir[0] = '\\';
    shell_current_dir[1] = '\0';

    tty_puts(0, "LI-DOS Shell baslatildi.\r\n"); // TTY 0'i konsol varsayalim

    // Ana kabuk döngüsü
    while (1) {
        display_prompt(); // Komut istemini göster

        // Komut satırını oku
        if (read_command(shell_cmd_buf, SHELL_CMD_BUF_SIZE) < 0) {
            tty_puts(0, "Shell Error: Komut okuma hatasi.\r\n");
            continue; // Donguye devam et
        }

        // Komut satırını ayrıştır
        if (parse_command(shell_cmd_buf, &cmd) < 0) {
            tty_puts(0, "Shell Error: Komut ayrıştırma hatasi.\r\n");
            continue; // Donguye devam et
        }

        // Komutu çalıştır (dahili veya harici - minimalde sadece dahili)
        execute_command(&cmd);
    }
}

// --- Dahili Fonksiyon Implementasyonlari ---

// Komut istemini gösterir
static void display_prompt(void) {
    tty_puts(0, shell_current_dir); // Mevcut dizini yaz
    tty_puts(0, SHELL_PROMPT);      // Komut istemini yaz
}

// TTY'den bir satir komut girdisi okur
static int read_command(char *buffer, size_t size) {
    // tty_read fonksiyonu kanonik modda satir sonu gelene kadar bloklar.
    // Okunan veriye null terminator eklemeyi unutma.
    size_t bytes_read = tty_read(0, buffer, size - 1); // TTY 0'dan oku, buffer tasmamasi icin 1 eksik oku

    if (bytes_read == 0 && tty_check_eof_or_error() == 1) { // tty_io'da boyle bir kontrol fonksiyonu olmali
        // EOF veya hata durumu
        return -1;
    }

    // Okunan verinin sonuna null terminator ekle
    buffer[bytes_read] = '\0';

    // Eger satir CRLF ile bitiyorsa, sadece LF veya sadece CR varsa, sonundaki \r veya \n'i temizle
    if (bytes_read > 0) {
         if (buffer[bytes_read-1] == '\n') {
             buffer[bytes_read-1] = '\0';
             if (bytes_read > 1 && buffer[bytes_read-2] == '\r') {
                 buffer[bytes_read-2] = '\0';
             }
         } else if (buffer[bytes_read-1] == '\r') {
             buffer[bytes_read-1] = '\0';
         }
    }

    return (int)bytes_read; // Okunan byte sayisini dondur
}

// Komut satırını komut adı ve argümanlarına ayrıştırır
static int parse_command(char *line, struct command_line *cmd) {
    char *ptr = line;
    int i;

    cmd->argc = 0;
    cmd->cmd_name[0] = '\0';

    // Başlangıçtaki boşlukları atla
    while (*ptr == ' ' || *ptr == '\t') {
        ptr++;
    }

    // Komut adını bul (ilk boşluğa veya satır sonuna kadar)
    char *cmd_start = ptr;
    char *cmd_end = ptr;
    while (*cmd_end != ' ' && *cmd_end != '\t' && *cmd_end != '\0') {
        cmd_end++;
    }

    // Komut adını kopyala (boyut kontrolü yaparak)
    size_t name_len = cmd_end - cmd_start;
    if (name_len > SHELL_CMD_NAME_MAX) {
        name_len = SHELL_CMD_NAME_MAX; // Maks boyutu aşarsa kes
    }
    memcpy(cmd->cmd_name, cmd_start, name_len);
    cmd->cmd_name[name_len] = '\0'; // Null terminator ekle

    // Argümanları bul
    ptr = cmd_end; // Komut adının sonundan başla
    while (*ptr != '\0' && cmd->argc < SHELL_MAX_ARGS) {
        // Boşlukları atla
        while (*ptr == ' ' || *ptr == '\t') {
            ptr++;
        }
        if (*ptr == '\0') break; // Argüman yoksa bitir

        // Argümanın başlangıcını kaydet
        cmd->args[cmd->argc] = ptr;

        // Argümanın sonunu bul (bir sonraki boşluğa veya satır sonuna kadar)
        char *arg_end = ptr;
        while (*arg_end != ' ' && *arg_end != '\t' && *arg_end != '\0') {
            arg_end++;
        }

        // Argümanı sonlandır (boşluğun yerine null terminator koy)
        if (*arg_end != '\0') {
            *arg_end = '\0';
            ptr = arg_end + 1; // Bir sonraki potansiyel argümana geç
        } else {
            ptr = arg_end; // Satır sonuna ulaşıldı
        }

        cmd->argc++; // Argüman sayısını artır
    }

    return 0; // Başarılı ayrıştırma
}

// Ayrıştırılmış komutu çalıştırır (dahili komutlar)
static int execute_command(const struct command_line *cmd) {
    if (cmd->cmd_name[0] == '\0') {
        // Boş komut
        return 0;
    }

    // Dahili komutları kontrol et
    if (strcmp(cmd->cmd_name, "help") == 0) {
        return shell_cmd_help(cmd);
    } else if (strcmp(cmd->cmd_name, "ls") == 0) {
        return shell_cmd_ls(cmd);
    } else if (strcmp(cmd->cmd_name, "cd") == 0) {
        return shell_cmd_cd(cmd);
    } else if (strcmp(cmd->cmd_name, "cat") == 0) {
        return shell_cmd_cat(cmd);
    } else if (strcmp(cmd->cmd_name, "exit") == 0 || strcmp(cmd->cmd_name, "shutdown") == 0) {
        return shell_cmd_exit(cmd);
    }
    // ... Diger dahili komutlar eklenebilir

    else {
        // Dahili komut değilse, harici komut arama (minimalde desteklenmez)
        tty_puts(0, "Shell Error: Komut bulunamadı: ");
        tty_puts(0, cmd->cmd_name);
        tty_puts(0, "\r\n");
        return -1; // Komut bulunamadı
    }

    return 0; // Başarılı çalışma (eğer kod buraya ulaşırsa)
}

// --- Dahili Komut Implementasyonları ---

// help komutu
static int shell_cmd_help(const struct command_line *cmd) {
    tty_puts(0, "Mevcut Komutlar:\r\n");
    tty_puts(0, "  help         - Bu yardim mesajini gosterir.\r\n");
    tty_puts(0, "  ls           - Mevcut dizindeki dosyalari listeler.\r\n");
    tty_puts(0, "  cd <dizin>   - Mevcut dizini degistirir.\r\n");
    tty_puts(0, "  cat <dosya>  - Dosya icerigini ekrana yazar.\r\n");
    tty_puts(0, "  exit         - Kabuktan cikar ve sistemi kapatir.\r\n");
    tty_puts(0, "  shutdown     - exit ile aynidir.\r\n");
    return 0;
}

// ls komutu
static int shell_cmd_ls(const struct command_line *cmd) {
    struct file_object *dir = (struct file_object *)0;
    struct fat_dir_entry entry_buffer;
    struct fat_dir_entry *entry;
    char filename[13]; // 8.3 + null + padding
    int i;

    // Mevcut dizini aç
    dir = fs_open(shell_current_dir, "r"); // "r" modu dizinleri de acabilmeli
    if (!dir) {
        tty_puts(0, "Shell Error: Mevcut dizin açılamadı!\r\n");
        return -1;
    }
    if (!(dir->attributes & FAT_ATTR_DIRECTORY)) {
        tty_puts(0, "Shell Error: Mevcut yol bir dizin degil?\r\n");
        fs_close(dir);
        return -1;
    }

    tty_puts(0, "Dizin: ");
    tty_puts(0, shell_current_dir);
    tty_puts(0, "\r\n");

    // Dizin girdilerini oku ve listele
    // fs_read_dir fonksiyonu implementasyonu, bir sonraki girdiyi okuyup dondurmeli
    // ve tum girdiler okundugunda NULL dondurmelidir.
    // Dizin objesinin icinde kendi okuma pozisyonunu tutmalidir.
    while ((entry = fs_read_dir(dir, &entry_buffer)) != (struct fat_dir_entry *)0) {
        // Gecersiz girdileri (silinmis, bos, LFN) atla
        if (entry->filename[0] == 0x00 || entry->filename[0] == 0xE5 || (entry->attribute & FAT_ATTR_LONG_NAME)) {
            continue;
        }

        // 8.3 formatındaki dosya ismini okunabilir stringe çevir
        // entry->filename ve entry->ext'ten alıp null terminate et.
        memset(filename, ' ', sizeof(filename)); // Boslukla doldur
        filename[sizeof(filename)-1] = '\0';

        memcpy(filename, entry->filename, 8);
        filename[8] = '.'; // Uzanti ayirici

        // Uzantiyi kopyala ve bosluklari temizle
        memcpy(filename + 9, entry->ext, 3);

        // Sondaki bosluklari null terminator ile degistir
        for(i = 11; i >= 0; i--) {
             if (filename[i] == ' ') filename[i] = '\0';
             else break;
        }
         // Eger uzanti bosluksa '.' ve bosluklari sil
         if (filename[8] == '.' && filename[9] == '\0') filename[8] = '\0';


        // Dizin mi dosya mi? İşaretle
        if (entry->attribute & FAT_ATTR_DIRECTORY) {
            tty_puts(0, "<DIR> ");
        } else {
             // Dosya boyutu yazdırma (uint32_t)
             // printk("%lu bytes ", entry->file_size); // Sayiyi stringe cevirme gerekir
             tty_puts(0, "      "); // Placeholder bosluklar
        }

        // Dosya ismini yazdir
        tty_puts(0, filename);
        tty_puts(0, "\r\n");
    }

    fs_close(dir); // Dizini kapat
    return 0;
}

// cd komutu
static int shell_cmd_cd(const struct command_line *cmd) {
    const char *target_dir_path;
    struct file_object *target_dir = (struct file_object *)0;
    int status = -1; // Varsayilan hata

    if (cmd->argc < 1) {
        // Argüman yoksa kök dizine git (veya mevcut dizini göster)
        target_dir_path = "\\";
    } else if (cmd->argc == 1) {
        target_dir_path = cmd->args[0]; // Hedef yol argüman olarak verildi
        // "." ve ".." gibi ozel dizinleri islemek daha karmasiktir.
        // Basitlik icin sadece mutlak veya gecerli alt dizin isimlerini kabul edelim.
        // Mutlak yol "\\" ile baslar.
    } else {
        tty_puts(0, "Shell Error: cd komutu tek arguman alir.\r\n");
        return -1;
    }

    // Hedef yolu açmayı dene (dizin olmalı)
    // fs_open mutlak yollari ("\\DIR\\SUBDIR") veya kok dizinde isimleri isleyebilmeli.
    // Relatif yollar ("SUBDIR") icin mevcut dizin yoluyla birlestirme gerekir.
    // Bu ornekte basitlik icin sadece mutlak yollari ("\\...") ve kok dizin isimlerini ("DIR") kabul edelim.
    // Eger target_dir_path "\\" ile baslamiyorsa, mevcut yolun altinda aranir.
    // Bu durumda "mevcut_yol" + "\\" + "target_dir_path" stringini olusturmak gerekir.

    char full_target_path[SHELL_CURRENT_DIR_MAX_LEN + SHELL_CMD_BUF_SIZE]; // Yeterince buyuk bir buffer
    full_target_path[0] = '\0';

    if (target_dir_path[0] == '\\') {
         // Mutlak yol
         // Yol uzunlugu kontrolu yapilmalı
         if (strlen(target_dir_path) > SHELL_CURRENT_DIR_MAX_LEN) {
              tty_puts(0, "Shell Error: Yol cok uzun.\r\n");
              return -1;
         }
         memcpy(full_target_path, target_dir_path, strlen(target_dir_path) + 1);

    } else {
         // Relatif yol - mevcut dizinle birlestir
         // Mevcut yolun uzunlugu + 1 ('\\') + hedef yolun uzunlugu + 1 (null) < buffer boyutu olmali.
         size_t current_len = strlen(shell_current_dir);
         size_t target_len = strlen(target_dir_path);

         if (current_len + 1 + target_len + 1 > SHELL_CURRENT_DIR_MAX_LEN) {
             tty_puts(0, "Shell Error: Ortaya çıkan yol cok uzun.\r\n");
             return -1;
         }

         memcpy(full_target_path, shell_current_dir, current_len);
         // Kok dizin "\\" ise ekstra "\\" ekleme
         if (current_len > 1 || shell_current_dir[0] != '\\') {
              full_target_path[current_len++] = '\\'; // Araya separator ekle
         }

         memcpy(full_target_path + current_len, target_dir_path, target_len + 1);
    }

    // Hedef yolu dizin olarak açmayı dene
    target_dir = fs_open(full_target_path, "r");
    if (!target_dir) {
         tty_puts(0, "Shell Error: Dizin bulunamadi veya acilamadi: ");
         tty_puts(0, full_target_path);
         tty_puts(0, "\r\n");
         return -1;
    }

    // Açılan nesne bir dizin mi?
    if (!(target_dir->attributes & FAT_ATTR_DIRECTORY)) {
        tty_puts(0, "Shell Error: Yol bir dizin degil: ");
        tty_puts(0, full_target_path);
        tty_puts(0, "\r\n");
        fs_close(target_dir);
        return -1;
    }

    // Başarılı! Mevcut dizini güncelle
    memcpy(shell_current_dir, full_target_path, strlen(full_target_path) + 1);
    fs_close(target_dir); // Dizin nesnesini kapat (açık tutmaya gerek yok)
     printk("Shell: Mevcut dizin '%s' olarak degistirildi.\r\n", shell_current_dir);
    return 0;
}

// cat komutu
static int shell_cmd_cat(const struct command_line *cmd) {
    const char *filename;
    struct file_object *file = (struct file_object *)0;
    char read_buffer[SECTOR_SIZE]; // Dosya okuma bufferi
    size_t bytes_read;

    if (cmd->argc < 1) {
        tty_puts(0, "Shell Error: cat komutu dosya adi gerektirir.\r\n");
        return -1;
    } else if (cmd->argc == 1) {
        filename = cmd->args[0]; // Dosya adi argüman olarak verildi
        // Relatif yol handling (cd komutundaki gibi) burada da gereklidir.
        // Basitlik icin sadece kok dizindeki dosyalari cat yapabilsin veya mutlak yol ("\\FILE.TXT").
        // Tam yol olusturma mantigi cd ile aynidir.
        // Bu ornekte sadece argumanı dosya adi kabul edelim, fs_open relatif pathleri handle etmeli veya mutlak yol olusturulmali.
        // cd komutundaki tam yol olusturma mantigi buraya kopyalanmalidir.
        // Simdilik sadece arguman stringini fs_open'a gonderelim. fs_open'in relativ/mutlak pathleri isledigi varsayilsin.
         char full_file_path[SHELL_CURRENT_DIR_MAX_LEN + SHELL_CMD_BUF_SIZE];
         full_file_path[0] = '\0';
         // cd komutundaki tam yol olusturma mantigini buraya kopyala
         size_t current_len = strlen(shell_current_dir);
         size_t target_len = strlen(filename);

         if (target_dir_path[0] == '\\') { // Yanlis degisken ismi (target_dir_path), filename olmali
             if (target_len > SHELL_CURRENT_DIR_MAX_LEN) {
                 tty_puts(0, "Shell Error: Yol cok uzun.\r\n"); return -1;
             }
             memcpy(full_file_path, filename, target_len + 1);
         } else {
             if (current_len + 1 + target_len + 1 > SHELL_CURRENT_DIR_MAX_LEN) {
                 tty_puts(0, "Shell Error: Ortaya çıkan yol cok uzun.\r\n"); return -1;
             }
             memcpy(full_file_path, shell_current_dir, current_len);
              if (current_len > 1 || shell_current_dir[0] != '\\') {
                 full_file_path[current_len++] = '\\';
             }
             memcpy(full_file_path + current_len, filename, target_len + 1);
         }
         filename = full_file_path; // Artik filename tam yolu gosteriyor.


    } else {
        tty_puts(0, "Shell Error: cat komutu tek dosya adi alir.\r\n");
        return -1;
    }


    // Dosyayi okuma modunda ac
    file = fs_open(filename, "r");
    if (!file) {
        tty_puts(0, "Shell Error: Dosya acilamadi: ");
        tty_puts(0, filename);
        tty_puts(0, "\r\n");
        return -1;
    }

    // Acilan nesne bir dosya mi?
    if (file->attributes & FAT_ATTR_DIRECTORY) {
        tty_puts(0, "Shell Error: Yol bir dosya degil: ");
        tty_puts(0, filename);
        tty_puts(0, "\r\n");
        fs_close(file);
        return -1;
    }

    // Dosya icerigini oku ve ekrana yaz
    while ((bytes_read = fs_read(file, read_buffer, sizeof(read_buffer))) > 0) {
        // Okunan bufferi ekrana yaz (tty_write kullan)
        tty_write(0, read_buffer, bytes_read); // TTY 0'a yaz
    }

    fs_close(file); // Dosyayi kapat
     printk("\r\nShell: Dosya '%s' yazdirildi.\r\n", filename); // Sonunda yeni satir ekle
    tty_puts(0, "\r\n"); // Okunan dosya sonunda yeni satir

    return 0;
}

// exit/shutdown komutu
static int shell_cmd_exit(const struct command_line *cmd) {
    tty_puts(0, "Shellden cikiliyor. Sistem kapatiliyor...\r\n");
    // Sistemi kapat (sys_shutdown varsa onu, yoksa panic veya direkt hlt)
     sys_shutdown(); // Kontrollu kapatma
    cli(); // Kesmeleri kapat
    hlt(); // CPU'yu durdur
     panic("Shell exited."); // Panik ile kapatma
    return 0; // Buraya erisilmemeli
}


// shell.c sonu