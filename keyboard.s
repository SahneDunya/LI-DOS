; keyboard.S
; Lİ-DOS Klavye Modülü
; Yazar: Sahne Dünya
; Hedef: 16-bit Real Mode, Intel 8086+
; Amac: Klavye girisini islemek (BIOS polling ve IRQ 1 kesmesi)

.code16                ; 16-bit Real Mode kodu

; C kodundan erisilebilecek fonksiyonlar
.global keyboard_getchar       ; BIOS: Tuşa basılana kadar bekle ve oku
.global keyboard_checkkey      ; BIOS: Tuş var mı kontrol et
.global keyboard_getchar_nowait ; BIOS: Tuş varsa oku, yoksa bekleme (check + read)
.global keyboard_init_irq      ; IRQ: Kesme tabanlı klavye isleyicisini kur
; Kesme isleyicisi kendisi global olmak zorunda degil ama IVT'ye adresi yazilacak.

.text                  ; Kod bolumu

; --- BIOS int 16h Fonksiyonlari (Polling) ---
; Bu fonksiyonlar basit klavye girisi icin kullanilabilir, cekirdek boot ederken vs.

; unsigned int keyboard_getchar(void)
; Tuşa basılana kadar bekler, basılan tuşun ASCII ve Scan Code'unu döndürür (AX'te).
keyboard_getchar:
    mov ah, 0x00       ; BIOS int 16h Function 00h: Read Key, Wait
    int 0x16           ; BIOS Klavye Servisi Çağrısı
    ret                ; AX = (Scan Code << 8) | ASCII

; unsigned int keyboard_checkkey(void)
; Klavye tamponunda tuş var mı kontrol eder.
; Varsa ZF=0, AX = (Scan Code << 8) | ASCII (Tampondan silinmez).
; Yoksa ZF=1, AX = 0.
keyboard_checkkey:
    mov ah, 0x01       ; BIOS int 16h Function 01h: Check Key
    int 0x16           ; BIOS Klavye Servisi Çağrısı
    ret                ; AX = (Scan Code << 8) | ASCII (varsa), ZF durumuna bakılmalı

