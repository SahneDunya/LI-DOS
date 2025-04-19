; rs_io.S
; Lİ-DOS RS-232 (Serial Port) Düsük Seviye G/Ç Modulu
; Yazar: Sahne Dünya
; Hedef: 16-bit Real Mode, Intel 8086+
; Amac: Seri port donanimi ile dogrudan port G/Ç kullanarak iletisim.

.code16                ; 16-bit Real Mode kodu

; C kodundan erisilebilecek fonksiyonlar
.global rs_init            ; Seri portu baslat ve yapilandir
.global rs_putc            ; Seri porta tek karakter gonder (polling)
.global rs_has_data        ; Seri portta okunacak veri var mi kontrol et
.global rs_getc            ; Seri porttan tek karakter oku (polling)

.text                  ; Kod bolumu

; --- UART Kayitcik Offsetleri (Base Port'a Gore) ---
; Bu offsetler cogu 16550 UART uyumlu cipset icin gecerlidir.
.equ UART_DATA         = 0  ; Veri Kayitcigi (R/W, DLAB=0), Bolucu Dusuk Byte (R/W, DLAB=1)
.equ UART_IER          = 1  ; Kesme Etkinlestirme Kayitcigi (R/W, DLAB=0), Bolucu Yuksek Byte (R/W, DLAB=1)
.equ UART_IIR          = 2  ; Kesme Kimlik Kayitcigi (R), FIFO Kontrol Kayitcigi (W)
.equ UART_LCR          = 3  ; Hat Kontrol Kayitcigi (R/W) - Bit 7 DLAB (Divisor Latch Access Bit)
.equ UART_MCR          = 4  ; Modem Kontrol Kayitcigi (R/W)
.equ UART_LSR          = 5  ; Hat Durum Kayitcigi (R) - Bit 0 Data Ready, Bit 5 THRE (Transmitter Holding Register Empty)
.equ UART_MSR          = 6  ; Modem Durum Kayitcigi (R)
.equ UART_SCRATCH      = 7  ; Scratchpad Kayitcigi (R/W)

; --- Fonksiyon Implementasyonlari ---

; void rs_init(unsigned short port_base, unsigned short baud_divisor, unsigned char lcr_value, unsigned char ier_value, unsigned char mcr_value)
; Seri portu baslatir ve konfigure eder.
; Parametreler (stack'te, C'den tersten itildigi varsayilir):
; [bp+12]: mcr_value (uint8_t)
; [bp+10]: ier_value (uint8_t)
; [bp+8]:  lcr_value (uint8_t)
; [bp+6]:  baud_divisor (uint16_t)
; [bp+4]:  port_base (uint16_t)
rs_init:
    push bp            ; BP'yi kaydet
    mov bp, sp         ; Stack frame olustur
    push dx            ; DX'i kaydet (port adresi icin kullanilacak)
    push ax            ; AX'i kaydet (gecici degerler icin)
    push cx            ; CX'i kaydet (gecici degerler icin)

    mov dx, [bp+4]     ; DX = port_base

    ; Baud rate ayarlamak icin DLAB'i etkinlestir (Line Control Register'in 7. biti)
    mov al, [bp+8]     ; AL = lcr_value
    or al, 0x80        ; DLAB bitini set et (0x80)
    out dx+UART_LCR, al; LCR'a yaz

    ; Baud rate bolucusunu yaz (DLAB=1 iken Data ve IER portlarina)
    mov ax, [bp+6]     ; AX = baud_divisor (16-bit)
    out dx+UART_DATA, al ; Bolucunun dusuk byte'ini yaz (port_base+0)
    mov al, ah         ; AX'in yuksek byte'ini AL'ye tasi
    out dx+UART_IER, al  ; Bolucunun yuksek byte'ini yaz (port_base+1)

    ; Baud rate ayarlari bitti, DLAB'i devre disi birak (lcr_value'nun kendisini yazarak)
    mov al, [bp+8]     ; AL = lcr_value (DLAB biti temizlenmis hali)
    out dx+UART_LCR, al; LCR'a yaz

    ; Kesme Etkinlestirme Kayitcigini (IER) ayarla (Polling icin 0x00)
    mov al, [bp+10]    ; AL = ier_value
    out dx+UART_IER, al; IER'a yaz

    ; Modem Kontrol Kayitcigini (MCR) ayarla (Ornegin DTR ve RTS'i etkinlestirmek icin 0x03)
    mov al, [bp+12]    ; AL = mcr_value
    out dx+UART_MCR, al; MCR'a yaz

    ; FIFO'yu temizle ve etkinlestir (IIR portuna yazarak) - 16550+ UART'lar icin
    ; 0x07 = FIFO'yu etkinlestir, Rx ve Tx FIFO'larini temizle.
    ; out dx+UART_IIR, 0x07 ; Eger FIFO kullanilacaksa

    pop cx             ; Kaydedilen registerlari geri yukle
    pop ax
    pop dx
    pop bp
    ret                ; Fonksiyondan don

; void rs_putc(unsigned short port_base, unsigned char char_to_send)
; Seri porta bir karakter gonderir. Gonderici tamponu bosalana kadar bekler (polling).
; Parametreler (stack'te): [bp+6]: char_to_send (uint8_t), [bp+4]: port_base (uint16_t)
rs_putc:
    push bp
    mov bp, sp
    push dx
    push ax

    mov dx, [bp+4]     ; DX = port_base
    mov al, [bp+6]     ; AL = char_to_send

.wait_for_thre:
    ; Hat Durum Kayitcigini (LSR) oku (Port_base + 5)
    in al, dx+UART_LSR
    ; THRE bitini kontrol et (Bit 5). 1 ise gonderici tamponu bos.
    test al, 0x20      ; AL'nin 5. bitini kontrol et
    jz .wait_for_thre  ; Bit 0 ise (bos degil), donguyu tekrarla

    ; Gonderici tamponu bos, karakteri Veri Kayitcigina yaz (Port_base + 0)
    mov al, [bp+6]     ; Gonderilecek karakteri tekrar AL'ye yukle
    out dx+UART_DATA, al ; Veri Kayitcigina yaz

    pop ax
    pop dx
    pop bp
    ret

; unsigned char rs_has_data(unsigned short port_base)
; Seri portun alici tamponunda okunacak veri olup olmadigini kontrol eder.
; Donus degeri: 1 (veri var) veya 0 (veri yok).
; Parametreler (stack'te): [bp+4]: port_base (uint16_t)
rs_has_data:
    push bp
    mov bp, sp
    push dx
    push ax

    mov dx, [bp+4]     ; DX = port_base

    ; Hat Durum Kayitcigini (LSR) oku (Port_base + 5)
    in al, dx+UART_LSR
    ; Data Ready bitini kontrol et (Bit 0). 1 ise alici tamponunda veri var.
    test al, 0x01      ; AL'nin 0. bitini kontrol et
    jz .no_data        ; Bit 0 ise (veri yok), atla

    ; Veri var, AX'e 1 koy ve don
    mov ax, 1
    jmp .done

.no_data:
    ; Veri yok, AX'e 0 koy ve don
    xor ax, ax

.done:
    pop ax
    pop dx
    pop bp
    ret                ; AX = 1 veya 0

; unsigned char rs_getc(unsigned short port_base)
; Seri porttan bir karakter okur. Karakter gelene kadar bekler (polling).
; Donus degeri: AL (okunan karakter).
; Parametreler (stack'te): [bp+4]: port_base (uint16_t)
rs_getc:
    push bp
    mov bp, sp
    push dx
    push ax

    mov dx, [bp+4]     ; DX = port_base

.wait_for_data:
    ; Hat Durum Kayitcigini (LSR) oku (Port_base + 5)
    in al, dx+UART_LSR
    ; Data Ready bitini kontrol et (Bit 0). 1 ise veri var.
    test al, 0x01      ; AL'nin 0. bitini kontrol et
    jz .wait_for_data  ; Bit 0 ise (veri yok), donguyu tekrarla

    ; Veri var, Veri Kayitcigindan oku (Port_base + 0)
    in al, dx+UART_DATA ; Veriyi oku, AL'ye koy

    pop ax
    pop dx
    pop bp
    ret                ; AL = okunan karakter (AX'in dusuk 8 biti)

; rs_io.S sonu