// panic.c
// Lİ-DOS Kernel Panik Modulu Implementasyonu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89

#include "panic.h"   // Panik arayuzu
#include "printk.h"  // Kernel cikti fonksiyonu (vsprintf kullanir)
// Assembly yardimci fonksiyonlari icin (ornegin cli, hlt)
// Bu fonksiyonlarin asm.h gibi bir baslikta bildirildigini varsayalim.
extern void cli(void); // Kesmeleri kapat
extern void hlt(void); // CPU'yu durdur

// Kernel panic durumunda cagirilir.
// Hata mesajini yazar ve sistemi durdurur.
// Bu fonksiyon asla geri donmez.
void panic(const char *fmt, ...) {
    // Kesmeleri hemen devre disi birak, boylece baska bir kesme
    // panik sirasinda sistemi daha da karistirmasin.
    cli();

    // Konsola veya seri porta (printk nereye yonlendirilirse) panik mesajini yazdir.
    // Oncelikle belirgin bir baslik yazdir.
    printk("KERNEL PANIC!\n");

    // Hata format stringini ve argumanlarini kullanarak hata detayini yazdir.
    // printk degisken sayida arguman alir ve vsprintf kullanir.
    // C89 standardinda degisken sayida arguman icin va_list vb. kullanimi gereklidir,
    // printk fonksiyonunun kendisi bu yapilari (ornegin custom va_list.h'tan) kullanir.
    va_list args;
    // va_start macro'su, degisken arguman listesine erisimi baslatir.
    // fmt son sabit argumandir.
    // va_list args; va_start(args, fmt); // C89 va_start kullanimi
    // Not: va_start implementasyonu derleyiciye ve mimariye baglidir.
    // Kernelin kendi va_list.h dosyasini icerdigini varsayiyoruz.

    // Custom kernel va_list kullanimi (ornegin va_list.h basligindan)
    VA_START(args, fmt); // Kendi VA_START macro'nuz veya fonksiyonunuz

    // printk fonksiyonunu cagirmak icin va_list'i kullan
    vprintk(fmt, args); // vprintk, va_list alan printk versiyonudur. printk.c'de implemente edilecek.

    // va_list'i temizle
    VA_END(args); // Kendi VA_END macro'nuz veya fonksiyonunuz

    // Sistemi durdurdugumuzu belirten bir mesaj yazdir.
    printk("System halted.\n");

    // Sistemi tamamen durdur.
    // CPU'yu durdurmak icin hlt komutu kullanılır.
    // Eğer kesmeler kapalıysa ve NMI (Non-Maskable Interrupt) gelmezse CPU durur.
    // Ek bir güvenlik önlemi olarak sonsuz döngü içine hlt koyulabilir.
    for (;;) {
        hlt(); // CPU'yu durdur
        // Eğer hlt'den bir şekilde çıkılırsa (örn. NMI), döngü devam eder.
    }

    // Bu noktaya asla erisilmemelidir.
}

// panic.c sonu