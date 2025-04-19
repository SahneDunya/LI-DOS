// tty_io.c
// Lİ-DOS TTY (Teletype) Modulu Implementasyonu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89

#include "tty_io.h" // TTY arayuzu ve yapilari
#include "console.h" // console_putc gibi temel cikti fonksiyonlari (eger konsol TTY ise)
// #include "serial.h"  // serial_putc, serial_getc gibi seri port fonksiyonlari (eger serial TTY ise)
// #include "sched.h"   // Zamanlayici (okuma bekleyen gorevleri uyandirmak icin)
// #include "printk.h"  // Debug cikti icin

// TTY cihaz ornekleri dizisi
static struct tty ttys[MAX_TTYS];

// TTY modülünü baslatir (TTY dizisini temizler)
void tty_init_module(void) {
    int i;
    for (i = 0; i < MAX_TTYS; i++) {
        ttys[i].state = TTY_STATE_UNUSED;
        // Diger alanlari da sifirlamak iyi olur.
        ttys[i].mode = 0;
        ttys[i].in_buf_head = 0;
        ttys[i].in_buf_tail = 0;
        ttys[i].in_buf_count = 0;
        ttys[i].putc_dev = (void (*)(char))0;
        ttys[i].has_data_dev = (int (*)(void))0;
        ttys[i].getc_dev = (char (*)(void))0;
    }
    // printk("TTY module initialized.\n");
}

// Belirtilen indexteki TTY cihazini baslatir ve dusuk seviye aygit fonksiyonlarina baglar.
int tty_init_device(int id, void (*putc_func)(char c), int (*has_data_func)(void), char (*getc_func)(void)) {
    if (id < 0 || id >= MAX_TTYS) {
        // printk("TTY init error: Invalid device ID %d\n", id);
        return -1; // Gecersiz ID
    }
    if (ttys[id].state != TTY_STATE_UNUSED) {
         // printk("TTY init warning: Device %d already in use.\n", id);
         // return -1; // Zaten kullaniliyorsa hata
    }
    if (!putc_func || !has_data_func || !getc_func) {
         // printk("TTY init error: Null device functions for ID %d\n", id);
         return -1; // Gecersiz fonksiyon pointerlari
    }


    // TTY cihazini baslat
    ttys[id].state = TTY_STATE_INITED;
    ttys[id].mode = TTY_MODE_CANONICAL | TTY_MODE_ECHO; // Varsayilan mod: Kanonik + Yankilama
    ttys[id].in_buf_head = 0;
    ttys[id].in_buf_tail = 0;
    ttys[id].in_buf_count = 0;

    // Dusuk seviye aygit fonksiyonlarini kaydet
    ttys[id].putc_dev = putc_func;
    ttys[id].has_data_dev = has_data_func;
    ttys[id].getc_dev = getc_func;

    // printk("TTY device %d initialized.\n", id);
    return 0;
}


// TTY cihazindan tek karakter okur (modlara gore isleme yapar, bloklayici olabilir).
char tty_getc(int id) {
    if (id < 0 || id >= MAX_TTYS || ttys[id].state != TTY_STATE_INITED) {
        // printk("TTY getc error: Invalid device ID %d\n", id);
        return '\0'; // Gecersiz TTY
    }

    struct tty *tty = &ttys[id];

    // Kanonik modda ise satir sonu (\n) gelene kadar bekle
    if (tty->mode & TTY_MODE_CANONICAL) {
        // Tamponda bir satir olusana kadar bekle (en az bir \n karakteri)
        // Bu bekleyis implementasyonu multitasking modeline baglidir.
        // Kooperatifte: Tamponu kontrol et, yoksa schedule() cagir veya polling yap.
        // On alicida: Gorevi blokla ve scheduler'a birak.
        while (tty->in_buf_count == 0 || (tty->in_buffer[tty->in_buf_head] != '\n')) {
             // Tamponda veri yoksa veya satir sonu yoksa, aygittan veri oku (polling veya bekle)
             // Bu ornekte basit polling ve schedule cagiriyoruz.
             if (tty->has_data_dev && tty->has_data_dev()) {
                 char c = tty->getc_dev(); // Aygittan karakter oku (bloklayici olabilir)
                 tty_input(id, c); // Gelen karakteri TTY isleme fonksiyonuna gonder
             } else {
                 // Aygitta veri yoksa, baska gorevlerin calismasi icin kontrolu birak.
                 // schedule(); // Eğer zamanlayıcı varsa çağır.
                 // Veya sadece kisa bir bekleme yap.
                 // io_delay(); // asm.S'ten
             }
        }
        // Tamponda satir var ve okunacak ilk karakter \n degilse (bu olmamalı, \n en son islenir)
        // Asil kanonik modda, tampondan \n'e kadar olan karakterleri verip \n dahil atmak gerekir.
        // Basit implementasyon: Tamponda en az 1 karakterin (\n dahil) oldugunu varsayip ilkini alalim.
    } else { // Raw modda ise herhangi bir karakteri bekle
         while (tty->in_buf_count == 0) {
              if (tty->has_data_dev && tty->has_data_dev()) {
                  char c = tty->getc_dev(); // Aygittan karakter oku
                  tty_input(id, c); // Gelen karakteri isleme
              } else {
                  // schedule(); // Eğer zamanlayıcı varsa çağır
                  // io_delay();
              }
         }
    }

    // Tampondan karakteri al
    char c = tty->in_buffer[tty->in_buf_head];
    tty->in_buf_head = (tty->in_buf_head + 1) % TTY_BUF_SIZE; // Head pointeri ilerlet (dairesel)
    tty->in_buf_count--; // Tampondaki karakter sayisini azalt

    // Kanonik modda, bir satir okunduktan sonra tampondaki karakterleri temizlemek gerekebilir (veya sadece \n'e kadar olan kismi).
    // Basitlik icin, her getc \n'i gorunce bir sonraki satir icin yeniden baslamali mantigi kullanilabilir.
    // Veya kanonik okuma fonksiyonu (tty_read) tüm satiri cekmelidir.
    // Bu tty_getc, kanonik modda kullanilacaksa, satir hazir oldugunda tek tek karakterleri verir.

    return c;
}

