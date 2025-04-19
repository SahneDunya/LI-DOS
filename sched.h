// sched.h
// Lİ-DOS Görev Zamanlayıcı (Scheduler) Arayuzu
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Kernel gorevleri icin kooperatif multitasking.

#ifndef _SCHED_H
#define _SCHED_H

// Gerekli tureler icin basit typedef'ler
// Kernelinizde baska bir baslik dosyasinda (types.h gibi) olabilir.
typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef uint16_t size_t; // 16-bit ortamda size_t genellikle uint16_t olabilir.

// Maksimum gorev sayisi
#define MAX_TASKS 16 // Ornek: Cok fazla gorev 64KB RAM'de yer sikintisi yaratir.

// Gorev durumları
#define TASK_STATE_UNUSED   0 // Gorev yuvasi bos
#define TASK_STATE_READY    1 // Gorev calismaya hazir
#define TASK_STATE_RUNNING  2 // Gorev su an calisiyor
#define TASK_STATE_BLOCKED  3 // Gorev bir kaynak bekliyor (ornegin G/Ç) - Kooperatifte daha az kullanilir
#define TASK_STATE_EXITING  4 // Gorev sonlaniyor

// Gorev baglami (calismasi durduruldugunda kaydedilen registerlar vb.)
// Bu yapi, bir gorevin calismayi biraktigi yerden devam etmesini saglar.
// Kooperatif multitasking'de, genellikle gorevin 'schedule()' cagrisindan
// hemen sonraki durumunu kaydederiz.
struct task_context {
    // Genel amacli registerlar (pusha sirasiyla kaydetmek uygun olabilir)
    uint16_t di;
    uint16_t si;
    uint16_t bp;
    uint16_t sp; // SS:SP register cifti ayri saklanabilir.
    uint16_t bx;
    uint16_t dx;
    uint16_t cx;
    uint16_t ax;
    // Segment registerlari
    uint16_t es;
    uint16_t ds;
    // Flags registeri
    uint16_t flags;
    // Kod adresi (CS:IP) - schedule cagrisindan donulecek adres.
    // Bu genelde stack'te 'call schedule' tarafindan kaydedilir.
    // Baglam degisim assembly kodu bunu stack'ten alip yapiya kaydedebilir.
    uint16_t ip;
    uint16_t cs;
    // SS registeri - SP ile birlikte stack'i tanimlar
    uint16_t ss; // BP registeri stack icinde oldugu icin ss de kaydedilmeli.
};

// Gorev yapısı
struct task {
    struct task_context context; // Gorevin baglami
    uint8_t state;               // Gorevin durumu (READY, RUNNING vb.)
    void *stack_base;            // Tahsis edilen stack alaninin başlangici (dusuk adres)
    size_t stack_size;           // Stack alaninin boyutu (byte)
    // Diger gorev bilgileri eklenebilir (ID, isim, öncelik vb.)
};

// Zamanlayiciyi baslatir
void sched_init(void);

// Yeni bir kernel gorevi olusturur ve zamanlayiciya ekler.
// task_entry: Gorevin baslayacagi fonksiyon pointeri.
// stack_base: Gorev icin tahsis edilmis stack alaninin başlangici.
// stack_size: Gorev icin tahsis edilmis stack alaninin boyutu.
// Donus degeri: Olusturulan gorevin indexi veya hata durumunda -1.
int sched_create_task(void (*task_entry)(void), void *stack_base, size_t stack_size);

// Zamanlayiciyi calistirir. Mevcut gorevin baglamini kaydeder,
// siradaki gorevi secer ve onun baglamini yukleyerek calistirmaya baslar.
// Bu fonksiyon cagrildiginda, bir sonraki cagrida farkli bir gorevden devam eder.
// Kooperatif multitasking'de gorevler bu fonksiyonu calismayi birakmak icin cagirir.
void schedule(void); // Fonksiyon geri donmez (noreturn concept)

// Hali hazirda calisan gorevin pointerini dondurur (Opsiyonel)
// struct task* get_current_task(void);

#endif // _SCHED_H