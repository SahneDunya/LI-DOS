// traps.c
// Lİ-DOS Kesme ve Tuzak Modulu Implementasyonu
// Yazar: Gemini (Orenk Kod)
// Hedef: 16-bit Real Mode, Intel 8086+, C89

#include "traps.h" // Tuzak arayuzu ve sabitler
// Kernel cikti modulu (hata mesajlari icin)
#include "printk.h"
// Dusuk seviye Assembly fonksiyonlari (CLI, STI, outb vb.)
#include "asm.h" // Ornek: cli, sti, outb fonksiyonlari burada

// --- Assembly Kesme Giris Stublari ---
// Bu fonksiyonlar C'de tanimlanir ama implementasyonlari Assembly'dedir (traps_asm.S veya asm.S).
// Her bir vektor veya vektor araligi icin bir Assembly stub olmalidir.
// Ornek:
/*
.code16
.global exception_stub_0
.global exception_stub_1
// ... exception_stub_31
.global irq_stub_0    ; Mapped to vector 0x20 after remapping
.global irq_stub_1    ; Mapped to vector 0x21
// ... irq_stub_15     ; Mapped to vector 0x2F

; Generic stub entry point macro (basit ornek)
; Bu macro, vektor numarasini stack'e iter, registerlari kaydeder ve C isleyicisini cagirir.
; Kullanimi: INTERRUPT_STUB 0x00
.macro INTERRUPT_STUB vector_no
    push_all_registers ; Ozel macro veya tek tek push AX,CX,DX,BX,SP,BP,SI,DI,ES,DS
    pushw \vector_no   ; Kesme numarasini stack'e it

    call c_interrupt_handler ; C isleyicisini cagir

    add sp, 2          ; Stack'ten kesme numarasini temizle
    pop_all_registers  ; Kaydedilen registerlari geri yukle (iret oncesi FLAGS,CS,IP hariç)

    iret               ; Kesmeden don
.endm

; Gercek stublar bu macro ile olusturulur (veya manuel yazilir)
; exception_stub_0: INTERRUPT_STUB 0x00
; exception_stub_1: INTERRUPT_STUB 0x01
; ...
; irq_stub_0: INTERRUPT_STUB 0x20 ; Vektor 0x20'ye eslenmis IRQ 0
; irq_stub_1: INTERRUPT_STUB 0x21 ; Vektor 0x21'ye eslenmis IRQ 1
; ...
*/
// Ornek kodda Assembly stub'larinin var oldugunu ve c_interrupt_handler'i
// cagirdiklarini varsayiyoruz.

