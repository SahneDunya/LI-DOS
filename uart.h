// uart.h
// Lİ-DOS UART Sürücüsü Modulu Arayuzu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: UART (8250/16550) cipi ile dogrudan etkilesim (polling).

#ifndef _UART_H
#define _UART_H

#include "types.h" // uint8_t, uint16_t, uint32_t gibi
#include "asm.h"   // inb, outb fonksiyonlari icin

// UART Port Base Adresleri (Genellikle COM1-COM4)
#define COM1_PORT 0x3F8
#define COM2_PORT 0x2F8
#define COM3_PORT 0x3E8 // Daha yeni PC'lerde bulunabilir
#define COM4_PORT 0x2E8 // Daha yeni PC'lerde bulunabilir

// UART Kayıtçı Offsetleri (Base Port'a Göre)
#define UART_RBR  0x00 // Receive Buffer Register (Read Only)
#define UART_THR  0x00 // Transmit Holding Register (Write Only)
#define UART_IER  0x01 // Interrupt Enable Register (R/W)
#define UART_IIR  0x02 // Interrupt Identification Register (Read Only)
#define UART_FCR  0x02 // FIFO Control Register (Write Only)
#define UART_LCR  0x03 // Line Control Register (R/W)
#define UART_MCR  0x04 // Modem Control Register (R/W)
#define UART_LSR  0x05 // Line Status Register (Read Only)
#define UART_MSR  0x06 // Modem Status Register (Read Only)
#define UART_SCR  0x07 // Scratchpad Register (R/W)

// Divisor Latch Register (DLAB set iken)
#define UART_DLL  0x00 // Divisor Latch Low (DLAB=1)
#define UART_DLM  0x01 // Divisor Latch High (DLAB=1)

// UART Kayıtçı Bit Tanımları

// Line Control Register (LCR)
#define UART_LCR_DLAB     0x80 // Divisor Latch Access Bit
#define UART_LCR_DATA_5BIT 0x00 // 5 Data bits
#define UART_LCR_DATA_6BIT 0x01 // 6 Data bits
#define UART_LCR_DATA_7BIT 0x02 // 7 Data bits
#define UART_LCR_DATA_8BIT 0x03 // 8 Data bits
#define UART_LCR_STOP_1BIT 0x00 // 1 Stop bit (5 data bits ise 1.5)
#define UART_LCR_STOP_2BIT 0x04 // 2 Stop bits (5 data bits ise 1.5)
#define UART_LCR_PARITY_NONE 0x00
#define UART_LCR_PARITY_ODD  0x08
#define UART_LCR_PARITY_EVEN 0x18
#define UART_LCR_PARITY_MARK 0x28
#define UART_LCR_PARITY_SPACE 0x38

// Line Status Register (LSR)
#define UART_LSR_DR       0x01 // Data Ready (Rx bufferda okunacak veri var)
#define UART_LSR_OE       0x02 // Overrun Error
#define UART_LSR_PE       0x04 // Parity Error
#define UART_LSR_FE       0x08 // Framing Error
#define UART_LSR_BI       0x10 // Break Interrupt
#define UART_LSR_THRE     0x20 // Transmitter Holding Register Empty (Gönderilecek veri yazılabilir)
#define UART_LSR_TEMT     0x40 // Transmitter Empty (Tüm veri gönderildi)
#define UART_LSR_FIFOE    0x80 // Error in Rx FIFO

// Interrupt Enable Register (IER) - Polling için genellikle 0x00
#define UART_IER_RX_DATA_AVAIL 0x01 // Data Available Interrupt Enable
#define UART_IER_THRE_EMPTY    0x02 // THR Empty Interrupt Enable
#define UART_IER_RX_LINE_STATUS 0x04 // Receiver Line Status Interrupt Enable
#define UART_IER_MODEM_STATUS   0x08 // Modem Status Interrupt Enable

// Modem Control Register (MCR)
#define UART_MCR_DTR      0x01 // Data Terminal Ready
#define UART_MCR_RTS      0x02 // Request To Send
#define UART_MCR_OUT1     0x04 // User-defined Output 1
#define UART_MCR_OUT2     0x08 // User-defined Output 2 (Genellikle IRQ Enable için)
#define UART_MCR_LOOP     0x10 // Loopback Mode

// UART Referans Saati (Hz) - Baud Rate hesaplamasi icin
#define UART_BASE_CLOCK 1843200 // Hz (1.8432 MHz)

// Baud Rate Hesaplama (Divisor = UART_BASE_CLOCK / (16 * Baud))
#define UART_BAUD_TO_DIVISOR(baud) (UART_BASE_CLOCK / (16 * (baud)))


// Belirtilen UART portunu baslatir ve yapilandirir.
// port_base: Kullanilacak portun base adresi (örn. COM1_PORT).
// baud: Baud rate degeri (örn. 9600, 115200).
// data_bits: Veri bitleri sayisi (UART_LCR_DATA_xBIT sabitlerinden).
// parity: Parity ayari (UART_LCR_PARITY_x sabitlerinden).
// stop_bits: Stop bitleri sayisi (UART_LCR_STOP_xBIT sabitlerinden).
// Donus degeri: 0 basari, -1 hata.
int uart_init_port(uint16_t port_base, uint32_t baud, uint8_t data_bits, uint8_t parity, uint8_t stop_bits);

// Belirtilen UART portuna tek byte gonderir (THR bosalana kadar bloklar).
// port_base: Kullanilacak portun base adresi.
// data: Gonderilecek byte.
void uart_putc(uint16_t port_base, uint8_t data);

// Belirtilen UART portundan okunacak veri olup olmadigini kontrol eder.
// port_base: Kullanilacak portun base adresi.
// Donus degeri: 1 (veri var), 0 (veri yok), <0 (hata).
int uart_has_data(uint16_t port_base);

// Belirtilen UART portundan tek byte okur (bloklamaz).
// port_base: Kullanilacak portun base adresi.
// Donus degeri: Okunan byte (0-255) veya -1 (veri yok veya hata).
int uart_getc_nonblock(uint16_t port_base);

// Belirtilen UART portundan tek byte okur (veri gelene kadar bloklar).
// port_base: Kullanilacak portun base adresi.
// Donus degeri: Okunan byte veya -1 (hata).
int uart_getc_block(uint16_t port_base);


#endif // _UART_H