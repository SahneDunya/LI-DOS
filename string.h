// string.h
// Lİ-DOS String ve Bellek Fonksiyon Bildirimleri
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: string.c dosyasindaki fonksiyonlarin bildirimleri.

#ifndef _LIDOS_STRING_H
#define _LIDOS_STRING_H

#include "types.h" // size_t tanimi icin

/*
 * String ve Bellek Manipülasyon Fonksiyonları
 * Implementasyonlari string.c dosyasindadir.
 */

// Null terminated stringin uzunlugunu bulur.
extern size_t strlen(const char *s);

// Kaynak stringi hedef stringe kopyalar. Hedef yeterince buyuk olmalidir.
extern char *strcpy(char *dest, const char *src);

// Iki stringi karsilastirir.
// s1 == s2 ise 0, s1 < s2 ise negatif, s1 > s2 ise pozitif bir deger dondurur.
extern int strcmp(const char *s1, const char *s2);

// n byte'i kaynaktan hedefe kopyalar. Bolgeler cakismamalidir.
extern void *memcpy(void *dest, const void *src, size_t n);

// n byte'i belirtilen degerle doldurur.
extern void *memset(void *s, int c, size_t n);

// n byte'i kaynaktan hedefe kopyalar. Bolgeler cakissa bile dogru calisir.
extern void *memmove(void *dest, const void *src, size_t n);

#endif // _LIDOS_STRING_H