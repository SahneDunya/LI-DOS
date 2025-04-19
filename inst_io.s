; inst_io.S
; Lİ-DOS Kurulum Programı Düşük Seviye G/Ç
; Yazar: Sahne Dünya
; Hedef: 16-bit Real Mode, Intel 8086+
; Amac: BIOS int 10h ve int 16h kullanarak temel konsol/klavye G/Ç.

.code16

.global inst_putc
.global inst_puts
.global inst_getc
.global inst_checkkey

.text

; void inst_putc(char c)
; BIOS int 10h kullanarak ekrana karakter yazar.
; Parametre (stack): [bp+4]: c (char, int olarak itilir)
inst_putc:
    push bp
    mov bp, sp
    push bx
    push ax
    push cx
    push dx

    mov bh, 0          ; Page number 0
    mov bl, 0x07       ; Attribute (light grey on black)
    mov ah, 0x0E       ; BIOS int 10h Function 0Eh: Write char in Teletype mode
    mov al, [bp+4]     ; Character to write
    xor cx, cx         ; Number of times to write (1)

    int 0x10           ; BIOS Video Service

    pop dx
    pop cx
    pop ax
    pop bx
    pop bp
    ret

; void inst_puts(const char *s)
; Ekrana null terminated string yazar.
; Parametre (stack): [bp+4]: s (pointer, 16-bit offset)
inst_puts:
    push bp
    mov bp, sp
    push si
    push ds

    mov si, [bp+4]     ; SI = string pointer (offset)
    ; String pointerin segmenti DS olmali. Varsayalim C kodu stringi DS segmentinde veriyor.
    ; Ya da C kodu segment ve offseti ayri vermeli (C89 uyumlulugu icin daha dogru)
    ; Bu ornekte basitlik icin DS=CS veya DS=KernelDataSegment varsayalim.
    ; Farkli bir segmentten okuma gerekirse DS ayarlanmali.
    ; mov ax, [string_segment] ; Eger ayri segmentten okuma gerekirse
    ; mov ds, ax

.loop_puts:
    mov al, [si]       ; Karakteri al
    cmp al, 0          ; Null terminator mu?
    je .done_puts      ; Evet ise bitir

    push word si       ; inst_putc char parametresi. Stack'e pointeri it.
                       ; Char olarak gececegi icin aslinda sadece 1 byte gerekiyor.
                       ; C cagri kuralinda char int olarak itildigi icin 2 byte iteriz.
    mov ax, [si]       ; Karakteri AX'in AL'sine al
    push ax            ; int parametresi gibi stack'e it
    call inst_putc     ; Karakteri yazdir
    add sp, 2          ; Parametreyi stack'ten temizle

    inc si             ; Sonraki karaktere gec
    jmp .loop_puts

.done_puts:
    pop ds
    pop si
    pop bp
    ret

; char inst_getc(void)
; BIOS int 16h kullanarak klavyeden karakter okur (bekler).
; Donus degeri: AX = (Scan Code << 8) | ASCII
inst_getc:
    mov ah, 0x00       ; BIOS int 16h Function 00h: Read Key, Wait
    int 0x16           ; BIOS Klavye Servisi
    ret                ; AX = result

; int inst_checkkey(void)
; BIOS int 16h kullanarak klavye tamponunda tuş var mı kontrol eder.
; Varsa ZF=0, AX=(Scan Code << 8) | ASCII (tampondan silinmez), donus: 1.
; Yoksa ZF=1, AX=0, donus: 0.
inst_checkkey:
    mov ah, 0x01       ; BIOS int 16h Function 01h: Check Key
    int 0x16           ; BIOS Klavye Servisi
    jz .no_key_check   ; ZF set ise (tuş yok)

    mov ax, 1          ; Tuş var, AX = 1 yap (donus degeri)
    ret

.no_key_check:
    xor ax, ax         ; Tuş yok, AX = 0 yap (donus degeri)
    ret

; inst_io.S sonu