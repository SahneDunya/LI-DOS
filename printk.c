// printk.c
// Lİ-DOS Kernel Cikti Modulu Implementasyonu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89

#include "printk.h"  // printk arayuzu
#include "va_list.h" // Variadic arguman makrolari
// Formatlama icin vsprintf fonksiyonu bildirimi
#include "vsprintf.h" // vsprintf arayuzu
// Ciktinin gonderilecegi dusuk seviye G/Ç modulu
#include "console.h" // console modulu arayuzu (console_write kullanilacak)
// Veya serial port icin: #include "serial.h"

// printk tarafindan kullanilacak dahili buffer boyutu
// Formatlanan mesaj bu buffera yazilir.
// Mesaj bu boyutu asarsa kesilebilir.
#define PRINTK_BUFFER_SIZE 256

// Kernel icinden formatli cikti almak icin ana fonksiyon
void printk(const char *fmt, ...) {
    va_list args;

    // va_start makrosu ile degisken arguman listesini baslat
    VA_START(args, fmt); // Custom VA_START

    // vprintk fonksiyonunu cagir
    vprintk(fmt, args);

    // va_end makrosu ile arguman listesini temizle
    VA_END(args); // Custom VA_END
}

// printk'in va_list alan versiyonu
void vprintk(const char *fmt, va_list args) {
    char print_buffer[PRINTK_BUFFER_SIZE]; // Dahili formatlama bufferi
    int printed_len = 0; // vsprintf tarafindan yazilan karakter sayisi

    // vsprintf fonksiyonunu cagirarak formatli stringi buffera yaz
    // vsprintf fonksiyonunun vsprintf.c dosyasinda implemente edildigi varsayilir.
    // vsprintf, buffer'a yazilan karakter sayisini dondurur (NULL terminator haric).
    // Buffer tasmalarini vsprintf'in kendisinin yonetmesi idealdir.
    printed_len = vsprintf(print_buffer, fmt, args);

    // Yazilan karakter sayisi buffer boyutunu astiysa, isaret koyabiliriz.
    if (printed_len >= PRINTK_BUFFER_SIZE) {
        // Bufferin sonuna "[...]\n" gibi bir isaret ekleyebilirsiniz.
        // Basitlik icin bu ornekte bu kontrol atlandi veya vsprintf'in yonettigi varsayildi.
        printed_len = PRINTK_BUFFER_SIZE - 1; // buffer tasmadiysa length i kullan
                                             // tastiysa max boyutu kullan
    }
    // vsprintf zaten NULL terminator ekleyecektir. console_write NULL terminator beklemez,
    // bu yuzden printed_len kadar karakteri yazariz.

    // Formatlanan stringi dusuk seviye G/Ç modulune gonder
    // Ornekte console_write fonksiyonu kullaniliyor.
    console_write(print_buffer, printed_len);

    // Eger seri port cikisi da aktifse, oraya da gonderilebilir.
    // serial_write(print_buffer, printed_len); // Eger serial modulu varsa
}

// printk.c sonu