// vsprintf.c
// Lİ-DOS Kernel Formatli Cikti Modulu Implementasyonu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89

#include "vsprintf.h" // vsprintf arayuzu
#include "va_list.h"  // Variadic arguman makrolari
// #include "console.h" // Debug icin console_putc kullanilabilir

// --- Yardimci Fonksiyonlar ---

// Bir stringi tersine cevirir (integer'dan stringe cevirme icin)
static void reverse(char *buf, int len) {
    int i = 0;
    int j = len - 1;
    char temp;
    while (i < j) {
        temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
        i++;
        j--;
    }
}

// Isaretli (signed) long tipindeki bir sayiyi stringe cevirir.
// value: Cevrilecek sayi.
// buf: Sonucun yazilacagi buffer.
// base: Sayi sistemi tabani (10, 16, 8).
// Donus degeri: Yazilan karakter sayisi.
static int long_to_string(long value, char *buf, int base) {
    int i = 0;
    int is_negative = 0;
    unsigned long temp_val; // Hesaplamalar icin isaretsiz kopya

    if (value == 0) {
        buf[i++] = '0';
        buf[i] = '\0';
        return 1;
    }

    if (base == 10 && value < 0) {
        is_negative = 1;
        temp_val = (unsigned long)-value; // Negatif degeri pozitif yap
    } else {
        temp_val = (unsigned long)value;
    }

    // Sayiyi tabana gore rakamlarina ayir
    while (temp_val != 0) {
        int rem = temp_val % base;
        buf[i++] = (rem > 9) ? (rem - 10 + 'a') : (rem + '0'); // 0-9 veya a-f
        temp_val = temp_val / base;
    }

    // Negatif ise '-' isaretini ekle (sadece onluk taban)
    if (is_negative) {
        buf[i++] = '-';
    }

    buf[i] = '\0'; // Null terminator ekle

    reverse(buf, i); // Stringi tersine cevir (rakamlar tersten yazildi)

    return i;
}

// Isaretsiz (unsigned) long tipindeki bir sayiyi stringe cevirir.
// value: Cevrilecek sayi.
// buf: Sonucun yazilacagi buffer.
// base: Sayi sistemi tabani (10, 16, 8, 2).
// Donus degeri: Yazilan karakter sayisi.
static int unsigned_long_to_string(unsigned long value, char *buf, int base) {
    int i = 0;
    unsigned long temp_val = value;

    if (temp_val == 0) {
        buf[i++] = '0';
        buf[i] = '\0';
        return 1;
    }

    // Sayiyi tabana gore rakamlarina ayir
    while (temp_val != 0) {
        int rem = temp_val % base;
        buf[i++] = (rem > 9) ? (rem - 10 + 'a') : (rem + '0'); // 0-9 veya a-f
        temp_val = temp_val / base;
    }

    buf[i] = '\0'; // Null terminator ekle

    reverse(buf, i); // Stringi tersine cevir

    return i;
}