// --- C Kesme İşleyicisi ---
// Tum kesme ve istisnalar icin cagrilan ana C isleyici fonksiyonu.
// Assembly stub'lar tarafindan cagirilir.
void c_interrupt_handler(unsigned int interrupt_no) {
    // !!! BURASI KESME ISLEYICISI ICIDIR. DIKKATLI KOD YAZILMALIDIR. !!!
    // printk gibi fonksiyonlarin interrupt-safe olmasi gerekir.
    // Float point, uzun hesaplamalar, bloklayici islevler genellikle yasaktir.

    // Kesme/Istisna numarasina gore islem yap
    switch (interrupt_no) {
        // --- CPU İstisnaları (0-31) ---
        case DIVIDE_BY_ZERO_VEC:
            printk("KERNEL PANIC: Divide by Zero Exception!\n");
            // panic("Divide by Zero at CS:IP...\n"); // Daha fazla bilgiyle panic cagrilabilir
            for(;;); // Hata durumunda dur
        case INVALID_OPCODE_VEC:
            printk("KERNEL PANIC: Invalid Opcode Exception!\n");
            for(;;); // Hata durumunda dur
        // ... Diger istisnalar icin case'ler eklenebilir
        // Varsayilan istisna isleyicisi
        default:
             if (interrupt_no < PIC_REMAP_OFFSET) { // CPU istisnalari genellikle 0-31 arasindadir
                 printk("KERNEL PANIC: Unhandled Exception 0x%x!\n", interrupt_no);
                 // panic("Unhandled Exception 0x%x at CS:IP...\n", interrupt_no);
                 for(;;); // Hata durumunda dur
             }
            break; // IRQ'lar icin asagiya devam et
    }

    // --- Donanim Kesmeleri (IRQs) (PIC_REMAP_OFFSET ve ustu) ---
    if (interrupt_no >= PIC_REMAP_OFFSET) {
        unsigned int irq = interrupt_no - PIC_REMAP_OFFSET; // IRQ numarasini hesapla (0-15)

        // Belirli IRQ'lar icin ozel isleyicilere yonlendir
        switch (irq) {
            case 0: Timer IRQ
                // printk("Timer tick!\n"); // Cok sik cikti yapmamak lazim
                // Zamanlayici modülünün C isleyicisini çağır (varsa)
                // timer_irq_handler_c(); // sched.c'ye zamanlama mantigini bildir
                break;
            case 1: Keyboard IRQ
                // printk("Keyboard IRQ!\n");
                // Klavye modülünün C isleyicisini çağır (varsa)
                // scan code okuma, buffer doldurma vb. burada veya buradan cagrilan fonksiyonda yapilir.
                 keyboard_irq_handler_c();
                break;
            case 3: COM2 IRQ
                 serial_irq_handler_c(COM2_PORT); // Seri port modülünün alıcı/gönderici isleyicisini çağır
                break;
            case 4: COM1 IRQ
                // serial_irq_handler_c(COM1_PORT); // Seri port modülünün alıcı/gönderici isleyicisini çağır
                break;
            // ... Diger IRQ'lar (Disk, vb.) icin case'ler eklenebilir

            default:
                 Islenmeyen IRQ
                 printk("Unhandled IRQ %d (Vector 0x%x)\n", irq, interrupt_no);
                break;
        }

        // !!! PIC'e İşlemin Bittiğini Bildir (EOI - End Of Interrupt) !!!
        // Bu, PIC'in ayni IRQ seviyesinden baska kesmeleri veya daha dusuk seviyeli kesmeleri
        // islemesine olanak tanir. UNUTULMAMALIDIR.
        // Master PIC (IRQ 0-7) icin.
        outb(0x20, 0x20); // Master PIC Command Port

        // Eger IRQ 8-15 (Slave PIC) ise, Slave PIC'e de EOI gonderilmeli.
        if (irq >= 8) {
            outb(0xA0, 0x20); // Slave PIC Command Port
        }
    }

    // C isleyicisinden donus. Assembly stub'i return'u yakalar, registerlari geri yukler ve iret ile cikar.
}


