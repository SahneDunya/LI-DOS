// main.c
// Lİ-DOS Çekirdek Ana Giriş Noktası (C Dili)
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Çekirdek modüllerini başlatmak ve sistemi baslatmak.

// Gerekli temel tureler
#include "types.h" // uint8_t, uint16_t, uint32_t, size_t gibi

// Assembly yardımcıları (cli, sti, inb, outb vb.)
#include "asm.h"      // Temel Assembly fonksiyonları

// Çekirdek log/debug çıktısı (vsprintf altındadır)
#include "printk.h"   // Kernel log/debug çıktısı

// Panik fonksiyonu
#include "panic.h"    // Panik fonksiyonu (erken kullanilabilir)

// Çekirdek modüllerinin arayüzleri (init fonksiyonları ve diğer gerekli bildirimler)
// Bağımlılık sırasına dikkat ederek include edelim:

// Temel G/Ç Sürücüleri (printk kullanmadan önce başlatılmalı)
#include "console.h"  // Temel konsol çıktısı
#include "uart.h"     // UART sürücüsü (serial altinda veya direkt)
#include "serial.h"   // Seri port (tty_io altinda veya direkt)

// Diğer Donanım Sürücüleri (G/Ç bağımsız veya temel G/Ç sonrası)
#include "speaker.h"  // PC Hoparlor surucusu
#include "lpt.h"      // Paralel Port surucusu
#include "rtc.h"      // RTC surucusu

// Kesme/Exception yönetimi
#include "traps.h"    // Kesme/Exception yönetimi

// Bellek yönetimi
#include "mm.h"       // Bellek yönetimi (heap)
// Linker script tarafından sağlanan BSS sonu sembolü (heap başlangıcı)
extern void *_bss_end;

// Disk Sürücüleri
#include "hd.h"       // Sabit disk sürücüsü
#include "fdc.h"      // Disket sürücüsü

// Dosya sistemi (Disk sürücülerine bağımlı)
#include "fs.h"       // Dosya sistemi

// Paket yöneticisi (Dosya sistemine bağımlı)
#include "pkg.h"      // Paket yöneticisi

// TTY katmanı (Console ve/veya Serial'a bağımlı)
#include "tty_io.h"   // TTY katmanı

// Görev zamanlayıcı (Bellek yönetimine bağımlı)
#include "sched.h"    // Görev zamanlayıcı

// Kabuk (TTY ve Dosya sistemine bağımlı, genellikle bir görev olarak başlatılır)
#include "shell.h"    // Kabuk ana fonksiyonu (shell_main)

// Genel sistem fonksiyonları (bu dosya sys_init'i implemente eder)
// #include "sys.h" // sys_init bu dosyanin kendisi

