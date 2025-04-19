// printk.h
// Lİ-DOS Kernel Cikti Modulu Arayuzu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Kernel icinden formatli cikti almak.

#ifndef _PRINTK_H
#define _PRINTK_H

// Variadic argumanlar icin gerekli tipler ve makrolar.
// Bu basligin (ornegin va_list.h) kernelinizde tanimli oldugunu varsayiyoruz.
// Ornek:
 typedef char* va_list;
 #define VA_START(list, arg) list = (char*)&arg + sizeof(arg)
 #define VA_ARG(list, type) (list += sizeof(type), *(type*)(list - sizeof(type)))
 #define VA_END(list) list = (char*)0
// Gercek implementasyon mimariye ve derleyiciye gore degisir.
#include "va_list.h" // Kernelin custom va_list tanimlarini iceren baslik

// printk fonksiyonu: Kernel icinden formatli cikti almak icin kullanilir.
// printf benzeri format stringi ve degisken sayida arguman alir.
void printk(const char *fmt, ...);

// vprintk fonksiyonu: printk'in va_list alan versiyonu.
// printk ve panic gibi fonksiyonlar bu versiyonu cagirabilir.
void vprintk(const char *fmt, va_list args);

#endif // _PRINTK_H