// Tuzak Modulunun Baslatilmasi
// Kesme Vektor Tablosunu (IVT) ayarlar ve PIC'i remapping yapar.
void traps_init(void) {
    int i;
    // IVT 0x0000 segmentindedir
    uint16_t ivt_segment = 0x0000;
    uint16_t *ivt_ptr = (uint16_t *)((uint32_t)ivt_segment << 4); // 0x0000:0x0000 adresi

    cli(); // IVT ve PIC ayarlari sirasinda kesmeleri devre disi birak

    // --- PIC'i Yeniden Programlama (Remapping) ---
    // Bu kisim keyboard.S ornegindeki PIC remapping ile aynidir.
    // Burada merkezi olarak yapilmasi daha uygundur.
    // Yaygin 8259A remapping ornegi (IRQs 0-7 -> Vektor 0x20-0x27, IRQs 8-15 -> Vektor 0x28-0x2F)

    // Master PIC (0x20, 0x21)
    outb(0x20, 0x11); // ICW1: Start init, ICW4 needed
    outb(0x21, PIC_REMAP_OFFSET); // ICW2: Master PIC vector offset (0x20)
    outb(0x21, 0x04); // ICW3: Tell Master PIC about Slave PIC (connected to IRQ 2)
    outb(0x21, 0x01); // ICW4: 80x86 mode

    // Slave PIC (0xA0, 0xA1) - Eger Slave PIC kullaniliyorsa
    outb(0xA0, 0x11); // ICW1: Start init, ICW4 needed
    outb(0xA1, PIC_REMAP_OFFSET + 8); // ICW2: Slave PIC vector offset (0x28)
    outb(0xA1, 0x02); // ICW3: Tell Slave PIC its cascade identity (connected to Master's IRQ 2)
    outb(0xA1, 0x01); // ICW4: 80x86 mode

    // Varsayilan olarak tum IRQ'lari maskele (kapat)
    outb(0x21, 0xFF); // Master PIC mask register (OCW1) - Tum IRQ'lari kapat
    outb(0xA1, 0xFF); // Slave PIC mask register (OCW1) - Tum IRQ'lari kapat

    // --- IVT'yi Guncelleme ---
    // Tum IVT girdilerini (0'dan 255'e kadar) kendi Assembly stub'larimizin adresleriyle doldur.
    // Her vektor icin ayri stub olmalidir (exception_stub_0, ..., irq_stub_0, ...).
    // Bu ornekte, her vektorun kendi stub'i oldugunu ve bu stub'larin
    // c_interrupt_handler'i cagiracak sekilde ayarlandigini varsayiyoruz.
    // Her stub'in adresi 'vector_stub_func_name' olarak biliniyor olsun.

    // Ornek: Vektor 0x00'dan 0x2F'e kadar olan girdileri ayarla.
    // Tum vektorleri ayarlamak daha guvenlidir (0-255).
    for (i = 0; i <= 0xFF; i++) { // Tüm 256 vektör
         // Her vektor icin ilgili Assembly stub'inin segment ve offset adresini al
         // Burasi, Assembly stub'larinin nasil adlandirildigina baglidir (ornegin exception_stub_0, irq_stub_0 vb.)
         uint16_t stub_segment = 0; // Stub'larin segmenti (ornegin kernelin kod segmenti)
         uint16_t stub_offset = 0; // Stub'larin offseti

         // Gercek kodda burasi, her i degeri icin dogru stub adresini bulma mantigini icermelidir.
         // Basit ornek: Sadece birkac bilinen vektor icin atama yapalim.
         if (i == DIVIDE_BY_ZERO_VEC) { stub_segment = seg(exception_stub_0); stub_offset = offset(exception_stub_0); }
         else if (i == INVALID_OPCODE_VEC) { stub_segment = seg(exception_stub_6); stub_offset = offset(exception_stub_6); }
         // ... Diger istisna stublari
         else if (i == TIMER_IRQ_VEC) { stub_segment = seg(irq_stub_0); stub_offset = offset(irq_stub_0); }
         else if (i == KEYBOARD_IRQ_VEC) { stub_segment = seg(irq_stub_1); stub_offset = offset(irq_stub_1); }
         // ... Diger IRQ stublari
         else {
              // Islenmeyen veya tanimlanmamis vektorler icin bos veya varsayilan bir handler (opsiyonel)
              // Veya mevcut BIOS handler'larini birakabilirsiniz.
              continue; // Bu ornekte sadece bilinenleri ayarliyoruz.
         }


         // IVT girdisine segment ve offseti yaz
         // IVT adresi: 0x0000:i*4
         ivt_ptr[i*2] = stub_offset; // Offset (16-bit)
         ivt_ptr[i*2 + 1] = stub_segment; // Segment (16-bit)
    }

    // --- Belirli IRQ Maskelerini Kaldir (Etkinlestir) ---
    // Kullanilacak donanim kesmelerini (örn. Timer, Klavye, Seri Port) PIC mask registerinda etkinlestir.
    // IRQ maskesi 0 = acik, 1 = kapali.
    // Maske degeri, ilgili IRQ bitlerinin set oldugu bir maskedir.
     outb(0x21, 0xFE); // Master PIC: Sadece IRQ 0 (Timer) acik (maske 1111 1110b)
     outb(0x21, 0xFD); // Master PIC: Sadece IRQ 1 (Keyboard) acik (maske 1111 1101b)
     outb(0x21, 0xF7); // Master PIC: Sadece IRQ 3 (COM2) acik (maske 1111 0111b)
     outb(0x21, 0xEF); // Master PIC: Sadece IRQ 4 (COM1) acik (maske 1110 1111b)

    // Genellikle kullanilacak IRQ'larin maskeleri ayri ayri temizlenir.
    uint8_t master_mask = 0xFF; // Tum master IRQ'lari kapali basla
    uint8_t slave_mask = 0xFF;  // Tum slave IRQ'lari kapali basla

    // Kullanilacak IRQ'lari ac (maskeyi temizle - ilgili bit 0 olsun)
    master_mask &= ~(1 << 0); // IRQ 0 (Timer) ac
    master_mask &= ~(1 << 1); // IRQ 1 (Keyboard) ac
    master_mask &= ~(1 << 3); // IRQ 3 (COM2) ac
    master_mask &= ~(1 << 4); // IRQ 4 (COM1) ac
    master_mask &= ~(1 << 2); // IRQ 2 (Cascade - Slave PIC) ac --> Slave kullaniliyorsa zorunlu!

    // Slave IRQ'lar (8-15)
    // slave_mask &= ~(1 << (8-8)); // IRQ 8 ac (eger kullaniliyorsa)

    outb(0x21, master_mask); // Master PIC maskesini yaz
    outb(0xA1, slave_mask);  // Slave PIC maskesini yaz (eger varsa)


    sti(); // Kesmeleri tekrar etkinlestir (kurulum tamamlandi)

    // printk("Traps and Interrupts initialized.\n");
}

// traps.c sonu