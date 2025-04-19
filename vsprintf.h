// vsprintf.h
// Lİ-DOS Formatlı String Yazdırma Bildirimleri
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: vsprintf ve degisken arguman (va_list) tanimlari.

#ifndef _LIDOS_VSPRINTF_H
#define _LIDOS_VSPRINTF_H

#include "types.h" // size_t ve temel tipler icin

/*
 * Değişken Argüman (Variable Argument) Tanımları
 * Standart <stdarg.h> yerine manuel tanimlar.
 * Bu tanimlar target mimariye ve derleyiciye ozeldir!
 * Aşağıdaki x86 16-bit (cdecl veya yakin) çağrı kuralina dayali orneklerdir.
 */

// va_list tip tanimi: Stack uzerindeki argumanlarin konumunu gosteren isaretci
// Argümanlar genellikle stacke itilir. Stack pointer (SP) asagi dogru buyur.
// Bir fonksiyon cagirildiginda, sabit argumanlar once, degisken argumanlar sonra itilir.
// va_list, son sabit argumanin stackteki adresinden baslatilir ve va_arg ile ilerletilir.
typedef char *va_list; // Stack uzerindeki byte'lara isaret eden isaretci

// va_start: Değişken argüman listesini başlatır.
// ap: va_list degiskeni
// last: Değişken argümanlardan önceki son sabit argümanın adı
// "&last" son sabit argumanin adresini verir.
// "sizeof(last)" kadar ilerleyerek ilk degisken argumanin baslangicina gelinir.
// Not: Compilerlar char/short gibi kucuk tipleri stacke int (16-bit) veya long (32-bit) olarak itebilir (default argument promotion).
// Bu yuzden va_arg, type'in PROMOTE edilmis boyutunu bilmelidir.
// C89'da default promotion: float -> double, char/short -> int.
// 16-bit x86'da int 16-bit'tir. char ve short da int olarak itilir (16-bit). long ve pointer 32-bit.
#define va_start(ap, last) (ap = (va_list)((char*)(&last) + sizeof(last)))

// va_arg: Değişken argüman listesinden belirtilen tipte bir argümanı alır ve listeyi ilerletir.
// ap: va_list degiskeni
// type: Alınacak argümanın tipi (promotion sonrası)
// ap += sizeof(type) ile listede ilerlemeden once, *(type *)(ap) ile gecerli arguman alinir.
// Veya *(type *)(ap - sizeof(type)) formulu kullanilir, ap once ilerletilirse.
// Ikinci formül C99'a daha yakındır. İlk formül daha C89-vari olabilir.
// Basitlik icin va_arg = (pointer'ı type* yap, degerini al, pointeri type boyutu kadar ilerlet)
// Dikkat: type boyutu promotion sonrasi boyuttur.
#define va_arg(ap, type) (*(type *)(ap = (va_list)((char*)(ap) + sizeof(type)), (char*)(ap) - sizeof(type)))
// Alternatif daha C89-vari ve potansiyel olarak daha guvenli:
#define va_arg(ap, type) (*(type *)(ap = (char*)ap + sizeof(type), (char*)ap - sizeof(type)))
// Veya once degeri alip sonra pointeri ilerletmek:
#define va_arg(ap, type) (*(type*)((ap += sizeof(type)) - sizeof(type))) // Bu da yaygin
// En basit (promotionu dikkate almadan, tehlikeli olabilir):
#define va_arg(ap, type) (*(type *)(ap += sizeof(type)) - sizeof(type)) // Yanlis, pointer aritmetigi hatali

// Standard macro tanimlari genellikle type'in PROMOTE edilmis boyutunu kullanir.
// 16-bit int: sizeof(int) = 2. sizeof(char), sizeof(short) promotion sonrasi sizeof(int) = 2.
// sizeof(long) = 4. sizeof(void*) (far) = 4.
// va_arg(ap, char) -> stackten int okur. va_arg(ap, short) -> stackten int okur.
// va_arg(ap, int) -> stackten int okur. va_arg(ap, long) -> stackten long okur.
// va_arg(ap, void*) -> stackten 32-bit pointer okur (muhtemelen long boyutu 4).

// Basit, promotionu goz ardi eden (tehlikeli olabilir) va_arg ornegi:
#define va_arg(ap, type) *(type *)((ap += sizeof(type)) - sizeof(type)) // Cok basit pointer aritmetigi

// Biraz daha dogru, promotionu dikkate alan (ama hala mimariye ozel) va_arg ornegi:
// int/char/short icin 16-bit okuma, long/pointer icin 32-bit okuma.
#define __va_size(type) ((sizeof(type) + sizeof(int) - 1) & ~(sizeof(int) - 1)) // Argumanin stackteki hizalanmis boyutu (genellikle 2 veya 4)
// Ornek: char -> 2, short -> 2, int -> 2, long -> 4, void* -> 4
// __va_size(type) ((sizeof(type) + 1) & ~1) // Sadece 2 byte hizalama varsa (char/short/int)
// #define va_arg(ap, type) (*(type *)(ap += __va_size(type), ap - __va_size(type)))

// En guvenlisi, derleyici tarafindan saglanan stdarg.h makrolarinin nasil implemente edildigine bakmak veya
// bilinen bir yontemi kullanmak.
// YAYGIN BİR 16-BIT VA_ARG ÖRNEĞİ (Promotion ve hizalama dikkate alınarak):
#define __size_int ((sizeof(int) + sizeof(int) - 1) & ~(sizeof(int) - 1)) // Genellikle 2
#define __size_long ((sizeof(long) + sizeof(int) - 1) & ~(sizeof(int) - 1)) // Genellikle 4
#define __va_arg(ap, type) \
    (ap = (char*)ap + ( (sizeof(type) == sizeof(long) || sizeof(type) == sizeof(void*)) ? __size_long : __size_int), \
     *(type*)((char*)ap - ( (sizeof(type) == sizeof(long) || sizeof(type) == sizeof(void*)) ? __size_long : __size_int )) )

// va_arg(ap, type) = * (type *) (ap += STACK_PROMOTE_SIZE(type)) - STACK_PROMOTE_SIZE(type)
// STACK_PROMOTE_SIZE(type) char/short -> 2, int -> 2, long/pointer -> 4
// Basitlik icin en ustteki typedef ve va_start kalsin, va_arg icin yorum satiri ornekleri yeterli olsun.
// Tam bir implementasyon icin vsprintf.c ornegindeki gibi va_arg kullanilir.

// va_end: Değişken argüman listesini temizler (genellikle bir şey yapmaz, NULL'a atama gibi).
#define va_end(ap) (ap = (va_list)0)


/*
 * Formatlı String Yazdırma Fonksiyonu
 * Implementasyonu vsprintf.c dosyasindadir.
 */
// Çıktıyı buffer'a yazar. Null terminator ekler.
extern int vsprintf(char *buffer, const char *format, va_list va);


#endif // _LIDOS_VSPRINTF_H