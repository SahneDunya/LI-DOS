// serial.c
// Lİ-DOS Seri Port Modulu Implementasyonu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89

#include "serial.h" // Seri port arayuzu ve sabitler

// rs_io.S'teki dusuk seviye Assembly fonksiyonlari bildirimleri
// Bu fonksiyonlarin rs_io.S dosyasinda implemente edildigi varsayilir.
extern void rs_init(uint16_t port_base, uint16_t baud_divisor, uint8_t lcr_value, uint8_t ier_value, uint8_t mcr_value);
extern void rs_putc(uint16_t port_base, uint8_t char_to_send);
extern uint8_t rs_has_data(uint16_t port_base);
extern uint8_t rs_getc(uint16_t port_base);


// Seri portu baslatir ve yapilandirir.
int serial_init(uint16_t port_base, uint32_t baud, uint8_t data_bits, uint8_t parity, uint8_t stop_bits) {

    uint32_t divisor_calc;
    uint16_t baud_divisor;
    uint8_t lcr_value;
    uint8_t ier_value = 0x00; // Polling icin kesmeleri devre disi birak
    uint8_t mcr_value = SERIAL_MCR_DTR | SERIAL_MCR_RTS | SERIAL_MCR_OUT2; // DTR, RTS ve OUT2 set (OUT2 genellikle interrupt etkinlestirme icin kullanilir)

    // Baud rate bolucusunu hesapla.
    // UART Referans Saati = 1.8432 MHz. Bolucu = 1843200 / (16 * baud).
    // Veya 115200 / baud.
    if (baud == 0) return -1; // 0 baud gecerli degil.

    divisor_calc = 115200 / baud;

    // Bolucu 16-bit'e sigmali
    if (divisor_calc == 0 || divisor_calc > 0xFFFF) {
        // printk("Serial init error: Invalid baud rate divisor calculation.\n");
        return -1;
    }
    baud_divisor = (uint16_t)divisor_calc;

    // Hat Kontrol Kayitcigi (LCR) degerini olustur.
    // LCR = Data Bits | Stop Bits | Parity
    lcr_value = data_bits | stop_bits | parity;

    // DLAB bitini kendimiz LCR'a eklemiyoruz, rs_init Assembly fonksiyonu
    // baud_divisor yazarken bu biti yonetiyor.

    // Dusuk seviye Assembly baslatma fonksiyonunu cagir.
    rs_init(port_base, baud_divisor, lcr_value, ier_value, mcr_value);

    // Serial portun dogru configure edilip edilmedigini kontrol etmek daha karmasik (orn. MSR okuma).
    // Basitlik icin basarili varsayalim.
    return 0;
}

// Belirtilen seri porta bir karakter gonderir (bloklayici).
void serial_putc(uint16_t port_base, char c) {
    // Dusuk seviye Assembly gonder fonksiyonunu cagir.
    rs_putc(port_base, (uint8_t)c);
}

// Belirtilen seri porta null terminated string gonderir (bloklayici).
void serial_puts(uint16_t port_base, const char *s) {
    // Null terminator karakterine gelene kadar dongu.
    while (*s != '\0') {
        // Her karakteri tek tek gonder.
        serial_putc(port_base, *s);
        // Pointeri bir sonraki karaktere ilerlet.
        s++;
    }
}

// Belirtilen seri portun alici tamponunda okunacak veri olup olmadigini kontrol eder.
// Donus degeri: 1 (veri var), 0 (veri yok).
int serial_has_data(uint16_t port_base) {
    // Dusuk seviye Assembly kontrol fonksiyonunu cagir.
    return (int)rs_has_data(port_base);
}

// Belirtilen seri porttan bir karakter okur (bloklayici).
char serial_getc(uint16_t port_base) {
    // Dusuk seviye Assembly oku fonksiyonunu cagir.
    // rs_getc veri gelene kadar Assembly'de bekler.
    return (char)rs_getc(port_base);
}

// serial.c sonu