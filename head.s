; head.S
; Lİ-DOS Kernel Başlangıç Noktası (Yüklenen Kısım)
; Yazar: Sahne Dünya
; Hedef: 16-bit Real Mode, Intel 8086+
; Amac: Bootloader'dan sonra calisir, Real Mode ortami kurar, C'ye atlar.

.code16                        ; 16-bit Real Mode kodu
.global _start                 ; Kernel yükleme adresi (başlangıç noktası)
.global c_main                 ; C ana fonksiyonu (diger dosyalarda tanimli)

; Linker scriptten BSS baslangic ve bitis adreslerini al (Kernel segmenti icinde offset)
extern _bss_start
extern _bss_end

; Kernelin bellekte yükleneceği segment (boot.S'te belirlenen ile ayni olmali)
.equ KERNEL_LOAD_SEGMENT = 0x1000

; Kernel stack boyutu ve yeri (Kernel segmenti icinde)
.equ KERNEL_STACK_SIZE = 4096  ; Ornek stack boyutu (4KB)
; Stack en yuksek adresten (0xFFFF) asagi dogru buyur.
; Kernel stackin en üstü 0xFFFF'e ayarlanacak.
.equ KERNEL_STACK_TOP_OFFSET = 0xFFFF

.text                          ; Kod bölümü
.org 0                         ; head.S çekirdek imajının en başıdır, bu yüzden offset 0'dır.

_start:
    ; Segment registerlarını ayarla
    ; CS zaten bootloader tarafından KERNEL_LOAD_SEGMENT'e ayarlandı (retf ile).
    mov ax, KERNEL_LOAD_SEGMENT
    mov ds, ax                 ; DS = Kernel segmenti
    mov es, ax                 ; ES = Kernel segmenti
    mov ss, ax                 ; SS = Kernel segmenti

    ; Stack pointeri ayarla
    mov sp, KERNEL_STACK_TOP_OFFSET ; SP = Kernel segmentinin en üst offseti

    ; BSS bölümünü sıfırla (.bss sectiondaki tüm değişkenleri 0 yapar)
    ; Linker script _bss_start ve _bss_end sembollerini sağlar.
    mov di, offset _bss_start  ; DI = BSS başlangıç offseti (ES:DI hedef)
                               ; ES zaten kernel segmenti olarak ayarlı
    mov ax, offset _bss_end    ; AX = BSS bitiş offseti
    sub ax, offset _bss_start  ; AX = BSS boyutu
    mov cx, ax                 ; CX = Tekrar sayisi (byte)
    shr cx, 1                  ; CX = Tekrar sayisi (word) - stosw kullanilacaksa
                               ; BYTE BYTE SIFIRLAYALIM: CX = BSS boyutu (byte)
    mov cx, ax                 ; CX = BSS boyutu (byte)

    xor al, al                 ; AL = 0 (stosb ile yazılacak değer)
    rep stosb                  ; ES:DI adresinden baslayarak CX kez AL degerini yazar (DI her seferinde artar)

    ; Gerekli diğer donanım başlangıç ayarları burada veya C'de yapılabilir.
    ; Örn: PIC (8259A) init'i C'de traps_init icinde yapilir. Timer init'i de oyle.

    ; Yüksek seviye C ana fonksiyonunu çağır
    call c_main

    ; c_main fonksiyonu normalde dönmemelidir (kernel sonsuz döngüde kalır).
    ; Ama dönerse sistemi durdur veya panik yap.
    cli
    hlt

; head.S sonu