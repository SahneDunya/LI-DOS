// sched.c
// Lİ-DOS Görev Zamanlayıcı (Scheduler) Implementasyonu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89

#include "sched.h" // Zamanlayici arayuzu ve yapilari
#include "console.h" // Ornek cikti icin
// #include "printk.h" // Daha iyi cikti icin
// Baglam degisim Assembly fonksiyonu
extern void context_switch(struct task_context *old_ctx, struct task_context *new_ctx);

// Gorev listesi
static struct task tasks[MAX_TASKS];
// Halihazirda calisan gorev
static struct task *current_task = (struct task *)0;
// Zamanlayicinin sececegi siradaki gorev indexi
static int next_task_index = 0;
// Kac gorev olusturuldu
static int task_count = 0;

// Idle gorev fonksiyonu
void idle_task(void) {
    while (1) {
        //console_puts("."); // Idle oldugunu goster (opsiyonel)
        // printk("."); // printk kullanilabilir
        // Yapilacak baska is yoksa CPU'yu hlt ile duraklatmak enerji tasarrufu saglar.
        // hlt(); // asm.S'ten hlt fonksiyonu
        schedule(); // Kontrolu zamanlayiciya geri ver
    }
}

// Zamanlayiciyi baslatir
void sched_init(void) {
    int i;
    // Gorev listesini baslat
    for (i = 0; i < MAX_TASKS; i++) {
        tasks[i].state = TASK_STATE_UNUSED;
    }
    task_count = 0;
    next_task_index = 0;

    // En az bir gorev olmali - bir "idle" gorev olusturalim.
    // Idle gorev icin stack tahsisi gereklidir. Ornek olarak sabit bir alan kullanalim.
    // Gercek sistemde bellek yoneticisi (mm) buradan cagrilmalidir.
    // Kernelin 64KB segmenti icinde bir stack alani tahsis edin.
    // Ornek: Cekirdek kodu 0x7E00'de basliyorsa, 0x8000'den sonra stackler baslayabilir.
    // Basitlik icin, kernelin bitisinden sonra belirli bir offseti stack basi olarak alalim.
    // Kernel boyutu biliniyorsa, oradan sonra stackler ayarlanir.
    // Ornegin 4KB (4096 byte) stack kullanilacaksa.
    #define IDLE_TASK_STACK_SIZE 1024 // Ornek: 1KB stack
    // Kernelin yüklendiği segmenti bilmemiz gerekiyor (genelde CS veya DS).
    // Varsayalım ki kernel verileri ve stackler aynı segmentte (0x07E0 gibi).
    // Stackler, bu segment icinde yüksek adreslerden aşağı doğru büyür.
    // Stack alanını kernel kodunun bittiği yerden sonra ayırabiliriz.
    // Veya en basit, sabit bir yüksek adres offsetini stack başı alabiliriz.
    // Örneğin, 0x07E0 segmentinde 0xF000 offseti stack başı olsun (64KB segmentin sonuna yakın).
    // Stack büyüdükçe 0xF000'den aşağı inecektir.
    // Idle task'in stack'i 0xF000 - IDLE_TASK_STACK_SIZE ile 0xF000 arasinda olsun.
    // Stack pointer (SP) stack'in en üstüne (en yüksek adresine) ayarlanır.
    // BP'nin stack frame'i için yeterli yer bırakılmalıdır.
    #define KERNEL_DATA_SEGMENT 0x07E0 // Kernelin calistigi segment (ornektir)
    #define IDLE_STACK_TOP_OFFSET 0xF000 // Idle task stackinin en ust offseti

    // Stack alanini fiziksel olarak ayirmak gerekir (ornegin .bss'te veya manuel olarak).
    // Ornek kodda manuel tahsis gosterilmez, pointer gecerli kabul edilir.
    void *idle_stack_base = (void *)IDLE_STACK_TOP_OFFSET - IDLE_TASK_STACK_SIZE; // Dusuk adres

    // Idle gorevi olustur
    // sched_create_task(idle_task, idle_stack_base, IDLE_TASK_STACK_SIZE); // Boyle cagrilir
    // Direkt icerde olusturalim (basitlik icin)
    int idle_idx = sched_create_task(idle_task, idle_stack_base, IDLE_TASK_STACK_SIZE);
    if (idle_idx == -1) {
        // Panik! Idle gorev olusturulamadi.
        // panic("Failed to create idle task!\n"); // panic modulu kullanilabilir
        for(;;); // Sistem baslayamaz, durdur.
    }

    // Ilk calisacak gorevi ayarla (simdilik ilk olusturulan gorev)
    current_task = &tasks[idle_idx]; // Baslangicta idle task calisacak
    current_task->state = TASK_STATE_RUNNING;
    next_task_index = (idle_idx + 1) % MAX_TASKS; // Sonraki gorevi ayarla
}

