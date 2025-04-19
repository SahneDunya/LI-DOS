// shell.h
// Lİ-DOS Minimal Kabuk (Shell) Arayuzu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Temel Komut Satiri Arayuzu saglamak.

#ifndef _SHELL_H
#define _SHELL_H

// Gerekli temel tureler
#include "types.h" // uint8_t, uint16_t, uint32_t, size_t gibi

// Gerekli diger kernel modulleri
#include "tty_io.h" // Kullanici G/Ç'si icin
#include "fs.h"    // Dosya sistemi erisimi icin (okuma ve dizin listeleme yeterli)
#include "panic.h" // Sistem kapatma/panik icin
 #include "sys.h"   // sys_shutdown icin
 #include "asm.h"   // cli, hlt icin

// Kabuk sabitleri
#define SHELL_PROMPT       "LID> " // Komut istemi stringi
#define SHELL_CMD_BUF_SIZE 128   // Komut satiri girdi buffer boyutu (satir duzenleme dahil)
#define SHELL_MAX_ARGS     16    // Bir komut satirindaki maks arguman sayisi
#define SHELL_CMD_NAME_MAX 31    // Komut adi icin maks uzunluk (null haric)
#define SHELL_ARG_MAX_LEN  63    // Bir arguman stringi icin maks uzunluk (eger argumanlar kopyalaniyorsa)
                                 // Argumanlar girdi bufferina pointer ise bu gerekli degil.
#define SHELL_CURRENT_DIR_MAX_LEN 255 // Mevcut dizin yolu stringi icin maks uzunluk (FS_MAX_PATH gibi)

// Ayrıştırılmış komut satırı yapısı
struct command_line {
    char cmd_name[SHELL_CMD_NAME_MAX + 1]; // Komut adı
    char *args[SHELL_MAX_ARGS];          // Argüman stringlerine pointerlar (cmd_buf icine isaret eder)
    int  argc;                           // Argüman sayısı
};

// Kabuğun ana döngüsü fonksiyonu.
// Bu fonksiyon, genellikle bir kernel görevi olarak başlatılır.
void shell_main(void);

#endif // _SHELL_H