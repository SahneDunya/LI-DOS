// vsprintf.c
// Lİ-DOS Formatlı String Yazdırma Yardımcısı
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: vsprintf fonksiyonunun minimal implementasyonu.
// printk gibi fonksiyonlar bu dosyayi kullanir.

#include "types.h" // Temel türler
#include "vsprintf.h" // vsprintf bildirimi ve va_list tanımlarını içerir.
                      // va_list, va_start, va_arg, va_end makrolari burada tanimli olmali.

// vsprintf.h ornegi (va_list tanimlarini icermeli):
 #ifndef _VSPRINTF_H
 #define _VSPRINTF_H
 #include "types.h" // size_t icin
// // va_list tanimlari - stdarg.h yerine mimariye ozel implementasyon
 typedef char *va_list;
 #define va_start(ap, last) (ap = (va_list)&last + sizeof(last))
 #define va_arg(ap, type) (ap += sizeof(type), *(type *)(ap - sizeof(type)))
 #define va_end(ap)
//
 extern int vsprintf(char *buffer, const char *format, va_list va);
 #endif // _VSPRINTF_H


// --- Sayiyi Stringe Cevirme Yardimcilari ---
// Bu fonksiyonlar vsprintf'in ihtiyac duyduğu yardımcı fonksiyonlardır.
// vsprintf.c içinde static olarak tanımlanabilirler.
static void reverse_string(char *str) {
    int len = 0;
    char *s = str;
    char temp;
    while(*s != '\0') { len++; s++; } // strlen implementasyonu
    int i, j;
    for (i = 0, j = len - 1; i < j; i++, j--) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
    }
}

// İşaretsiz sayıyı stringe çevirir (decimal veya hex)
// buffer: Sonuç stringin yazılacağı buffer (yeterince büyük olmalı)
// base: Sayı sistemi tabanı (10 veya 16)
// Donus: buffer'in baslangic adresi
static char *uint_to_string(unsigned long value, char *buffer, int base) { // long kullanimi 32-bit adres icin
    char digits[] = "0123456789ABCDEF"; // Rakam karakterleri
    int i = 0;
    unsigned long temp = value; // temp long olmalı

    // Sıfır özel durum
    if (temp == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return buffer;
    }

    // Sayıyı basamaklarına ayır ve ters sırada buffera yaz
    while (temp != 0) {
        buffer[i++] = digits[temp % base];
        temp /= base;
    }
    buffer[i] = '\0'; // Stringi sonlandır

    reverse_string(buffer); // Stringi ters çevir

    return buffer; // Bufferin basini dondur
}

// İşaretli sayıyı stringe çevirir (decimal)
static char *int_to_string(int value, char *buffer, int base) {
    // Sadece base 10 desteklenir
    if (base != 10) return buffer; // Hata veya varsayılan

    if (value < 0) {
        *buffer++ = '-';
        value = -value; // Pozitif yap
    }
    // İşaretsiz çeviriciyi çağır
    uint_to_string((unsigned int)value, buffer, 10); // int'i unsigned int'e cast
                                                    // Dikkat: 16-bit int Max 32767, uint 65535.
                                                    // vsprintf'in %d'si stackten int okur, %u stackten unsigned int.
                                                    // vsprintf cagirirken intler otomatik olarak int olarak itilir,
                                                    // %d icin va_arg(va, int), %u icin va_arg(va, unsigned int)
                                                    // %p icin va_arg(va, void*). 16-bitte far pointer 32-bit, bu yuzden uint_to_string long almalı.


    return buffer; // Yazmaya basladigi yeri dondur (isaretli sayilar icin isaretin sonrasi)
}


// vsprintf fonksiyonunun implementasyonu
// vsprintf: string bufferına formatlı çıktı yazar.
// buffer: Çıktının yazılacağı hedef buffer.
// format: Format stringi (örn. "Sayi: %d, Yazi: %s").
// va: Değişken argüman listesi.
// Donus degeri: Yazılan karakter sayısı (null terminator hariç).

