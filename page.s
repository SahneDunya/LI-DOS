; page.S
; Lİ-DOS Düşük Seviye Bellek/Adresleme Yardımcıları
; Yazar: Sahne Dünya
; Hedef: 16-bit Real Mode, Intel 8086+
; Amac: Segment registerlari gibi bellek/adresleme ile ilgili Assembly fonksiyonlari.
; NOT: Bu dosya 16-bit Real Mode'da SAYFALAMA (PAGING) implementasyonu ICERMEZ.

.code16                ; 16-bit Real Mode kodu

; C kodundan erisilebilecek fonksiyonlar
.global get_cs
.global get_ds
.global get_es
.global get_ss
.global set_ds
.global set_es
.global set_ss

.text                  ; Kod bolumu

; unsigned short get_cs(void)
; CS segment registerinin degerini dondurur (AX'te).
get_cs:
    mov ax, cs         ; CS degerini AX'e yukle
    ret                ; AX = CS

; unsigned short get_ds(void)
; DS segment registerinin degerini dondurur (AX'te).
get_ds:
    mov ax, ds
    ret                ; AX = DS

; unsigned short get_es(void)
; ES segment registerinin degerini dondurur (AX'te).
get_es:
    mov ax, es
    ret                ; AX = ES

; unsigned short get_ss(void)
; SS segment registerinin degerini dondurur (AX'te).
get_ss:
    mov ax, ss
    ret                ; AX = SS

; void set_ds(unsigned short segment)
; DS segment registerinin degerini ayarlar.
; Parametreler (stack'te): [bp+4]: segment (uint16_t)
set_ds:
    push bp
    mov bp, sp
    mov ax, [bp+4]     ; Segment degerini AX'e yukle
    mov ds, ax         ; AX'i DS'ye yukle
    pop bp
    ret

; void set_es(unsigned short segment)
; ES segment registerinin degerini ayarlar.
; Parametreler (stack'te): [bp+4]: segment (uint16_t)
set_es:
    push bp
    mov bp, sp
    mov ax, [bp+4]
    mov es, ax
    pop bp
    ret

; void set_ss(unsigned short segment)
; SS segment registerinin degerini ayarlar.
; Parametreler (stack'te): [bp+4]: segment (uint16_t)
set_ss:
    push bp
    mov bp, sp
    mov ax, [bp+4]
    mov ss, ax
    pop bp
    ret

; page.S sonu