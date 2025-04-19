// speaker.h
// Lİ-DOS PC Hoparlörü Modulu Arayuzu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: PIT ve Kontrol Portu uzerinden PC Hoparloru yonetimi.

#ifndef _SPEAKER_H
#define _SPEAKER_H

#include "types.h" // uint8_t, uint16_t gibi
#include "asm.h"   // inb, outb fonksiyonlari icin

// PIT Kanal 2 Veri Portu (Frekans ayarlari icin)
#define PIT_CHANNEL2_DATA_PORT 0x42
// Kontrol Portu (Hoparlor gating icin)
#define SPEAKER_CONTROL_PORT   0x61

// Speaker modülünü baslatir.
void speaker_init(void);

// Belirli bir frekansta ses çalar (bloklayici).
// frequency: Hertz cinsinden frekans (örn. 440 for A4).
// duration_ms: Sesin çalacağı süre (milisaniye).
// Not: Sure implementasyonu zamanlayici kesmesine (traps) veya basit bir gecikmeye dayanabilir.
void speaker_play_tone(uint16_t frequency, uint16_t duration_ms);

// Hoparlörü kapatır (sesi durdurur).
void speaker_off(void);


#endif // _SPEAKER_H