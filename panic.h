// panic.h
// Lİ-DOS Kernel Panik Modulu Arayuzu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Kurtarilamaz hatalarda sistemi durdurmak ve bilgi vermek.

#ifndef _PANIC_H
#define _PANIC_H

// panic fonksiyonu kurtarilamaz bir hata durumunda cagirilir.
// Hata mesajini formatlayarak cikti alir ve sistemi durdurur.
// Bu fonksiyon asla geri donmez.
// printf benzeri format stringi ve degisken sayida arguman alir.
void panic(const char *fmt, ...);

#endif // _PANIC_H