// TTY cihazina tek karakter yazar.
void tty_putc(int id, char c) {
    if (id < 0 || id >= MAX_TTYS || ttys[id].state != TTY_STATE_INITED || !ttys[id].putc_dev) {
        // printk("TTY putc error: Invalid device or function for ID %d\n", id);
        return; // Gecersiz TTY veya fonksiyon
    }
    ttys[id].putc_dev(c); // Dusuk seviye aygit fonksiyonunu cagir
}

// TTY cihazina null terminated string yazar.
void tty_puts(int id, const char *s) {
    if (id < 0 || id >= MAX_TTYS || ttys[id].state != TTY_STATE_INITED) return; // Gecersiz TTY
    while (*s != '\0') {
        tty_putc(id, *s);
        s++;
    }
}

// TTY cihazindan belirtilen buffer'a 'count' kadar veri okur (modlara gore isleme yapar, bloklayici olabilir).
size_t tty_read(int id, char *buf, size_t count) {
    if (id < 0 || id >= MAX_TTYS || ttys[id].state != TTY_STATE_INITED || !buf || count == 0) {
        // printk("TTY read error: Invalid parameters for ID %d\n", id);
        return 0; // Gecersiz parametreler
    }

    struct tty *tty = &ttys[id];
    size_t bytes_read = 0;

    // Kanonik modda: Satir sonu (\n) gelene kadar bekle ve satir sonuna kadar oku
    if (tty->mode & TTY_MODE_CANONICAL) {
         // Tamponda bir satir olusana kadar bekle
         while (tty->in_buf_count == 0 || (tty->in_buffer[(tty->in_buf_head + tty->in_buf_count -1) % TTY_BUF_SIZE] != '\n')) {
              // Tamponda satir sonu yoksa, aygittan karakterleri oku ve tty_input'a gonder.
              // Bu bekleme mekanizmasi tty_getc icindeki ile benzer olabilir.
               if (tty->has_data_dev && tty->has_data_dev()) {
                  char c = tty->getc_dev(); // Aygittan karakter oku
                  tty_input(id, c); // Gelen karakteri TTY isleme fonksiyonuna gonder
               } else {
                  // schedule(); // Eğer zamanlayıcı varsa çağır
               }
         }

         // Tamponda satir var. Tampondan oku, \n dahil, buffer boyutunu (count) asmayacak sekilde.
         while (bytes_read < count && tty->in_buf_count > 0) {
              char c = tty->in_buffer[tty->in_buf_head];
              tty->in_buffer[tty->in_buf_head] = '\0'; // Okunan yeri temizle (opsiyonel)
              tty->in_buf_head = (tty->in_buf_head + 1) % TTY_BUF_SIZE;
              tty->in_buf_count--;

              buf[bytes_read++] = c;

              if (c == '\n') {
                  break; // Satir sonuna ulastik, okumayi bitir
              }
         }
         // Okunan karakterleri tampondan cikarirken, aslinda sadece head pointerini ilerletmek dairesel tampon icin yeterlidir.
         // Okunan karakterleri tampondan atmak (head'i ilerletmek ve count'u azaltmak) gerekiyor.


    } else { // Raw modda: Tampondaki ilk 'count' veya daha az karakteri oku (bekleyebilir)
        // Tamponda karakter olana kadar bekle
         while (tty->in_buf_count == 0) {
              if (tty->has_data_dev && tty->has_data_dev()) {
                  char c = tty->getc_dev(); // Aygittan karakter oku
                  tty_input(id, c); // Gelen karakteri isleme
               } else {
                   // schedule(); // Eğer zamanlayıcı varsa çağır
               }
         }

        // Tampondan oku, 'count' veya tampon boyutu kadar (hangisi kucukse)
        while (bytes_read < count && tty->in_buf_count > 0) {
            char c = tty->in_buffer[tty->in_buf_head];
            tty->in_buffer[tty->in_buf_head] = '\0'; // Okunan yeri temizle (opsiyonel)
            tty->in_buf_head = (tty->in_buf_head + 1) % TTY_BUF_SIZE;
            tty->in_buf_count--;

            buf[bytes_read++] = c;
        }
    }

    // printk("TTY read %d bytes from device %d\n", bytes_read, id);
    return bytes_read; // Okunan byte sayisini dondur
}