// --- Ana vsprintf Fonksiyonu ---
// Kernel Safe vsprintf (vsnprintf benzeri) fonksiyonu.
// Formatli ciktıyı 'buf' bufferina 'size' byte'i gecmeyecek sekilde yazar.
// fmt: Format stringi.
// args: va_list'teki argumanlar.
// Donus degeri: Buffer'a yazilan karakter sayisi (NULL terminator haric).
// Buffer'in 'size' byte'tan kisa olmasi durumunda tasmayi onler.
int vsprintf(char *buf, size_t size, const char *fmt, va_list args) {
    char *str = buf; // Buffer'a yazmak icin pointer
    int fmt_len = 0; // Formatlanan toplam karakter sayisi (NULL haric)
    char num_buf[32]; // Sayi cevirme icin gecici buffer (long max hane sayisi + isaret)

    // Buffer gecerlilik kontrolu
    if (!buf || size == 0) {
        return 0; // Gecersiz buffer veya boyut
    }
    // Buffer'in sonuna NULL terminator icin yer birak
    size_t max_write_len = size - 1;

    // Format stringi boyunca ilerle
    while (*fmt != '\0') {
        if (*fmt == '%') {
            fmt++; // '%' karakterini atla

            // Eger '%' ile bitti ise veya bilinmeyen format ise
            if (*fmt == '\0') break;

            // Format belirleyicisini isle
            switch (*fmt) {
                case '%': // '%%' -> '%' karakteri
                    if (fmt_len < max_write_len) {
                        *str++ = '%';
                        fmt_len++;
                    }
                    break;
                case 'c': // '%c' -> Karakter
                    {
                        char c = (char)VA_ARG(args, int); // char, int olarak itilir stacke
                        if (fmt_len < max_write_len) {
                            *str++ = c;
                            fmt_len++;
                        }
                    }
                    break;
                case 's': // '%s' -> String
                    {
                        const char *s = VA_ARG(args, const char *); // String pointer
                        if (s == (const char *)0) s = "(null)"; // NULL pointer durumu
                        while (*s != '\0') {
                            if (fmt_len < max_write_len) {
                                *str++ = *s;
                                fmt_len++;
                                s++;
                            } else {
                                goto end_formatting; // Buffer doldu, cik
                            }
                        }
                    }
                    break;
                case 'd': // '%d' veya '%i' -> Isaretli ondalik integer
                case 'i':
                    {
                        int val = VA_ARG(args, int); // int stackte int olarak itilir
                        int num_len = long_to_string((long)val, num_buf, 10);
                        // Num_buf'taki stringi ana buffera kopyala
                        int i;
                        for (i = 0; i < num_len; i++) {
                             if (fmt_len < max_write_len) {
                                 *str++ = num_buf[i];
                                 fmt_len++;
                             } else {
                                 goto end_formatting; // Buffer doldu
                             }
                        }
                    }
                    break;
                 case 'u': // '%u' -> Isaretsiz ondalik integer
                    {
                        unsigned int val = VA_ARG(args, unsigned int); // unsigned int stackte unsigned int olarak itilir
                        int num_len = unsigned_long_to_string((unsigned long)val, num_buf, 10);
                        int i;
                        for (i = 0; i < num_len; i++) {
                             if (fmt_len < max_write_len) {
                                 *str++ = num_buf[i];
                                 fmt_len++;
                             } else {
                                 goto end_formatting; // Buffer doldu
                             }
                        }
                    }
                    break;
                 case 'x': // '%x' veya '%X' -> Hexadecimal integer (kucuk harf)
                 case 'X': // Hexadecimal integer (buyuk harf)
                    {
                        unsigned int val = VA_ARG(args, unsigned int);
                        int num_len = unsigned_long_to_string((unsigned long)val, num_buf, 16);
                        int i;
                        // '%X' icin buyuk harfe cevirme yapilabilir burada veya unsigned_long_to_string icinde.
                        // Basitlik icin ornekte kucuk harf ('a-f') kullanildi. Buyuk harf istenirse
                        // num_buf karakterleri 'a'dan 'f'ye ise 'A'dan 'F'ye cevrilir.
                        for (i = 0; i < num_len; i++) {
                             char digit = num_buf[i];
                             // Eger %X ise ve karakter 'a'-'f' arasindaysa, buyuk harfe cevir.
                             // if (*fmt == 'X' && digit >= 'a' && digit <= 'f') {
                             //     digit = digit - 'a' + 'A';
                             // }
                             if (fmt_len < max_write_len) {
                                 *str++ = digit;
                                 fmt_len++;
                             } else {
                                 goto end_formatting; // Buffer doldu
                             }
                        }
                    }
                    break;
                 case 'o': // '%o' -> Sekizlik (octal) integer
                    {
                         unsigned int val = VA_ARG(args, unsigned int);
                         int num_len = unsigned_long_to_string((unsigned long)val, num_buf, 8);
                         int i;
                         for (i = 0; i < num_len; i++) {
                              if (fmt_len < max_write_len) {
                                  *str++ = num_buf[i];
                                  fmt_len++;
                              } else {
                                  goto end_formatting; // Buffer doldu
                              }
                         }
                    }
                    break;
                // '%p' (pointer) veya diger format belirleyicileri (genislik, precision, flagler)
                // daha karmasik implementasyon gerektirir ve bu basit ornekte atlanmistir.
                // '%p' genellikle adresi hexadecimal yazar, 16-bit Real Mode'da Segment:Offset olarak yazmak faydali olabilir.
                // Bu da pointerin segment ve offsetini elde etme yollari gerektirir ki va_arg ile dogrudan kolay olmayabilir.
                default:
                    // Bilinmeyen format belirleyici, '%' ve karakterin kendisini yaz
                    if (fmt_len < max_write_len) {
                        *str++ = '%';
                        fmt_len++;
                    }
                    if (fmt_len < max_write_len) {
                        *str++ = *fmt;
                        fmt_len++;
                    }
                    break;
            }
            fmt++; // Format belirleyicisinden sonraki karakteri atla
        } else {
            // Normal karakter, buffera yaz
            if (fmt_len < max_write_len) {
                *str++ = *fmt;
                fmt_len++;
            }
            fmt++; // Sonraki karaktere gec
        }
    }

end_formatting:
    // Buffer sonuna NULL terminator ekle
    *str = '\0';

    return fmt_len; // Yazilan karakter sayisini dondur (NULL haric)
}

// vsprintf.c sonu