// --- Çekirdek Ana Giriş Fonksiyonu ---
// head.S tarafından çağrılır.
void c_main(void) {
    // NOT: head.S Real Mode ortamını (segmentler, stack) ve BSS'i sıfırlamayı yapmıştır.
    // Kesmeler şu anda devre dışı (head.S veya boot.S cli yapmıştır).

    // --- 1. Temel G/Ç Modüllerini Başlat ---
    // Console çıktısını başlat (ekrana yazı yazabilmek için ilk adım)
    console_init();
    // printk artık kullanılabilir olmalı (console'a yazacak şekilde ayarlandıysa)
    printk("LI-DOS Kernel baslatiliyor...\r\n");

    // UART/Serial portunu başlat (eğer console'dan ayrı veya tty_io tarafından kullanılacaksa)
    // COM1'i 115200 baud, 8 veri, No Parity, 1 Stop bit olarak başlat (örnek)
    if (uart_init_port(COM1_PORT, 115200, UART_LCR_DATA_8BIT, UART_LCR_PARITY_NONE, UART_LCR_STOP_1BIT) != 0) {
        // UART başlatma hatası panik nedeni olabilir veya alternatif bir porta geçilir.
        printk("UART Error: COM1 baslatilamadi!\r\n");
        // panic("UART Init Failed"); // Hata kritikse panik
    } else {
        printk("UART: COM1 baslatildi.\r\n");
         // Serial port modülünü başlat (UART'ı kullanıyorsa)
        serial_init(COM1_PORT); // Serial modulu COM1 portunu kullanacak sekilde init ediliyor
        printk("Serial: COM1 modulu baslatildi.\r\n");
    }


    // Diğer temel sürücüleri başlat (G/Ç bağımsız veya temel G/Ç sonrası)
    speaker_init();
    lpt_init(LPT1_PORT); // LPT1'i başlat (örnek)
    rtc_init();


    // --- 2. Kesme Yönetimini Başlat ---
    // PIC'i yapılandır, IDT'yi kur, kesme işleyicilerini ayarla.
    traps_init(); // Bu fonksiyon printk kullanabilir.
    printk("Traps: Kesme yönetimi baslatildi.\r\n");


    // --- 3. Bellek Yönetimini Başlat ---
    // Heap havuzu BSS'in bitiminden RAM'in sonuna kadardır (0x1000:0x0000'dan 0x1000:0xFFFF'e kadar olan 64KB segment içinde).
    // _bss_end linker tarafından sağlanan BSS'in bitiş adresidir (offset).
    // Bellek havuzunun başlangıcı _bss_end adresinden başlar.
    // Boyutu = 0xFFFF - _bss_end + 1'dir.
    uint16_t mem_pool_start_offset = (uint16_t)(size_t)_bss_end; // Pointer to offset cast
    size_t mem_pool_size = KERNEL_STACK_TOP_OFFSET - mem_pool_start_offset + 1; // Stackin en üstüne kadar


    // mm_init heap havuzunun başlangıç adresini (pointer) ve boyutunu bekler.
    // Heap başlangıç adresi = Kernel segmenti (0x1000) : BSS sonu offseti (_bss_end)
    // C'de bu adresi bir pointer olarak geçirmek için dikkatli olunmalı.
    // Bir pointer (segment:offset veya flat 32-bit) olarak temsil edilmesi gerekir.
    // 16-bit Real Mode C'de pointerlar derleyiciye bağlıdır.
    // Eğer derleyici near pointerları 16-bit offset, far pointerları segment:offset (32-bit) yapıyorsa,
    // burada near pointer yeterli olabilir eğer tüm kernel aynı segmentte ise.
    // Far pointer kullanmak daha güvenlidir: MK_FP(segment, offset) gibi bir makro veya manuel 32-bit pointer oluşturma.
     uint32_t mem_pool_start_ptr = ((uint32_t)KERNEL_LOAD_SEGMENT << 16) | (uint32_t)_bss_end;
    // mm_init'in void * parametresinin ne beklediğine bağlı.
     mm_init(void *mem_pool_start, size_t mem_pool_size);
    // Eğer mm_init Real Mode far pointer (seg:off) bekliyorsa:
     mm_init((void *)(((uint32_t)KERNEL_LOAD_SEGMENT << 16) | (uint32_t)_bss_end), mem_pool_size);
    // Eğer mm_init sadece offset bekliyor ve kernel segmentini varsayıyorsa:
    mm_init((void *)(size_t)_bss_end, mem_pool_size); // Basitlik icin sadece offset gonderelim

    printk("MM Init: Bellek yonetimi baslatildi. Heap @ %p, Boyut %u\r\n", (void *)(size_t)_bss_end, mem_pool_size);


    // --- 4. TTY Katmanını Başlat ---
    // Console ve/veya Serial'i kullanarak TTY cihazlarını yapılandırır.
    tty_io_init(); // Bu fonksiyon console_init ve serial_init'in cagrilmis olmasini bekler.
    printk("TTY: TTY katmani baslatildi.\r\n");


    // --- 5. Disk Sürücülerini Başlat ---
    // Hard disk ve Disket sürücülerini başlat.
    // Bu sürücüler BIOS int 13h kullanıyorsa fdc.c'deki gibi init argümanı (drive id) alabilir.
     hd_init(); // hd.c init fonksiyonu drive id alabilir
     fdc_init(); // fdc.c init fonksiyonu drive id alabilir
    // Şimdilik init argümanı almadıklarını varsayalım veya varsayılanları kullanırlar.
    if (hd_init() != 0) { // hd_init default hard diski (0x80) baslatabilir.
         printk("HD Error: Sabit disk surucusu baslatilamadi!\r\n");
         // panic("HD Init Failed"); // Kritik hata
    } else {
         printk("HD: Sabit disk surucusu baslatildi.\r\n");
    }

    if (fdc_init() != 0) { // fdc_init default disket suruculerini (0x00, 0x01) baslatabilir.
         printk("FDC Error: Disket surucusu baslatilamadi!\r\n");
         // Disket kritik degilse panik yapmayiz.
    } else {
         printk("FDC: Disket surucusu baslatildi.\r\n");
    }


    // --- 6. Dosya Sistemini Başlat ---
    // Dosya sistemini bir sürücüye monte et. Hedef sabit diski (0x80) monte edelim.
    // fs_init mount edilecek sürücü ID'sini argüman olarak alır.
    if (fs_init(0x80) != 0) {
         printk("FS Error: Dosya sistemi baslatilamadi (surucu 0x80)!\r\n");
         panic("FS Init Failed"); // Dosya sistemi kritik bir bileşen
    } else {
         printk("FS: Dosya sistemi baslatildi (surucu 0x80 monte edildi).\r\n");
    }


    // --- 7. Paket Yöneticisini Başlat ---
    // Paket yöneticisi veritabanını kontrol eder/oluşturur. Dosya sistemine bağımlıdır.
    pkg_init(); // Başarısız olursa içinde hata basar.
    printk("PKG: Paket yoneticisi baslatildi.\r\n");


    // --- 8. Kesmeleri Etkinleştir ---
    // PIT, Klavye, Seri Port gibi kesmelerin işlenmesi için genel kesme bayrağını set et.
    sti(); // Enable Interrupts
    printk("Kernel kesmeleri etkinlestirildi.\r\n");


    // --- 9. Görev Zamanlayıcıyı Başlat ve İlk Görevleri Oluştur ---
    // Zamanlayıcıyı başlat. mm'e bağımlıdır.
    sched_init(); // scheduler init edilir, ilk gorev olusturulmaz.
    printk("Sched: Görev zamanlayici baslatildi.\r\n");

    // Kabuk görevini oluştur. shell_main fonksiyonu yeni bir görev olarak çalışacak.
    // Kabuk stack'i için bellek tahsis et (mm_alloc_stack veya mm_alloc kullanilabilir)
    // Stack boyutu SHELL_STACK_SIZE gibi bir sabitle belirlenmeli.
    // Bu ornekte basitlik icin sabit stack adresi veya mm_alloc cagrisi yapmiyoruz.
    // sched_create_task fonksiyonu, gorev stacki icin bellek tahsis etmeyi kendi icinde yapabilir
    // veya buraya tahsis edilmis stack pointeri gecirilir.
    // Varsayalim ki sched_create_task stacki kendi tahsis ediyor ve shell_main fonksiyon pointerini aliyor.

    // Kabuk stack boyutu (shell_main fonksiyonunun ve cagiracagi fonksiyonlarin ihtiyacina göre)
    #define SHELL_TASK_STACK_SIZE 2048 // Örnek stack boyutu (2KB)

    if (sched_create_task(shell_main, SHELL_TASK_STACK_SIZE) == 0) { // shell_main fonksiyonuna pointer
         printk("Sched: Kabuk gorevi olusturuldu.\r\n");
    } else {
         printk("Sched Error: Kabuk gorevi olusturulamadi!\r\n");
         // panic("Shell Task Creation Failed"); // Kritik hata
    }

    // Başka başlangıç görevleri burada oluşturulabilir.

    // --- 10. Çoklu Görev Ortamını Başlat ---
    // Zamanlayıcıyı çalıştırmaya başla. Bu çağrı normalde geri dönmez.
    printk("Sched: Zamanlayici calistiriliyor...\r\n");
    sched_start();

    // Buraya hiçbir zaman ulaşılmamalıdır. Eğer scheduler dönerse, bir hata var demektir.
    panic("Scheduler returned unexpectedly!");

    // Sonsuz döngü (eğer scheduler dönseydi sistem kilitlenirdi)
     while(1); // sched_start dönmüyorsa bu gereksiz
}


// main.c sonu