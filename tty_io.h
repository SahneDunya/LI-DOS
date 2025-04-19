// tty_io.h
// Lİ-DOS TTY (Teletype) Modulu Arayuzu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Kullanici giris/cikisini tamponlama ve isleme (satir duzenleme, yankilama).

#ifndef _TTY_IO_H
#define _TTY_IO_H

// Gerekli temel tureler
// Kernelinizde baska bir baslik dosyasinda (types.h gibi) olabilir.
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef uint16_t size_t; // 16-bit ortamda size_t genellikle uint16_t olabilir.

// Maksimum TTY cihazi sayisi
#define MAX_TTYS 4 // Ornek: 1 konsol + 3 seri port

// TTY Input Buffer Boyutu (Satir düzenleme ve tamponlama için)
#define TTY_BUF_SIZE 128 // Ornek: Maksimum satir uzunlugu

// TTY Mod Bayraklari
#define TTY_MODE_CANONICAL  0x01 // Canonical mode (giris satir satir islenir)
#define TTY_MODE_RAW        0x02 // Raw mode (giris karakter karakter hemen verilir)
#define TTY_MODE_ECHO       0x04 // Giris karakterlerini yankila (ekrana yaz)

// TTY cihaz yapısı
// Her TTY (konsol, seri port vb.) icin bir tane olacaktir.
struct tty {
    uint8_t state; // TTY durumu (kullanımda, boş, vb.)
    uint8_t mode;  // TTY modu (kanonik, raw, yankilama)

    // Giris tamponu (dairesel buffer veya basit buffer)
    char in_buffer[TTY_BUF_SIZE];
    size_t in_buf_head; // Tamponun başı (okunacak ilk karakter)
    size_t in_buf_tail; // Tamponun sonu (yeni karakterlerin yazilacagi yer)
    size_t in_buf_count; // Tampondaki karakter sayisi

    // Düşük seviye aygıt sürücüsü fonksiyonlarına pointerlar
    // TTY bu fonksiyonlari kullanarak gercek donanim G/Ç yapar.
    void (*putc_dev)(char c); // Aygıta tek karakter yazma fonksiyonu
    int (*has_data_dev)(void); // Aygıtta okunacak veri olup olmadığını kontrol fonksiyonu
    char (*getc_dev)(void); // Aygıttan tek karakter okuma fonksiyonu (bloklayici veya degil)

    // O an bu TTY'den okuma bekleyen goreve bir isaret (opsiyonel, multitasking ile ilgili)
    // struct task *waiting_task; // sched modülünden struct task pointeri
};

// TTY durumları (örnek)
#define TTY_STATE_UNUSED 0
#define TTY_STATE_INITED 1

// TTY modülünü baslatir
void tty_init_module(void);

// Belirtilen indexteki TTY cihazini baslatir ve dusuk seviye aygit fonksiyonlarina baglar.
// id: TTY cihazi indexi (0..MAX_TTYS-1)
// putc_func: Bu TTY icin kullanilacak putc fonksiyonu (ornegin console_putc veya serial_putc'nin sarmalayicisi)
// has_data_func: Bu TTY icin kullanilacak has_data fonksiyonu (ornegin serial_has_data)
// getc_func: Bu TTY icin kullanilacak getc fonksiyonu (ornegin serial_getc)
// Donus degeri: 0 basari, -1 hata.
int tty_init_device(int id, void (*putc_func)(char c), int (*has_data_func)(void), char (*getc_func)(void));

// TTY cihazindan tek karakter okur (modlara gore isleme yapar, bloklayici olabilir).
// id: Okunacak TTY cihazi indexi.
// Donus degeri: Okunan karakter (veya hata/veri yok ise ozel bir deger).
char tty_getc(int id);

// TTY cihazina tek karakter yazar (alt aygitin putc fonksiyonunu cagirir).
// id: Yazilacak TTY cihazi indexi.
// c: Yazilacak karakter.
void tty_putc(int id, char c);

// TTY cihazina null terminated string yazar.
// id: Yazilacak TTY cihazi indexi.
// s: Yazilacak string.
void tty_puts(int id, const char *s);

// TTY cihazindan belirtilen buffer'a 'count' kadar veri okur (modlara gore isleme yapar, bloklayici olabilir).
// id: Okunacak TTY cihazi indexi.
// buf: Verinin yazilacagi buffer.
// count: Maksimum okunacak byte sayisi.
// Donus degeri: Gercekten okunan byte sayisi (0 basari).
size_t tty_read(int id, char *buf, size_t count);

// TTY cihazina belirtilen buffer'dan 'count' kadar veri yazar.
// id: Yazilacak TTY cihazi indexi.
// buf: Yazilacak veriyi iceren buffer.
// count: Yazilacak byte sayisi.
void tty_write(int id, const char *buf, size_t count);

// Dusuk seviye aygit sürücüsü (klavye veya seri port kesme isleyicisi gibi) tarafindan cagirilir.
// Aygittan gelen karakteri TTY'ye iletir ve tamponlama/isleme yapar.
// id: Karakterin geldigi TTY cihazi indexi.
// c: Gelen karakter.
void tty_input(int id, char c);

// TTY cihazinin modunu ayarlar (kanonik, raw, yankilama).
// id: Modu ayarlanacak TTY cihazi indexi.
// mode_flags: Ayarlanacak mod bayraklari (TTY_MODE_x sabitlerinin bitwise OR'u).
void tty_set_mode(int id, uint8_t mode_flags);


#endif // _TTY_IO_H