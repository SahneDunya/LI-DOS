// sys.c
// Lİ-DOS Genel Sistem Modulu Implementasyonu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89

#include "sys.h" // Genel sistem arayuzu
// Kernelin baslatilacak diger modulleri
#include "console.h" // Konsol modulu
#include "serial.h"  // Seri port modulu
#include "hd.h"      // Sabit disk modulu
// #include "file_system.h" // Dosya sistemi modulu
#include "sched.h"   // Zamanlayici modulu
#include "mm.h"      // Bellek yonetimi modulu
#include "printk.h"  // Kernel cikti modulu
// #include "traps.h" // Kesme ve tuzak isleyicileri
// #include "keyboard.h" // Klavye modulu (IRQ kurulumu icin)

// Kernelin baslangic fonksiyonu.
// Assembly head kodu tarafindan kontrol C'ye gectiginde cagirilir.
void sys_init(void) {
    // C'de ilk islevsel kodu yürüttüğümüzü gösteren bir mesaj
    // printk henuz hazir olmayabilir, bu yuzden once en temel ciktiyi baslat.

    // 1. Temel konsol veya seri port cikti modullerini baslat
    console_init(); // Konsolu baslat (printk'ten once gerekliyse)
    // Ornegin COM1'i baslat (debug cikisi icin)
     serial_init(COM1_PORT, 115200, SERIAL_DATA_8BIT, SERIAL_PARITY_NONE, SERIAL_STOP_1BIT);

    // printk modulu vsprintf ve cikti suruculerine baglidir.
    // Cikti suruculeri (console/serial) basladiktan sonra printk hazir olabilir.
     printk("LI-DOS Kernel baslatiliyor...\n");

    // 2. Bellek Yonetimi modülünü baslat.
    // Bu modül, kernelin kullanabileceği belleği organize eder (ornegin 64KB alani).
     mm_init(); // Bellek haritasini alip yonetimi kurar.

    // 3. Kesme ve Tuzak isleyicilerini kur (IDT ayarlari).
    // Donanim kesmelerini (timer, keyboard, disk vb.) ve CPU istisnalarini yonetmek icin.
     traps_init(); // IDT kurulumu, varsayilan isleyiciler.

    // 4. Donanim suruculerini baslat.
     hd_init(); // Sabit disk surucusunu baslat.
    // // Seri portu printk kullanacaksa zaten baslatilmisti.
    // // Klavye IRQ handlerini kur (eger kesme tabanli kullanilacaksa).
     keyboard_init_irq(); // PIC remapping ve IVT guncelleme yapar.

    // 5. Dosya Sistemini baslat (disk surucusune baglidir).
     file_system_init(); // Root dosya sistemini bagla vb.

    // 6. Zamanlayiciyi baslat ve gorevleri olustur.
     sched_init(); // Idle gorev olusturur ve zamanlayiciyi hazirlar.

    // --- Sistem Basladi, Simdi Islemleri Baslat ---
    // User-space olmadigi icin, burada kernel gorevlerini olusturmaliyiz.
    // Ornegin, bir shell gorevi (eger shell ayri bir gorev olacaksa),
    // veya bir ana kontrol dongusu baslatilabilir.

    // Ornek: Bir test gorevi olusturalim (bu gorev icin stack mm_init'ten sonra tahsis edilmeli)
     #define TEST_TASK_STACK_SIZE 1024
     void test_task(void) { while(1) { printk("Test task running...\n"); schedule(); } }
     void *test_stack = mm_alloc_stack(TEST_TASK_STACK_SIZE); // mm modulu tarafindan tahsis edilir.
     if (test_stack) {
        sched_create_task(test_task, test_stack, TEST_TASK_STACK_SIZE);
     } else {
         printk("Failed to allocate stack for test task!\n");
     }


    // Zamanlayiciyi calistir. Bu noktadan sonra kontrol zamanlayicidadir
    // ve olusturulan gorevler calismaya baslar.
    // Eğer hiç görev oluşturulmadıysa veya sadece idle task varsa, idle task çalışır.
     schedule(); // Bu fonksiyon normalde geri dönmez.

    // Eger buraya ulasilirsa bir hata var demektir veya schedule() cagrilmadi.
     panic("sys_init exited unexpectedly!\n"); // panic modulu kullanilabilir
    for(;;); // Sonsuz donguye gir (sistem durdu)
}

// Sistemle ilgili temel bilgiler donduren fonksiyonlarin implementasyonu
 uint32_t get_total_memory(void) {
       // Bellek yonetimi modulu (mm) tarafindan tespit edilen veya bilinen toplam bellegi dondurur.
       return mm_get_total_memory(); // mm.c'de implemente edilebilir.
      return MAX_OS_SUPPORTED_RAM_KB * 1024; // Basit ornek: Tasarim limitini dondur
 }

// Kernelin kontrollu kapatilmasi (ornektir)
 void sys_shutdown(void) {
//     // Cikis dosya sistemlerini unmount etme, donanimi sifirlama vb. islemler
     printk("System shutting down...\n");
//     // Donanimi durdur
     cli(); // Kesmeleri kapat
     for(;;){ hlt(); } // CPU'yu durdur
 }
// sys.c sonu