int vsprintf(char *buffer, const char *format, va_list va) {
    char *buf_ptr = buffer;
    char temp_num_buffer[12]; // Sayı çevirimi için geçici buffer (int/uint için yeterli olmalı)
    char temp_ptr_buffer[9]; // Pointer çevirimi için (32-bit hex: 8 digit + null)

    if (!buffer || !format) return 0;

    while (*format != '\0') {
        if (*format == '%') {
            format++; // '%' işaretini atla
            switch (*format) {
                case 'c': // Tek karakter
                    {
                        char c = (char)va_arg(va, int); // char int olarak itilir stacke
                        *buf_ptr++ = c;
                    }
                    break;
                case 's': // String
                    {
                        char *s = va_arg(va, char *);
                        if (!s) s = "(null)";
                        while (*s != '\0') {
                            *buf_ptr++ = *s++;
                        }
                    }
                    break;
                case 'd': // Signed Decimal Integer
                case 'i':
                    {
                        int val = va_arg(va, int);
                        // Sayıyı stringe çevir
                        int_to_string(val, temp_num_buffer, 10);
                        // Çevrilen stringi buffer'a kopyala
                        char *s = temp_num_buffer;
                        while(*s != '\0') {
                            *buf_ptr++ = *s++;
                        }
                    }
                    break;
                case 'u': // Unsigned Decimal Integer
                    {
                        unsigned int val = va_arg(va, unsigned int);
                        // Sayıyı stringe çevir
                        uint_to_string((unsigned long)val, temp_num_buffer, 10); // uint_to_string long alıyor
                         char *s = temp_num_buffer;
                        while(*s != '\0') {
                            *buf_ptr++ = *s++;
                        }
                    }
                    break;
                case 'x': // Unsigned Hexadecimal Integer (lowercase)
                case 'X': // Unsigned Hexadecimal Integer (uppercase)
                    {
                        unsigned int val = va_arg(va, unsigned int);
                         // Sayıyı hex stringe çevir
                        uint_to_string((unsigned long)val, temp_num_buffer, 16); // base 16

                        // Büyük/küçük harf desteği eklenebilir (uint_to_string hex karakterleri büyük/küçük üretebilir)
                         char *s = temp_num_buffer;
                        while(*s != '\0') {
                            *buf_ptr++ = *s++;
                        }
                    }
                    break;
                 case 'p': // Pointer (Genellikle hex adres olarak)
                     {
                         void *ptr = va_arg(va, void *); // Stack'ten 32-bit far pointer okunur
                         unsigned long val = (unsigned long)ptr; // Pointer'ı unsigned long'a cast et

                         // Pointer adresini hex stringe çevir (32-bit adres için)
                         uint_to_string(val, temp_ptr_buffer, 16); // Hex çevirici

                         // printk %p için genellikle 0x formatını kullanır.
                         *buf_ptr++ = '0';
                         *buf_ptr++ = 'x';
                         // temp_num_buffer'ı kopyala
                         char *s = temp_ptr_buffer; // Pointer stringi
                         // Eğer 32-bit hex 8 haneden az ise başına sıfır ekle
                         // Bu implementasyon basittir, sıfır doldurma yapmaz.
                         // Gelişmiş vsprintf'lerde padding/width yönetilir.
                         // Basitlik icin 8 karakter dondurdugunu varsayalim.
                         while(*s != '\0') {
                            *buf_ptr++ = *s++;
                         }
                     }
                     break;

                case '%': // %% -> '%' karakteri
                    *buf_ptr++ = '%';
                    break;
                // ... Diğer format belirteçleri (%f float, %e scientific, padding, width, precision vb.)
                // Bu minimal implementasyonda desteklenmez.
                default:
                    // Bilinmeyen format belirteci, olduğu gibi yazdır veya hata ver
                    *buf_ptr++ = '%'; // Orjinal %
                    *buf_ptr++ = *format; // Bilinmeyen karakter
                    break;
            }
        } else {
            // Format karakteri değil, olduğu gibi kopyala
            *buf_ptr++ = *format;
        }
        format++; // Bir sonraki format karakterine geç
    }

    *buf_ptr = '\0'; // String sonuna null terminator ekle

    return (int)(buf_ptr - buffer); // Yazılan karakter sayısı (null hariç)
}

// vsprintf.c sonu