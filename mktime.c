// mktime.c
// Lİ-DOS Zaman Dönüşüm Modulu Implementasyonu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89

#include "mktime.h" // Zaman dönüşüm arayüzü ve yapıları
// Gerekirse console modulu icin
 #include "console.h"
 extern void console_puts(const char *s);
 extern void printk(const char *fmt, ...); // Debug icin

// Bir yilin artik yil olup olmadigini kontrol eder
// Parametre: 1900'den itibaren yil farki (struct tm.tm_year formati)
// Donus: 1 eger artik yil ise, 0 degilse.
static int is_leap_year(int year_since_1900) {
    int year = 1900 + year_since_1900; // Gercek yil
    // Artik yil kurali: 4'e bolunur VE (100'e bolunmez VEYA 400'e bolunur)
    return ((year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0)));
}

// Aylardaki gun sayilari (Artik yil haric)
static const int days_in_month[] = {
    0, // Ay 0 (gecersiz, 1-indexed kullanmiyoruz ama 0. indeksi bos birakiyoruz)
    31, // Ay 1 (Ocak)
    28, // Ay 2 (Subat) - is_leap_year ile ayarlanacak
    31, // Ay 3 (Mart)
    30, // Ay 4 (Nisan)
    31, // Ay 5 (Mayis)
    30, // Ay 6 (Haziran)
    31, // Ay 7 (Temmuz)
    31, // Ay 8 (Agustos)
    30, // Ay 9 (Eylul)
    31, // Ay 10 (Ekim)
    30, // Ay 11 (Kasim)
    31  // Ay 12 (Aralik)
};

// struct tm'de verilen yapısal zamani, epoch'tan bu yana saniye cinsinden time_t'ye donusturur.
// tm_isdst bu implementasyonda dikkate alinmaz.
// tm_wday ve tm_yday hesaplanabilir ama bu ornekte goz ardi edildi.
time_t mktime(struct tm *tm_ptr) {
    uint32_t total_days = 0;
    int year;
    int month;

    // Giriş değerlerini kontrol et (Basit kontrol)
    if (!tm_ptr) {
        return (time_t)-1; // Geçersiz pointer
    }
    // Daha kapsamlı kontrol gerekebilir (örn. ay, gün, saat araliklari)
    // mktime normalde bu değerleri normalize eder. Bu ornekte normalize etmiyoruz.

    // Yıl hesaplaması: Epoch (1970) ile hedef yıl (1900 + tm_year) arasındaki tam yılları say.
    // Epoch yili (1970) hesaplamaya dahil edilmez, hedef yil dahil edilir (gunleri ayri hesaplanacak).
    // Baslangic yili: 1970
    // Bitis yili: 1900 + tm_ptr->tm_year - 1 (Hedef yildan bir onceki yil)

    // Yıl 1970'den küçükse veya çok büyükse (uint32_t limitleri), hata döndür.
    if (tm_ptr->tm_year < 70) { // 1900 + year < 1970
         return (time_t)-1; // Epoch'tan onceki yillar desteklenmiyor
    }
    // TODO: Üst yıl limitini de kontrol et (örn. 2038 veya uint32_t bitişi 2106 civarı)


    // Epoch yili (1970) ile hedef yildan bir onceki yil arasindaki gun sayisini hesapla.
    // Yil 1970'den basladigi icin loop 1970'den (1900+70) baslar.
    for (year = 1900 + 70; year < 1900 + tm_ptr->tm_year; year++) {
        total_days += 365;
        if (is_leap_year(year - 1900)) { // Yıl 1900'den itibaren kaç yıl?
            total_days += 1; // Artik yil ise 1 gun ekle
        }
    }

    // Hedef yildaki (1900 + tm_ptr->tm_year) tam aylardaki (Ocak'tan tm_mon-1'e kadar) gun sayisini ekle.
    // Ay 0-indexed oldugu icin loop tm_mon'a kadar gider.
    for (month = 1; month < tm_ptr->tm_mon + 1; month++) { // tm_mon 0-11, dizi 1-12
        total_days += days_in_month[month];
        // Subat ayi ve artik yil ise 1 gun daha ekle
        if (month == 2 && is_leap_year(tm_ptr->tm_year)) {
            total_days += 1;
        }
    }

    // Hedef aydaki gunleri ekle (tm_mday 1-indexed oldugu icin tm_mday - 1 kadar gun)
    total_days += (tm_ptr->tm_mday - 1);

    // Toplam gun sayisini saniyeye cevir ve gun icindeki saniyeleri ekle.
    // uint32_t kullanarak carpma islemlerinde tasmayi onle.
    uint32_t total_seconds = total_days * (uint32_t)86400; // 86400 = 24 * 60 * 60
    total_seconds += tm_ptr->tm_hour * (uint32_t)3600; // 3600 = 60 * 60
    total_seconds += tm_ptr->tm_min * (uint32_t)60;
    total_seconds += tm_ptr->tm_sec;

    // tm_wday ve tm_yday alanlarini hesaplayip guncelleyebiliriz ama bu ornekte yapilmiyor.
    // tm_ptr->tm_wday = (total_days + epoch_wday_offset) % 7; // Epoch'un gunu (1 Oca 1970 Perşembe = 4)
    // tm_ptr->tm_yday = ... hesapla ...

    return (time_t)total_seconds;
}

// mktime.c sonu