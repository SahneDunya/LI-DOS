// traps.h
// Lİ-DOS Kesme ve Tuzak Modulu Arayuzu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: CPU istisnalarini ve donanim kesmelerini (IRQs) yonetmek.

#ifndef _TRAPS_H
#define _TRAPS_H

// Gerekli temel tureler
// Kernelinizde baska bir baslik dosyasinda (types.h gibi) olabilir.
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef int int32_t;

// Yaygin CPU Istisna (Exception) Vektorleri (0-31)
#define DIVIDE_BY_ZERO_VEC 0x00 // Bolme hatasi
#define DEBUG_VEC          0x01 // Debug exception (breakpoint, single step)
#define NMI_VEC            0x02 // Non-Maskable Interrupt
#define BREAKPOINT_VEC     0x03 // Breakpoint (INT 3)
#define OVERFLOW_VEC       0x04 // Overflow (INT INTO)
#define BOUND_RANGE_EXC_VEC 0x05 // BOUND instruction range exceeded
#define INVALID_OPCODE_VEC 0x06 // Gecersiz opcode
#define DEVICE_NOT_AVAIL_VEC 0x07 // Cihaz mevcut degil (FPU hatasi)
#define DOUBLE_FAULT_VEC   0x08 // Cift hata (hata islenirken baska hata olusmasi)
// ... Diger istisnalar (Protected Mode'da daha fazlasi var, ama Real Mode'da bazilari olmaz)

// BIOS Varsayilan Donanim Kesmesi (IRQ) Vektorleri (IRQ 0-7 -> Vektor 8-15)
#define BIOS_TIMER_VEC    0x08 // IRQ 0 (Timer)
#define BIOS_KEYBOARD_VEC 0x09 // IRQ 1 (Keyboard)
#define BIOS_CASCADE_VEC  0x0A // IRQ 2 (Slave PIC)
#define BIOS_COM2_VEC     0x0B // IRQ 3 (COM2)
#define BIOS_COM1_VEC     0x0C // IRQ 4 (COM1)
// ... IRQ 5-7

// PIC Remapping Sonrasi Donanim Kesmesi (IRQ) Vektorleri (IRQ 0-15 -> Vektor 0x20-0x2F)
// Genellikle OS'ler bu sekilde yeniden haritalar.
#define PIC_REMAP_OFFSET   0x20 // IRQ'lar icin yeni vektor baslangici
#define TIMER_IRQ_VEC     (PIC_REMAP_OFFSET + 0) // IRQ 0
#define KEYBOARD_IRQ_VEC  (PIC_REMAP_OFFSET + 1) // IRQ 1
#define CASCADE_IRQ_VEC   (PIC_REMAP_OFFSET + 2) // IRQ 2
#define COM2_IRQ_VEC      (PIC_REMAP_OFFSET + 3) // IRQ 3
#define COM1_IRQ_VEC      (PIC_REMAP_OFFSET + 4) // IRQ 4
// ... IRQ 5-15

// --- Assembly Kesme Giris Stubu ---
// C isleyicisine gecmeden once registerlari kaydeden Assembly kodu.
// Her vektor icin veya bir grup vektor icin ayri stublar olabilir.
// Bu ornekte tek bir generic stub kullanildigini varsayalim ve interrupt numarasini
// stack'e push edip C isleyicisini cagirdigini varsayalim.
// Bu fonksiyonlar asm.S'te veya ayri bir traps_asm.S dosyasinda bulunmalidir.

// Assembly stub'in C isleyicisini cagirmadan once stack'e kaydettigi durumun yapisi.
// CPU otomatik olarak Flags, CS, IP'yi iter. Assembly stub digerlerini iter.
// Sira cok onemlidir!
struct trap_frame {
    uint16_t di;
    uint16_t si;
    uint16_t bp;
    uint16_t sp; // Bu noktadan sonraki stack pointeri
    uint16_t bx;
    uint16_t dx;
    uint16_t cx;
    uint16_t ax;
    uint16_t es; // Assembly stub tarafindan pushlandiysa
    uint16_t ds; // Assembly stub tarafindan pushlandiysa
    uint16_t interrupt_no; // Assembly stub tarafindan stack'e pushlandiysa
    // CPU tarafindan otomatik pushlananlar (iret tarafindan cekilenler)
    uint16_t flags;
    uint16_t cs;
    uint16_t ip;
    // Eger bazi istisnalar hata kodu iterse (Protected Mode'da yaygin),
    // stack yapisi degisir ve error_code alani eklenmelidir.
    // Real Mode'da cogunlukla hata kodu itilmez.
     uint16_t error_code; // Eger istisna error_code iterse
};


// --- C Kesme Isleme Fonksiyonlari ---

// Tum kesme ve istisnalar icin cagrilan ana C isleyici fonksiyonu.
// interrupt_no: Tetiklenen kesme/istisnanin vektor numarasi.
// stack_frame: Kesme/istisna anindaki register durumunu iceren yapiya pointer (opsiyonel, bu ornekte sadece interrupt_no alalim).
// Bu ornekte basitlik icin sadece interrupt_no aliyor.
void c_interrupt_handler(unsigned int interrupt_no);

// Belirli istisnalar veya IRQ'lar icin daha ozel C isleyicileri (opsiyonel).
// c_interrupt_handler bunlara yonlendirme yapabilir.
 void handle_divide_by_zero(void);
 void handle_keyboard_irq_c(void); // Klavye IRQ'su icin C isleyici

// --- Tuzak Modulunun Baslatilmasi ---
// Kesme Vektor Tablosunu (IVT) ayarlar ve PIC'i remapping yapar.
void traps_init(void);

#endif // _TRAPS_H