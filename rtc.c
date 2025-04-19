// rtc.c
// Lİ-DOS RTC (Real-Time Clock / CMOS) Modulu Implementasyonu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: CMOS yongasindan tarih ve saat okuma.

#include "rtc.h"
#include "printk.h" // Debug cikti icin
// Sure implementasyonu icin (CMOS update bekleme)
#include "sys.h"
// Kesmeleri kapatmak/acmak icin (NMI maskeleme)
#include "asm.h" // cli, sti fonksiyonlari

// BCD (Binary Coded Decimal) degeri Binary değere çevirir.
uint8_t bcd_to_binary(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

// CMOS'tan belirtilen kayitcigin degerini okur.
int rtc_read_register(uint8_t reg) {
    uint8_t value;
    // NMI'yi maskeleme (isteğe bağlı, adres portunun bit 7'sini set et)
    // Çoğu durumda gerekmez ama güveli okuma için yapılabilir.
     outb(CMOS_ADDRESS_PORT, reg | 0x80); // NMI devre disi
    outb(CMOS_ADDRESS_PORT, reg); // NMI etkin kalir

    // Veri portundan degeri oku
     io_delay(); // Okuma arasinda kisa gecikme gerekebilir.
    value = inb(CMOS_DATA_PORT);

    // NMI maskelemesi yapildiysa geri açılır.
     outb(CMOS_ADDRESS_PORT, reg & 0x7F); // NMI etkin

    return value;
}

// CMOS'taki belirtilen kayitciga deger yazar.
void rtc_write_register(uint8_t reg, uint8_t value) {
    // NMI maskeleme (isteğe bağlı)
    // outb(CMOS_ADDRESS_PORT, reg | 0x80); // NMI devre disi
    outb(CMOS_ADDRESS_PORT, reg); // NMI etkin

    // Veri portuna yaz
    // io_delay(); // Yazma arasinda kisa gecikme gerekebilir.
    outb(CMOS_DATA_PORT, value);

    // NMI maskelemesi yapildiysa geri açılır.
    // outb(CMOS_ADDRESS_PORT, reg & 0x7F); // NMI etkin
}


// RTC modülünü baslatir.
void rtc_init(void) {
    // RTC init icin ozel donanim ayarlari genellikle gerekmez.
    // Sadece modülün var oldugunu belirtelim.
    // CMOS saat/tarih formati (BCD/Binary, 12/24 saat) Status B kayitcigindan okunabilir.
    uint8_t status_b = rtc_read_register(CMOS_REG_STATUS_B);
    if ((status_b & CMOS_STATUS_B_DATA_MODE) == 0) {
         printk("RTC Init: CMOS BCD modunda calisiyor.\r\n");
    } else {
         printk("RTC Init: CMOS Binary modunda calisiyor.\r\n");
    }

    if ((status_b & CMOS_STATUS_B_HOUR_12_24) != 0) {
        printk("RTC Init: CMOS 12-saat modunda calisiyor.\r\n");
    } else {
        printk("RTC Init: CMOS 24-saat modunda calisiyor.\r\n");
    }

    printk("RTC Init: Gercek Zaman Saati modulu yuklendi.\r\n");
}


// CMOS'tan anlik saat bilgisini okur.
int rtc_get_time(struct rtc_time *time) {
    uint8_t status_b;
    uint8_t seconds, minutes, hours;

    if (!time) return -1;

    // CMOS Update Cycle bitinin (Status A Bit 7) 0 olmasini bekle
    // Bu, okuma sirasinda saatin guncellenmesini engeller ve tutarli veri saglar.
    // Çok hızlı olacağı için genellikle bekleme gerekmez ama garantili okuma için yapılabilir.
    // while(rtc_read_register(CMOS_REG_STATUS_A) & 0x80); // Update Cycle devam ediyor mu?

    // Saat, dakika, saniyeyi oku
    seconds = rtc_read_register(CMOS_REG_SECONDS);
    minutes = rtc_read_register(CMOS_REG_MINUTES);
    hours = rtc_read_register(CMOS_REG_HOURS);

    // Veri formatini kontrol et (BCD veya Binary)
    status_b = rtc_read_register(CMOS_REG_STATUS_B);

    // Eger BCD modunda ise Binary'ye cevir
    if ((status_b & CMOS_STATUS_B_DATA_MODE) == 0) { // BCD
        time->seconds = bcd_to_binary(seconds);
        time->minutes = bcd_to_binary(minutes);
        hours = bcd_to_binary(hours); // Gecici degiskene cevir
    } else { // Binary
        time->seconds = seconds;
        time->minutes = minutes;
        // hours = hours; // Binary zaten
    }

    // 12-saat modunda ise (Status B Bit 1 set), 24-saat formatina cevir
    if ((status_b & CMOS_STATUS_B_HOUR_12_24) != 0) { // 12-saat modu
        if (hours & 0x80) { // PM ise (Bit 7 set)
            hours = (hours & 0x7F) + 12; // Bit 7'yi temizle ve 12 ekle
        }
         if ((hours & 0x7F) == 12 && !(hours & 0x80)) { // Saat 12 AM ise
             hours = 0; // Gece yarısı 00
         }
         if ((hours & 0x7F) == 12 && (hours & 0x80)) { // Saat 12 PM ise
             hours = 12; // Öğlen 12
         }
        time->hours = hours; // Donusturulmus saati kaydet
    } else { // 24-saat modu
        time->hours = hours; // Dogrudan kullan
    }

    // printk("RTC Time: %02u:%02u:%02u\r\n", time->hours, time->minutes, time->seconds);
    return 0;
}

// CMOS'tan anlik tarih bilgisini okur.
int rtc_get_date(struct rtc_date *date) {
     uint8_t status_b;
     uint8_t year, month, day, weekday;

     if (!date) return -1;

     // CMOS Update Cycle bitinin (Status A Bit 7) 0 olmasini bekle (isteğe bağlı)
     // while(rtc_read_register(CMOS_REG_STATUS_A) & 0x80);

     // Yil, ay, gun, haftanin gununu oku
     year = rtc_read_register(CMOS_REG_YEAR);
     month = rtc_read_register(CMOS_REG_MONTH);
     day = rtc_read_register(CMOS_REG_DAY_OF_MONTH);
     weekday = rtc_read_register(CMOS_REG_WEEKDAY);

     // Veri formatini kontrol et (BCD veya Binary)
     status_b = rtc_read_register(CMOS_REG_STATUS_B);

     // Eger BCD modunda ise Binary'ye cevir
     if ((status_b & CMOS_STATUS_B_DATA_MODE) == 0) { // BCD
         date->year = bcd_to_binary(year);
         date->month = bcd_to_binary(month);
         date->day = bcd_to_binary(day);
         date->weekday = bcd_to_binary(weekday);
     } else { // Binary
         date->year = year;
         date->month = month;
         date->day = day;
         date->weekday = weekday;
     }

     // Yil genellikle son 2 hanedir. Tam yıl (örn. 2025) icin BIOS veya baska bir kaynaktan
     // yuzyil bilgisini almak gerekir (CMOS'ta 0x32'de saklanabilir).
     // Bu ornekte sadece son 2 haneyi dondurur.

     printk("RTC Date: %02u/%02u/%02u (Weekday %u)\r\n", date->day, date->month, date->year, date->weekday);
     return 0;
}


// rtc.c sonu