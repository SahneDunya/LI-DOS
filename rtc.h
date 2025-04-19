// rtc.h
// Lİ-DOS RTC (Real-Time Clock / CMOS) Modulu Arayuzu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: CMOS yongasindan tarih ve saat okuma.

#ifndef _RTC_H
#define _RTC_H

#include "types.h" // uint8_t, uint16_t gibi
#include "asm.h"   // inb, outb fonksiyonlari icin

// CMOS Port Adresleri
#define CMOS_ADDRESS_PORT 0x70 // Adres Kayitcigi (Yazma)
#define CMOS_DATA_PORT    0x71 // Veri Kayitcigi (R/W)

// CMOS Kayitcik Adresleri (Offsetleri)
#define CMOS_REG_SECONDS       0x00 // Saniye (BCD veya Binary)
#define CMOS_REG_MINUTES       0x02 // Dakika (BCD veya Binary)
#define CMOS_REG_HOURS         0x04 // Saat (BCD veya Binary)
#define CMOS_REG_WEEKDAY       0x06 // Haftanın Günü (1=Pazar, 7=Cumartesi)
#define CMOS_REG_DAY_OF_MONTH  0x07 // Ayin Gunu (BCD veya Binary)
#define CMOS_REG_MONTH         0x08 // Ay (BCD veya Binary)
#define CMOS_REG_YEAR          0x09 // Yil (BCD veya Binary, genellikle son 2 hane)
#define CMOS_REG_STATUS_A      0x0A // Durum Kayitcigi A
#define CMOS_REG_STATUS_B      0x0B // Durum Kayitcigi B (Data Format, 12/24 Saat, Daylight Savings Enable)
#define CMOS_REG_STATUS_C      0x0C // Durum Kayitcigi C (Interrupt Status - Okumak kesmeyi sifirlar)
#define CMOS_REG_STATUS_D      0x0D // Durum Kayitcigi D (Valid RAM)

// CMOS Durum Kayitcigi B (Status B) Bitleri
#define CMOS_STATUS_B_DST_EN    0x01 // Daylight Savings Enable
#define CMOS_STATUS_B_HOUR_12_24 0x02 // 12/24 Saat Modu (1=12 saat, 0=24 saat)
#define CMOS_STATUS_B_DATA_MODE 0x04 // Veri Modu (1=Binary, 0=BCD)
#define CMOS_STATUS_B_SQW_EN    0x08 // Square Wave Enable
#define CMOS_STATUS_B_UPDATE_EN 0x10 // Update Ended Interrupt Enable
#define CMOS_STATUS_B_ALARM_EN  0x20 // Alarm Interrupt Enable
#define CMOS_STATUS_B_PERIODIC_EN 0x40 // Periodic Interrupt Enable
#define CMOS_STATUS_B_SET       0x80 // SET bit (RTC Update Cycle'i durdurur)

// Tarih ve Saat bilgisi icin yapilar
struct rtc_time {
    uint8_t hours;   // 0-23 (veya 1-12 AM/PM)
    uint8_t minutes; // 0-59
    uint8_t seconds; // 0-59
};

struct rtc_date {
    uint8_t year;    // Yilin son 2 hanesi
    uint8_t month;   // 1-12
    uint8_t day;     // 1-31
    uint8_t weekday; // 1-7 (Pazar=1)
};

// RTC modülünü baslatir.
void rtc_init(void);

// CMOS'tan belirtilen kayitcigin degerini okur.
// reg: Okunacak CMOS kayitciginin adresi (0x00-0x7F).
// Donus degeri: Okunan byte degeri veya -1 hata.
int rtc_read_register(uint8_t reg);

// CMOS'taki belirtilen kayitciga deger yazar.
// reg: Yazilacak CMOS kayitciginin adresi (0x00-0x7F).
// value: Yazilacak byte degeri.
void rtc_write_register(uint8_t reg, uint8_t value);

// CMOS'tan anlik saat bilgisini okur.
// time: Saat bilgisinin yazilacagi yapiya pointer.
// Donus degeri: 0 basari, -1 hata.
int rtc_get_time(struct rtc_time *time);

// CMOS'tan anlik tarih bilgisini okur.
// date: Tarih bilgisinin yazilacagi yapiya pointer.
// Donus degeri: 0 basari, -1 hata.
int rtc_get_date(struct rtc_date *date);

// BCD (Binary Coded Decimal) degeri Binary değere çevirir.
uint8_t bcd_to_binary(uint8_t bcd);


#endif // _RTC_H