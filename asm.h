// asm.h
// Lİ-DOS Assembly Yardımcı Fonksiyon Bildirimleri
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: asm.S dosyasindaki Assembly fonksiyonlarini C koduna bildirmek.

#ifndef _ASM_H
#define _ASM_H

// Kernel genelinde kullanilan temel tureler icin
// Kernelinizde baska bir baslik dosyasinda (types.h gibi) olabilir.
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t; // Gerekirse 32-bit degerler icin

// Scheduler baglam yapisi icin (context_switch fonksiyonunda kullanilir)
// sched.h dosyasindan alinir.
#include "sched.h" // struct task_context tanimi icin

// --- Port G/Ç Fonksiyonlari ---
// asm.S'te implemente edilecek port okuma/yazma fonksiyonlari.
// Port adresleri uint16_t'dir. Byte AL/AH, Word AX kullanir.

// Bir I/O portundan 1 byte okur.
extern uint8_t inb(uint16_t port);

// Bir I/O portuna 1 byte yazar.
extern void outb(uint16_t port, uint8_t value);

// Bir I/O portundan 1 word okur.
extern uint16_t inw(uint16_t port);

// Bir I/O portuna 1 word yazar.
extern void outw(uint16_t port, uint16_t value);

// Bir I/O portundan 'count' adet word okur, 'buffer_segment:buffer_offset' adresine yazar.
// Buffer adresi 16-bit segment ve offset olarak ayri ayri verilir.
extern void insw(uint16_t port, uint16_t buffer_segment, uint16_t buffer_offset, uint16_t count);

// Bir I/O portuna 'count' adet word yazar, 'buffer_segment:buffer_offset' adresinden okur.
// Buffer adresi 16-bit segment ve offset olarak ayri ayri verilir.
extern void outsw(uint16_t port, uint16_t buffer_segment, uint16_t buffer_offset, uint16_t count);

// --- Kesme Kontrol Fonksiyonlari ---
// CPU kesmelerini etkinlestirme/devre disi birakma.

// Kesmeleri devre disi birakir (CLI komutu).
extern void cli(void);

// Kesmeleri etkinlestirir (STI komutu).
extern void sti(void);

// --- CPU Kontrol Fonksiyonlari ---
// CPU'yu durdurma, bekletme vb.

// CPU'yu duraklatir (HLT komutu). Kesme gelene kadar bekler.
extern void hlt(void);

// Kisa bir G/Ç gecikmesi saglar (NOP'lar veya kisa dongu).
extern void io_delay(void);

// --- BIOS Yardimci Fonksiyonlari ---
// BIOS kesmelerini cagirmak icin Assembly sarmalayici fonksiyonlar.

// BIOS int 13h disk servisi cagrilarini yapar.
// Detayli parametreler ve donus degeri bios_disk_io Assembly implementasyonuna baglidir.
// Yaygin kullanimda AH'ta hata kodu veya AL'de transfer edilen sektor sayisi döner.
// Ornek HD modulu icin: uint8_t bios_disk_io(uint8_t command, uint8_t count, uint16_t cylinder, uint8_t head, uint8_t sector, uint16_t buffer_segment, uint16_t buffer_offset, uint8_t drive);
extern uint8_t bios_disk_io(uint8_t command, uint8_t count, uint16_t cylinder, uint8_t head, uint8_t sector, uint16_t buffer_segment, uint16_t buffer_offset, uint8_t drive);


// --- Zamanlayici Baglam Degisim Fonksiyonu ---
// Scheduler tarafindan gorevler arasi gecis yapmak icin kullanilir.

// Eski gorevin baglamini old_ctx'e kaydeder ve yeni gorevin baglamini new_ctx'ten yukleyerek atlar.
// Bu fonksiyon cagrildiginda, normal C fonksiyonlari gibi geri DONMEZ.
extern void context_switch(struct task_context *old_ctx, struct task_context *new_ctx);

// --- Tuzak/Kesme Giris Stublari (Opsiyonel Bildirimler) ---
// Bu stublar dogrudan C tarafindan cagrilmaz, CPU veya baska Assembly kodu atlar.
// Ancak adreslerini almak icin bildirilmeleri gerekebilir (IVT ayarlari gibi).
// Eger traps.c Assembly stub adreslerini direk aliyorsa extern bildirimleri buraya gelebilir.
// Ornek:
 extern void exception_stub_0(void); // Vektor 0 icin stub
 extern void irq_stub_0(void);     // Vektor 0x20 (IRQ 0) icin stub
// ... Diger stublar ...


#endif // _ASM_H