// Yeni bir kernel gorevi olusturur ve zamanlayiciya ekler.
int sched_create_task(void (*task_entry)(void), void *stack_base, size_t stack_size) {
    int i;

    // Bos bir gorev yuvasi bul
    for (i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_STATE_UNUSED) {
            break; // Bos yuvayi bulduk
        }
    }

    if (i == MAX_TASKS) {
        // Bos yuva bulunamadi
        // printk("Error: Maximum tasks reached!\n"); // printk kullanilabilir
        return -1;
    }

    // Gorev yuvasi bulundu (index i)
    struct task *new_task = &tasks[i];

    // Gorev bilgilerini ayarla
    new_task->state = TASK_STATE_READY;
    new_task->stack_base = stack_base;
    new_task->stack_size = stack_size;

    // Gorev baglamini ayarla.
    // Bu, gorevin ILK KEZ calistirilacaginda context_switch'in
    // dogru yere atlamasini ve stack'in duzgun olmasini saglar.
    // context_switch'in sonundaki retf komutu, stack'in tepesinden Flags, CS, IP ceker.
    // Bu nedenle, yeni gorevin stackinin tepesini soyle hazirlamaliyiz:
    // ... | Saved Registers (dummy/initial) | Flags | CS | IP (task_entry adresi)
    // Stack pointer (SP) Flags'in uzerine ayarlanmalidir.
    uint16_t initial_stack_top = (uint16_t)((uint32_t)stack_base + stack_size); // Stack'in en ust adresi (offset)
    uint16_t *stack_ptr = (uint16_t *)initial_stack_top; // Stack pointer (uint16_t pointer olarak)

    // Dummy veya baslangic register degerleri (context_switch'in pop islemleri icin yer tutar)
    // Bu, context_switch'in bekledigi saved register sirasinin tersidir.
    // Ornek (context_switch Assembly'deki pop sirasinin tersi):
    *(--stack_ptr) = KERNEL_DATA_SEGMENT; // ES (Dummy)
    *(--stack_ptr) = KERNEL_DATA_SEGMENT; // DS (Dummy)
    *(--stack_ptr) = 0; // AX (Dummy)
    *(--stack_ptr) = 0; // CX (Dummy)
    *(--stack_ptr) = 0; // DX (Dummy)
    *(--stack_ptr) = 0; // BX (Dummy)
    *(--stack_ptr) = (uint16_t)initial_stack_top - sizeof(struct task_context); // SP (Dummy - bu context_switch'e giristeki SP olurdu)
    *(--stack_ptr) = (uint16_t)initial_stack_top - sizeof(struct task_context); // BP (Dummy - bu context_switch'e giristeki BP olurdu)
    *(--stack_ptr) = 0; // SI (Dummy)
    *(--stack_ptr) = 0; // DI (Dummy)

    // Flags, CS, IP degerleri
    *(--stack_ptr) = (uint16_t)seg(task_entry);   ; Gorev giris segmenti (seg macro'su veya operatoru gerekir)
    *(--stack_ptr) = (uint16_t)offset(task_entry); ; Gorev giris offseti (offset macro'su veya operatoru gerekir)
    *(--stack_ptr) = 0x0202; // Flags (Interrupts enabled, default flags) - Pushf'ten gelen Flags degeri

    // Yeni gorevin kaydedilmis baglamini (sankim ilk kez calisiyormus gibi) ayarla
    // context_switch bu yapiyi kaydedilen baglam gibi gorup yukleyecektir.
    // Initial setup for a task that has *not* called schedule yet needs careful thought
    // to match the state saved by context_switch. A simpler approach for cooperative:
    // Set CS:IP in the context to task_entry. Set SS:SP to the stack top.
    // The first call to context_switch for this task should jump to task_entry.
    // This requires the context_switch to handle the initial jump case differently,
    // or set up the stack as if 'call schedule' just returned.
    // Simpler cooperative initial context setup:
    new_task->context.cs = seg(task_entry);
    new_task->context.ip = offset(task_entry);
    new_task->context.ss = KERNEL_DATA_SEGMENT; // Stack segmenti
    new_task->context.sp = (uint16_t)((uint32_t)stack_base + stack_size); // Stack tepesi (offset)
    // Diger registerlar 0 olabilir veya dummy degerler.
    new_task->context.ax = 0; new_task->context.bx = 0; new_task->context.cx = 0; new_task->context.dx = 0;
    new_task->context.si = 0; new_task->context.di = 0; new_task->context.bp = 0;
    new_task->context.es = KERNEL_DATA_SEGMENT; new_task->context.ds = KERNEL_DATA_SEGMENT;
    new_task->context.flags = 0x0202; // Default flags

    // Task sayisini artir
    task_count++;

    // Gorev indexini dondur
    return i;
}


// Zamanlayiciyi calistirir
void schedule(void) {
    struct task *old_task = current_task;

    // Siradaki calismaya hazir gorevi bul (Round-robin)
    int start_index = next_task_index;
    while (1) {
        struct task *next_task = &tasks[next_task_index];

        next_task_index = (next_task_index + 1) % MAX_TASKS; // Bir sonraki index

        // Gorev yuvasi kullaniliyor ve calismaya hazir mi?
        if (next_task->state == TASK_STATE_READY || next_task->state == TASK_STATE_RUNNING) {
             // Kendimiz degilsek ve hazirsa, onu calistir.
             if (next_task != old_task) {
                 current_task = next_task;
                 // Eski gorevin durumunu guncelle (eger RUNNING ise READY yap)
                 if (old_task->state == TASK_STATE_RUNNING) {
                      old_task->state = TASK_STATE_READY;
                 }
                 current_task->state = TASK_STATE_RUNNING; // Yeni gorevi RUNNING yap
                 break; // Siradaki gorev bulundu
             } else {
                 // Sadece biz (mevcut gorev) haziriz veya baska hazir gorev yok.
                 // Idle gorev calisiyorsa ve baska hazir gorev yoksa, yine idle calisir.
                 // Eger calisan tek gorev bizsek ve READY durumundaysak, yine biz calisiriz.
                 // Basit kooperatifte, eger baska READY gorev yoksa, çağıran gorev devam eder.
                 // Daha gelismis senaryoda idle task'e gecilmeli.
                 // Idle task zaten schedule() cagirdigi icin dongu devam eder.
                 // Eger buraya idle disinda bir gorev geldi ve baska hazir gorev yoksa,
                 // o gorev devam eder. Bu basitlik ornektir.
                 // Daha dogrusu, eger baska READY gorev yoksa ve calisan gorev BLOCKED veya EXITING degilse,
                 // calisan gorev READY durumuna gecirilip, idle task calistirilir.
                 // Ancak, kooperatifte BLOCKED durumundaki gorev schedule() cagirmayabilir.
                 // Simdilik sadece bir sonraki READY/RUNNING gorevi bulalim veya donek.
             }
        }

        // Dongu baslangica geri geldiyse ve hala baska gorev bulamadiysak
        if (next_task_index == start_index) {
             // printk("Scheduler: No other ready task found. Staying on current task.\n");
             // Eger tek gorev varsa veya baska hazir gorev yoksa, mevcut gorev devam eder.
             // C89'da break veya return'suz sonsuz dongu olabilir.
             // Ancak biz schedule() cagrisindan sonra yeni goreve atlayacagiz.
             // Eger buraya geldiysek ve hala old_task = current_task ise, demek ki baska gorev yok.
             // Bu durumda old_task (current_task) devam edecek demektir.
             // context_switch'e ayni iki pointeri gonderecegiz, bu da effectively donus yapar.
             current_task = old_task; // Mevcut gorev calismaya devam edecek.
             break;
        }
    }

    // Baglam degisimini yap
    // old_task'in baglamini kaydet ve current_task (yeni gorev) baglamini yukle
    context_switch(&old_task->context, &current_task->context);

    // context_switch fonksiyonu, yeni gorevin baglamina atlar ve ASLA cagiran yerden geri DONMEZ.
    // Kontrol artik secilen gorevdedir.
}

// sched.c sonu