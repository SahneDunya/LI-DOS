// speaker.c
// Lİ-DOS PC Hoparlörü Modulu Implementasyonu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: PIT ve Kontrol Portu uzerinden PC Hoparloru yonetimi.

#include "speaker.h"
#include "printk.h" // Debug cikti icin
// Sure implementasyonu icin gerekli (polling gecikmesi veya timer)
// Eger timer kesmesi kullanilacaksa traps.h ve timer handlerina bagimlilik olur.
// Basit ornekte polling gecikmesi varsayalim (veya sched.h'ten schedule cagrisi).
#include "sys.h" // Ornek gecikme veya schedule icin

// PIT Referans Saati (MHz) - PC Speaker icin kullanılan PIT kanali 2
#define PIT_BASE_FREQ 1193180 // Hz

void speaker_init(void) {
    // Hoparlor init icin ozel donanim ayarlari genellikle gerekmez.
    // Sadece modülün var oldugunu belirtelim.
    printk("Speaker Init: PC Hoparlor modulu yuklendi.\r\n");
}

// Belirli bir frekansta ses çalar (bloklayici).
void speaker_play_tone(uint16_t frequency, uint16_t duration_ms) {
    uint16_t divisor;
    uint8_t speaker_status;

    if (frequency == 0) {
        speaker_off(); // Frekans 0 ise sesi kapat
        return;
    }

    // Frekansi PIT bolucusune cevir
    // Bolucu = PIT_BASE_FREQ / frequency
    divisor = (uint16_t)(PIT_BASE_FREQ / frequency);

    // PIT Kanal 2'yi kare dalga moduna ayarla (Mod 3) ve bolucuyu yukle
    // Kontrol kelimesi: Kanal 2 (10b), Low/High byte erisimi (11b), Mod 3 (011b), Binary mod (0b) -> 10110110b = 0xB6
    outb(0x43, 0xB6); // PIT Kontrol Kelimesi portu

    // Bolucunun dusuk ve yuksek byte'ini PIT Kanal 2 veri portuna yaz
    outb(PIT_CHANNEL2_DATA_PORT, (uint8_t)(divisor & 0xFF)); // Dusuk byte
    outb(PIT_CHANNEL2_DATA_PORT, (uint8_t)(divisor >> 8)); // Yuksek byte

    // Hoparloru etkinlestir (Kontrol Portu 0x61 Bit 0 ve Bit 1)
    // Mevcut durumu oku, Bit 0 ve 1'i set et, geri yaz.
    speaker_status = inb(SPEAKER_CONTROL_PORT);
    outb(SPEAKER_CONTROL_PORT, speaker_status | 0x03); // Bit 0 (PIT gate to speaker) ve Bit 1 (speaker enable) set

    // Belirtilen sure kadar bekle (bloklayici)
    // Basit sure implementasyonu: Donem sayisi kadar boş döngü veya schedule() çağrısı
    // Daha doğrusu, bir timer kesme handler'ı sureyi saymalı.
    // Bu ornekte cok basit bir polling gecikmesi yapalim.
    // GERÇEK OS'TE BU ASLA YAPILMAMALIDIR - CPU'YU TIKAR.
    // Timer kesmesi kullanilarak bu islev implemente edilmelidir.
    // Ornek polling gecikmesi (yaklaşık süreler için):
    {
        uint32_t i;
        uint32_t delay_loops = (uint32_t)duration_ms * 1000; // Milisaniye başına 1000 döngü gibi (kalibrasyon gerekir)
        for (i = 0; i < delay_loops; i++) {
            // Çok kısa bir işlem yap (örn. NOP, io_delay)
            // io_delay(); // asm.h'ten (varsa)
            asm volatile("nop"); // C89 inline asm ornek
        }
    }
     // Veya zamanlayıcı modülü varsa:
     // timer_sleep_ms(duration_ms); // schedule cagrisi yapabilir

    // Süre doldu, sesi kapat
    speaker_off();
}

// Hoparlörü kapatır (sesi durdurur).
void speaker_off(void) {
    uint8_t speaker_status;
    // Hoparlor etkinlestirme bitlerini (Bit 0 ve 1) temizle
    speaker_status = inb(SPEAKER_CONTROL_PORT);
    outb(SPEAKER_CONTROL_PORT, speaker_status & ~0x03); // Bit 0 ve Bit 1 temizle
}

// speaker.c sonu