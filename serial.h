// serial.h
// Lİ-DOS Seri Port Modulu Arayuzu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: RS-232 seri port uzerinden karakter ve metin iletisimi.

#ifndef _SERIAL_H
#define _SERIAL_H

// Gerekli tureler icin basit typedef'ler
// Kernelinizde baska bir baslik dosyasinda (types.h gibi) olabilir.
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t; // Baud rate hesaplamasi icin 32-bit gerekebilir

// Yaygin Seri Port Base Adresleri
#define COM1_PORT 0x3F8
#define COM2_PORT 0x2F8
// Diger portlar (COM3=0x3E8, COM4=0x2E8) eklenebilir.

// Yaygin Baud Rate'ler icin Bolucu Degerleri (UART Referans Saati = 1.8432 MHz varsayimiyla)
// Bolucu = 1843200 / (16 * Baud Rate)
#define BAUD_DIVISOR_115200 (115200 / 115200) // 1
#define BAUD_DIVISOR_57600  (115200 / 57600)  // 2
#define BAUD_DIVISOR_38400  (115200 / 38400)  // 3
#define BAUD_DIVISOR_19200  (115200 / 19200)  // 6
#define BAUD_DIVISOR_9600   (115200 / 9600)   // 12
#define BAUD_DIVISOR_4800   (115200 / 4800)   // 24
#define BAUD_DIVISOR_2400   (115200 / 2400)   // 48

// Hat Kontrol Kayitcigi (LCR) Bit Alanlari Icin Sabitler
// Data Bitleri (Bit 0, 1)
#define SERIAL_DATA_5BIT 0x00
#define SERIAL_DATA_6BIT 0x01
#define SERIAL_DATA_7BIT 0x02
#define SERIAL_DATA_8BIT 0x03
// Stop Bitleri (Bit 2)
#define SERIAL_STOP_1BIT 0x00 // 1 stop bit (data_bits=5 ise 1.5 stop bit)
#define SERIAL_STOP_2BIT 0x04 // 2 stop bit (data_bits=5 ise 1.5 stop bit)
// Parity (Bit 3, 4, 5)
#define SERIAL_PARITY_NONE  0x00
#define SERIAL_PARITY_ODD   0x08
#define SERIAL_PARITY_EVEN  0x18
#define SERIAL_PARITY_MARK  0x28 // Bit 5 set, Bit 4 clear
#define SERIAL_PARITY_SPACE 0x38 // Bit 5 set, Bit 4 set
// Divisor Latch Access Bit (DLAB) (Bit 7) - rs_io.S tarafindan yonetilir, burada sadece bilgi.
#define SERIAL_LCR_DLAB 0x80

// Interrupt Enable Register (IER) Bit Alanlari (Polling icin genellikle 0x00)
#define SERIAL_IER_RX_DATA_AVAIL 0x01 // Data Available Interrupt
#define SERIAL_IER_TX_EMPTY      0x02 // Transmitter Empty Interrupt
// ... Diger IER bitleri (Line Status, Modem Status)

// Modem Control Register (MCR) Bit Alanlari
#define SERIAL_MCR_DTR 0x01 // Data Terminal Ready
#define SERIAL_MCR_RTS 0x02 // Request To Send
#define SERIAL_MCR_OUT1 0x04 // Kullanıcı tanımlı çıkış 1
#define SERIAL_MCR_OUT2 0x08 // Kullanıcı tanımlı çıkış 2 (Interrupt Enable için kullanılır)
#define SERIAL_MCR_LOOP 0x10 // Loopback modu

// Seri portu baslatir ve yapilandirir.
// port_base: Kullanilacak portun base adresi (orn. COM1_PORT)
// baud: Baud rate degeri (orn. 9600, 115200)
// data_bits: Veri bitleri sayisi (SERIAL_DATA_xBIT sabitlerinden)
// parity: Parity ayari (SERIAL_PARITY_x sabitlerinden)
// stop_bits: Stop bitleri sayisi (SERIAL_STOP_xBIT sabitlerinden)
// Donus degeri: 0 basari, -1 hata.
int serial_init(uint16_t port_base, uint32_t baud, uint8_t data_bits, uint8_t parity, uint8_t stop_bits);

// Belirtilen seri porta bir karakter gonderir (bloklayici).
void serial_putc(uint16_t port_base, char c);

// Belirtilen seri porta null terminated string gonderir (bloklayici).
void serial_puts(uint16_t port_base, const char *s);

// Belirtilen seri portun alici tamponunda okunacak veri olup olmadigini kontrol eder.
// Donus degeri: 1 (veri var), 0 (veri yok).
int serial_has_data(uint16_t port_base);

// Belirtilen seri porttan bir karakter okur (bloklayici - karakter gelene kadar bekler).
// Donus degeri: Okunan karakter.
char serial_getc(uint16_t port_base);

#endif // _SERIAL_H