// TTY cihazina belirtilen buffer'dan 'count' kadar veri yazar.
void tty_write(int id, const char *buf, size_t count) {
    if (id < 0 || id >= MAX_TTYS || ttys[id].state != TTY_STATE_INITED || !buf || count == 0) {
        // printk("TTY write error: Invalid parameters for ID %d\n", id);
        return; // Gecersiz parametreler
    }

    size_t i;
    for (i = 0; i < count; i++) {
        tty_putc(id, buf[i]); // Her karakteri yaz
    }
    // printk("TTY wrote %d bytes to device %d\n", count, id);
}

// Dusuk seviye aygit sürücüsü (klavye veya seri port kesme isleyicisi gibi) tarafindan cagirilir.
// Aygittan gelen karakteri TTY'ye iletir ve tamponlama/isleme yapar.
void tty_input(int id, char c) {
    if (id < 0 || id >= MAX_TTYS || ttys[id].state != TTY_STATE_INITED) {
        // printk("TTY input error: Invalid device ID %d\n", id);
        return; // Gecersiz TTY
    }

    struct tty *tty = &ttys[id];

    // Tampon doluysa karakteri at (veya hata ver)
    if (tty->in_buf_count >= TTY_BUF_SIZE) {
        // printk("TTY input warning: Buffer full for device %d\n", id);
        return; // Tampon dolu
    }

    // Yankilama (Echo)
    if (tty->mode & TTY_MODE_ECHO) {
        // Ozel karakterleri yankilarken dikkatli olun. \n, \r, \b vb.
        if (c == '\r' || c == '\n') {
             // Enter'a basıldığında CRLF yankıla ve yeni satıra geç
             tty_putc(id, '\r');
             tty_putc(id, '\n');
        } else if (c == '\b') {
             // Backspace yankılarken karakteri silmek için \b, boşluk, \b yaz
             tty_putc(id, '\b');
             tty_putc(id, ' ');
             tty_putc(id, '\b');
        } else {
             // Diger karakterleri normal yankila
             tty_putc(id, c);
        }
    }

    // Gelen karakteri tampona yaz (dairesel buffer)
    // Ozel karakterleri tampona yazmadan islemek gerekebilir (örn. Backspace tampondan siler).
    if (c == '\b') {
        // Tampon bos degilse son karakteri sil (mantiksal olarak)
        if (tty->in_buf_count > 0) {
             // Tail pointeri geri al (dairesel)
             tty->in_buf_tail = (tty->in_buf_tail == 0) ? (TTY_BUF_SIZE - 1) : (tty->in_buf_tail - 1);
             tty->in_buf_count--;
        }
    } else {
         // Diger karakterleri tampona ekle
         tty->in_buffer[tty->in_buf_tail] = c;
         tty->in_buf_tail = (tty->in_buf_tail + 1) % TTY_BUF_SIZE; // Tail pointeri ilerlet (dairesel)
         tty->in_buf_count++; // Tampondaki karakter sayisini artir
    }


    // Kanonik modda satir sonu gelirse (Enter veya \n), okuma bekleyen gorevi uyandir.
    if ((tty->mode & TTY_MODE_CANONICAL) && (c == '\n' || c == '\r')) {
        // Eğer bir görev bu TTY'den okuma bekliyorsa ve engellenmişse, onu hazır duruma getir.
        // if (tty->waiting_task && tty->waiting_task->state == TASK_STATE_BLOCKED) {
        //     tty->waiting_task->state = TASK_STATE_READY;
        //     // schedule(); // Gerekirse zamanlayıcıyı tetikle
        // }
    }

    // printk("TTY input char '%c' (0x%x) to device %d, buf_count: %d\n", c, c, id, tty->in_buf_count);
}

// TTY cihazinin modunu ayarlar (kanonik, raw, yankilama).
void tty_set_mode(int id, uint8_t mode_flags) {
    if (id < 0 || id >= MAX_TTYS || ttys[id].state != TTY_STATE_INITED) {
        // printk("TTY set_mode error: Invalid device ID %d\n", id);
        return; // Gecersiz TTY
    }
    ttys[id].mode = mode_flags;
    // printk("TTY device %d mode set to 0x%x\n", id, mode_flags);
}

// tty_io.c sonu