; unsigned int keyboard_getchar_nowait(void)
; Klavye tamponunda tuş varsa okur (tampondan siler) ve döndürür.
; Yoksa hemen döner ve 0 döndürür.
keyboard_getchar_nowait:
    call keyboard_checkkey ; Tuş var mı kontrol et
    jz .no_key         ; ZF set ise (tuş yok), atla
    ; Tuş var, şimdi tampondan oku (Function 00h non-blocking olur check'ten sonra)
    call keyboard_getchar ; Tuşu oku (bu tampondan siler)
    ret                ; AX = (Scan Code << 8) | ASCII
.no_key:
    xor ax, ax         ; AX = 0 yap
    ret                ; AX = 0 döndür


; --- IRQ 1 Kesme Isleme Yapisi ---
; Bu kisim, donanim kesmelerini kullanarak klavye girisini islemek icindir.
; Daha karmasiktir, PIC ayarlamasi ve IVT guncellemesi gerektirir.

; Klavye Kesme İşleyicisi (Bu label'in adresi IVT'ye yazılacak)
; Donanim kesmesi geldiginde CPU buraya atlar (IVT araciligiyla).
keyboard_interrupt_handler:
    ; !!! ÖNEMLİ: Kesme işleyicileri tüm registerları kaydetmelidir !!!
    pusha              ; Tüm genel amaçlı registerları stack'e kaydet (AX, CX, DX, BX, SP, BP, SI, DI)
    pushf              ; Flags registerini stack'e kaydet

    ; Klavye Scan Code'unu Port 0x60'tan oku
    in al, 0x60        ; Klavye Veri Portu
    ; Scan Code simdi AL registerinda

    ; --- Scan Code İşleme (Gerçek OS çekirdeğinde burada karmaşık mantık olur) ---
    ; Burada scan code'un ne anlama geldiği çözülür:
    ; - Hangi tuşa basıldı / bırakıldı? (Scan code'un en yüksek biti bırakmayı belirtir)
    ; - Shift, Ctrl, Alt gibi modifier tuşların durumu ne?
    ; - Bu scan code hangi ASCII karaktere veya özel tuş koduna karşılık geliyor? (Tuş haritası)
    ; - Elde edilen karakter veya tuş kodu bir klavye bufferına (örneğin dairesel buffer) yazılır.
    ; Örnek olarak, sadece scan code'u bir değişkene kaydedelim veya temel bir işleme yapalım.
    mov [last_scan_code], al ; Örnek: Sadece son scan code'u sakla

    ; --- PIC'e İşlemin Bittiğini Bildir (EOI - End Of Interrupt) ---
    ; Master PIC (IRQ 0-7) için EOI komutunu port 0x20'ye gönder.
    ; Eğer Slave PIC (IRQ 8-15) de kullanılıyorsa, Master PIC'e de EOI göndermek gerekir.
    mov al, 0x20       ; EOI komutu
    out 0x20, al       ; Master PIC Command Port

    ; Eğer Slave PIC kullanılıyorsa ve bu kesme Slave PIC'ten geldiyse (IRQ 8-15),
    ; Slave PIC'e de EOI göndermek gerekir:
    ; out 0xA0, al     ; Slave PIC Command Port

    ; !!! ÖNEMLİ: Kaydedilen registerları geri yükle ve kesmeden dön !!!
    popf               ; Flags registerini stack'ten geri yükle
    popa              ; Genel amaçlı registerları stack'ten geri yükle (pusha sırasının tersi)
    iret               ; Kesmeden dön (Interrupt Return - stack'ten CS, IP, Flags çeker)


; Klavye Kesme İşleyicisini Kurulum Fonksiyonu
; Bu fonksiyon C'den çağrılabilir (örneğin kernel başlangıcında).
; PIC'i yeniden programlar ve IVT'deki IRQ 1 girişini kendi isleyicimize yonlendirir.
keyboard_init_irq:
    push bp            ; BP'yi kaydet
    mov bp, sp         ; Stack frame olustur
    pusha              ; Tum registerlari kaydet

    cli                ; Kesmeleri devre disi birak (IVT'yi guncellerken guvenlik)

    ; --- PIC'i Yeniden Programlama (Remapping) ---
    ; BIOS varsayilan olarak IRQ 0-7'yi IVT vector 0x08-0x0F'ye esler.
    ; Bu adresler CPU Exception'lari ile cakisir (Divide by Zero, dll.).
    ; IRQ'lari guvenli bir alana tasiyalim, ornegin 0x20-0x2F (32-47).
    ; Bu basit bir 8259A remapping ornegidir.
    ; Daha fazla detay ve error handling gerekebilir gercek kodda.

    mov al, 0x11       ; ICW1: Start initialization, ICW4 needed
    out 0x20, al       ; Master PIC (0x20)

    mov al, 0x20       ; ICW2: Interrupt vector offset (Master will map to 0x20-0x27)
    out 0x21, al       ; Master PIC Data Port (0x21)

    mov al, 0x04       ; ICW3: Tell Master PIC about Slave PIC (Slave is on IRQ 2)
    out 0x21, al

    mov al, 0x01       ; ICW4: 80x86 mode
    out 0x21, al

    ; Slave PIC ayarlari (Eger varsa ve IRQ 8-15 kullanilacaksa)
    ; mov al, 0x11     ; ICW1: Start initialization, ICW4 needed
    ; out 0xA0, al     ; Slave PIC (0xA0)
    ; mov al, 0x28     ; ICW2: Interrupt vector offset (Slave will map to 0x28-0x2F)
    ; out 0xA1, al     ; Slave PIC Data Port (0xA1)
    ; mov al, 0x02     ; ICW3: Tell Slave PIC its cascade identity (connected to Master's IRQ 2)
    ; out 0xA1, al
    ; mov al, 0x01     ; ICW4: 80x86 mode
    ; out 0xA1, al

    ; IRQ Maskelerini Ayarla
    ; Tum IRQ'lari maskeleyelim, sadece IRQ 1'i (Klavye) acalim.
    ; IRQ maskeleri OCW1 komutu ile 0x21 (Master) ve 0xA1 (Slave) portlarina yazilir.
    ; Maske bitleri 0 = acik, 1 = kapali. IRQ 1 maskesi 0000 0010b = 0x02.
    ; Sadece IRQ 1'i acmak icin 1111 1101b = 0xFD maskesini yazmaliyiz.
    mov al, 0xFD       ; IRQ mask: Clear bit 1 (keyboard), set others
    out 0x21, al       ; Master PIC Data Port (0x21)
    ; out 0xA1, 0xFF   ; Slave PIC Data Port (Tum Slave IRQ'lari maskeleyelim simdilik)

    ; --- IVT (Interrupt Vector Table) Guncelleme ---
    ; IRQ 1'in IVTdeki karsiligi Vector 9'dur (BIOS varsayilan 0x09h).
    ; Bizim yeni isleyicimiz artik 0x20+1 = 0x21h vectorunde olmali (PIC remapping yaptik).
    ; Yani IVT[0x21]'i (adres 0x0000:0x0084) bizim keyboard_interrupt_handler adresimizle doldur.
    ; IVT girisi 4 byte'tir: [Offset (16-bit), Segment (16-bit)]

    mov dx, cs         ; Guncel kod segmentimizi al (kernel segmenti)
    mov ds, dx         ; DS = CS (veri erisimi icin)

    ; IVT entry 0x21 adresi: 0x0000 segmentinde 0x21 * 4 = 0x84 offset
    mov es, word 0x0000 ; ES = IVT segmenti (0x0000)
    mov di, word 0x0084 ; DI = IVT[0x21] offseti (0x21 * 4)

    ; Kesme isleyicimizin segment ve offset adresini IVT'ye yaz
    mov word [es:di], offset keyboard_interrupt_handler ; IVT[0x21]'in offset kismi
    mov word [es:di+2], seg keyboard_interrupt_handler   ; IVT[0x21]'in segment kismi

    sti                ; Kesmeleri ac (Kurulum tamamlandi)

    popa              ; Kaydedilen registerlari geri yukle
    pop bp             ; BP'yi geri yukle
    ret                ; Fonksiyondan don


; --- Veri Bolumu ---

.section .data
    last_scan_code db 0 ; Ornek: Son okunan scan code'u saklamak icin degisken

; Daha gelismis bir implementasyonda, burada bir klavye bufferi tanimlanabilir:
; .section .bss
; KB_BUFFER_SIZE equ 64
; keyboard_buffer resb KB_BUFFER_SIZE ; Klavye bufferi (bos bellek)
; kb_head dw 0                     ; Buffer head pointer (offset)
; kb_tail dw 0                     ; Buffer tail pointer (offset)

; keyboard.S sonu