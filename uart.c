// uart.c
// Lİ-DOS UART Sürücüsü Implementasyonu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: UART (8250/16550) cipi ile dogrudan etkilesim (polling).

#include "uart.h"
#include "printk.h" // Debug cikti icin
// Sure implementasyonu, schedule veya hata isleme icin
#include "sys.h"
#include "sched.h" // schedule() icin

// Belirtilen UART portunu baslatir ve yapilandirir.
int uart_init_port(uint16_t port_base, uint32_t baud, uint8_t data_bits, uint8_t parity, uint8_t stop_bits) {
    uint32_t divisor_calc;
    uint16_t baud_divisor;
    uint8_t lcr_value;
    uint8_t mcr_value = UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT2; // DTR, RTS ve OUT2 set (OUT2 genellikle IRQ etkinlestirme icin kullanilir)
    uint8_t ier_value = 0x00; // Polling icin kesmeleri devre disi birak
    uint8_t fcr_value = 0x00; // FIFO'lari devre disi birak (16550+ olsa 0x01 yapilirdi)

    // Baud rate bolucusunu hesapla.
    if (baud == 0) return -1;
    divisor_calc = UART_BAUD_TO_DIVISOR(baud);

    if (divisor_calc == 0 || divisor_calc > 0xFFFF) {
        printk("UART Init Error: Gecersiz baud rate hesaplamasi %lu.\r\n", divisor_calc);
        return -1;
    }
    baud_divisor = (uint16_t)divisor_calc;

    // --- UART Yapilandirma Adimlari ---

    // 1. Kesmeleri devre disi birak
    outb(port_base + UART_IER, 0x00);

    // 2. DLAB bitini set ederek Bolucu Kayitciklarina erisimi etkinlestir
    lcr_value = data_bits | stop_bits | parity; // Geri kalan LCR bitleri
    outb(port_base + UART_LCR, lcr_value | UART_LCR_DLAB);

    // 3. Bolucuyu yaz
    outb(port_base + UART_DLL, (uint8_t)(baud_divisor & 0xFF)); // Dusuk byte
    outb(port_base + UART_DLM, (uint8_t)(baud_divisor >> 8)); // Yuksek byte

    // 4. DLAB bitini temizleyerek Hat Kontrol ve Veri Kayitciklarina geri don
    outb(port_base + UART_LCR, lcr_value);

    // 5. FIFO'lari yapilandir ve temizle (varsa 16550+)
    // Bu ornekte FIFO'lar devre disi birakiliyor (0x00).
    // FIFO etkinlestirmek icin 0x01 (Enable FIFO) veya 0xC7 (Enable, Clear, 14-byte esik) gibi degerler kullanilir.
    outb(port_base + UART_FCR, fcr_value); // FIFO Control Register (Write Only)

    // 6. Modem Kontrol Kayitcigi'ni ayarla
    outb(port_base + UART_MCR, mcr_value);

    // 7. Herhangi bekleyen kesmeleri temizlemek icin IIR'yi oku
    inb(port_base + UART_IIR);
    // LSR'yi oku
    inb(port_base + UART_LSR);
    // MSR'yi oku
    inb(port_base + UART_MSR);
    // RBR'yi oku (bufferdaki olası veriyi temizle)
    inb(port_base + UART_RBR);


    printk("UART Init OK: Port 0x%x, Baud %lu\r\n", port_base, baud);
    return 0;
}

// Belirtilen UART portuna tek byte gonderir (THR bosalana kadar bloklar).
void uart_putc(uint16_t port_base, uint8_t data) {
    // THR (Transmit Holding Register) boşalana kadar bekle (Polling)
    // LSR'nin THRE bitini kontrol et.
    while (!((inb(port_base + UART_LSR)) & UART_LSR_THRE)) {
        // Gönderici meşgul, çok kısa bekle veya schedule çağır.
         io_delay(); // asm.h'ten (varsa)
         schedule(); // sched.h'ten (varsa)
    }

    // Veriyi gönder (THR'ye yaz)
    outb(port_base + UART_THR, data);
}

// Belirtilen UART portundan okunacak veri olup olmadigini kontrol eder.
int uart_has_data(uint16_t port_base) {
    // LSR'nin DR (Data Ready) bitini kontrol et.
    return (inb(port_base + UART_LSR)) & UART_LSR_DR;
}

// Belirtilen UART portundan tek byte okur (bloklamaz).
int uart_getc_nonblock(uint16_t port_base) {
    // Veri olup olmadığını kontrol et
    if (uart_has_data(port_base)) {
        // Veri varsa, RBR'den oku ve dondur
        return inb(port_base + UART_RBR);
    }

    // Veri yoksa -1 dondur
    return -1;
}

// Belirtilen UART portundan tek byte okur (veri gelene kadar bloklar).
int uart_getc_block(uint16_t port_base) {
    int data;
    // Veri gelene kadar bekle (Polling)
    while (!uart_has_data(port_base)) {
        // Veri yok, çok kısa bekle veya schedule çağır.
         io_delay(); // asm.h'ten (varsa)
         schedule(); // sched.h'ten (varsa)
    }

    // Veri geldi, oku ve dondur
    data = uart_getc_nonblock(port_base);

    return data; // -1 gelme ihtimali teorik olarak burada olmaz (cunku has_data kontrol edildi)
}

// uart.c sonu