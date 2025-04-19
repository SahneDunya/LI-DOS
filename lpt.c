// lpt.c
// Lİ-DOS Paralel Port Modulu Implementasyonu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Paralel port uzerinden basit cikti (örn. yaziciya).

#include "lpt.h"
#include "printk.h" // Debug cikti icin

// --- Dahili Degiskenler ---
// Başlatılmış portların listesini tutmak için (isteğe bağlı)
 static uint16_t inited_lpt_ports[MAX_LPT_PORTS]; // MAX_LPT_PORTS tanimlanmali


int lpt_init(uint16_t port_base) {
    // Portun var olup olmadığını kontrol etmek zor olabilir.
    // Genellikle belirli portlara yazıp okuyarak donanımın yanıt verip vermediğine bakılır.
    // Basitlik icin portun gecerli adres oldugunu varsayalim.
    // Kontrol portunu sifirla (yazıcıyı hazirla)
    outb(port_base + LPT_CONTROL_PORT, 0x0C); // Init=1, SelectIn=1, digerleri 0.
    // Yaklaşık 50 mikrosaniye bekleme gerekebilir burada.
     speaker_play_tone(0, 1); // Cok kisa sureli calmayacak sekilde (mock delay)
    // Sonra Init'i 0 yap
    outb(port_base + LPT_CONTROL_PORT, 0x08); // Init=0, SelectIn=1

    printk("LPT Init: Port 0x%x baslatildi.\r\n", port_base);
    return 0;
}

// Belirtilen paralel porta tek karakter (byte) gonderir.
int lpt_putc(uint16_t port_base, uint8_t c) {
    uint8_t status;
    int timeout = 1000; // Yazicinin hazir olmasi icin basit timeout

    // Yazicinin (veya alici cihazın) meşgul (Busy) olmamasını bekle.
    // Durum portunun Bit 7'si Busy'dir (ters). 0 ise Busy degil.
    // (Busy bit bazen porttan porta degisebilir veya ters mantik calisabilir).
    // Standart: Bit 7 INVERTED Busy. Yani 0xFF & 0x80 = 0x80 -> Busy DEĞİL.
    // Loop while Busy (Status Bit 7 == 0)
    do {
        status = inb(port_base + LPT_STATUS_PORT);
        if ((status & 0x80) != 0) break; // Not Busy (Bit 7 is 1)
        // Çok kısa bekleme
        // io_delay();
        timeout--;
    } while (timeout > 0);

    if (timeout == 0) {
         // printk("LPT Putc Error: Port 0x%x mesgul (timeout).\r\n", port_base);
         return -1; // Timeout
    }

    // Veriyi veri portuna yaz
    outb(port_base + LPT_DATA_PORT, c);

    // Strobe sinyalini etkinlestir (Veri porttaki verinin gecerli oldugunu belirtir)
    // Kontrol portunun Bit 0'ı Strobe'dir (ters mantik calisabilir). Genellikle 1 yapilir.
    // Mevcut kontrol portu degerini oku, Strobe bitini ayarla, geri yaz.
    uint8_t control_status = inb(port_base + LPT_CONTROL_PORT);
    outb(port_base + LPT_CONTROL_PORT, control_status | 0x01); // Strobe set (Bit 0)

    // Kisa bir sure bekle (Strobe sinyalinin islenmesi icin)
     io_delay();

    // Strobe sinyalini devre disi birak (Bit 0'ı 0 yap)
    outb(port_base + LPT_CONTROL_PORT, control_status & ~0x01); // Strobe clear

    // Yazicidan Ack sinyalini bekleme veya baska durum kontrolleri (daha karmasik)
    // Basitlik icin bekleme yapilmiyor.

    return 0; // Basari
}

// Belirtilen paralel porta null terminated string (byte dizisi) gonderir.
void lpt_puts(uint16_t port_base, const char *s) {
    if (!s) return;
    while (*s != '\0') {
        // lpt_putc fonksiyonu hata dondurse bile devam et
        lpt_putc(port_base, (uint8_t)*s);
        s++;
    }
}

// lpt.c sonu