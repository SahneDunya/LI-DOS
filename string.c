// string.c
// Lİ-DOS String ve Bellek Yardımcı Fonksiyonları
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Standart string.h ve memory.h fonksiyonlarının minimal implementasyonları.
// Bu dosya, bu fonksiyonları kullanacak diğer modüller tarafından include edilmez,
// derlenerek kernela baglanir.

#include "types.h" // Temel türler (size_t vb.)
#include "string.h" // Kendi bildirim basligi (Bu dosyadaki fonksiyonlari bildirir)

//string.h'in icinde soyle bir bildirim olmali (ornek):
 #ifndef _STRING_H
 #define _STRING_H
 #include "types.h" // size_t icin
 extern size_t strlen(const char *s);
 extern char *strcpy(char *dest, const char *src);
 extern int strcmp(const char *s1, const char *s2);
 extern void *memcpy(void *dest, const void *src, size_t n);
 extern void *memset(void *s, int c, size_t n);
 extern void *memmove(void *dest, const void *src, size_t n);
 #endif // _STRING_H


// String uzunluğunu bulur (null terminator hariç)
size_t strlen(const char *s) {
    size_t len = 0;
    if (!s) return 0;
    while (*s != '\0') {
        len++;
        s++;
    }
    return len;
}

// Kaynak stringi hedef stringe kopyalar. (Hedef yeterince büyük olmalı!)
char *strcpy(char *dest, const char *src) {
    char *original_dest = dest;
    if (!dest || !src) return dest;
    while (*src != '\0') {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0'; // Null terminatoru kopyala
    return original_dest;
}

// Stringleri karşılaştırır.
// s1 == s2 ise 0, s1 < s2 ise negatif, s1 > s2 ise pozitif bir değer döner.
int strcmp(const char *s1, const char *s2) {
    if (!s1 && !s2) return 0;
    if (!s1) return -1; // NULL < non-NULL
    if (!s2) return 1;  // non-NULL > NULL

    while (*s1 != '\0' && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (int)((unsigned char)*s1 - (unsigned char)*s2);
}

// n byte'ı kaynaktan hedefe kopyalar.
void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    if (!d || !s) return dest;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

// n byte'ı belirtilen değerle doldurur.
void *memset(void *s, int c, size_t n) {
    uint8_t *p = s;
    if (!p) return s;
    uint8_t value = (uint8_t)c; // Sadece düşük 8 bit kullanılır
    while (n--) {
        *p++ = value;
    }
    return s;
}

// n byte'ı kaynaktan hedefe kopyalar, çakışan bölgeleri doğru yönetir.
void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    if (!d || !s || n == 0) return dest;

    // Eğer hedef kaynak adresten büyükse, sondan kopyala
    if (d > s) {
        d = (uint8_t *)dest + n - 1;
        s = (const uint8_t *)src + n - 1;
        while (n--) {
            *d-- = *s--;
        }
    } else { // Hedef kaynak adresten küçük veya eşitse, baştan kopyala
        while (n--) {
            *d++ = *s++;
        }
    }
    return dest;
}

// string.c sonu