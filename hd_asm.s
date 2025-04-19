; bios_disk_io fonksiyonu (ornegin asm.S icine eklenir)
; BIOS int 13h cagrilarini C'den yapabilmek icin yardimci.
; Yazar: Sahne Dünya
; Hedef: 16-bit Real Mode

.code16

.global bios_disk_io
; uint8_t bios_disk_io(uint8_t command, uint8_t count, uint16_t cylinder, uint8_t head, uint8_t sector, uint16_t buffer_segment, uint16_t buffer_offset, uint8_t drive)
; Parametreler (stack'te, C'den tersten itildigi varsayilir):
; [bp+18]: drive (uint8_t)
; [bp+16]: buffer_offset (uint16_t)
; [bp+14]: buffer_segment (uint16_t)
; [bp+12]: sector (uint8_t)
; [bp+10]: head (uint8_t)
; [bp+8]:  cylinder (uint16_t)
; [bp+6]:  count (uint8_t)
; [bp+4]:  command (uint8_t)
; Donus degeri: AH registeri (BIOS hata kodu)

bios_disk_io:
    push bp            ; BP'yi kaydet
    mov bp, sp         ; Stack frame olustur
    push ds            ; Segment registerlarini kaydet
    push es

    ; Parametreleri registerlara yukle
    mov al, [bp+4]     ; command (AH'a kopyalanacak)
    mov ah, al         ; AH = command
    mov al, [bp+6]     ; AL = count
    mov cx, [bp+8]     ; CX = cylinder (tamami)
    mov bl, [bp+10]    ; BH = head (BL'ye yukle, sonra BH'a tasinir)
    mov bh, bl         ; BH = head
    mov cl, [bp+12]    ; CL = sector (CL'ye yukle)
    mov dl, [bp+18]    ; DL = drive
    mov es, [bp+14]    ; ES = buffer_segment
    mov bx, [bp+16]    ; BX = buffer_offset (ES:BX hedef)

    ; CHS formatina donusum (int 13h icin)
    ; CHS: Cylinder (10 bit), Head (8 bit), Sector (6 bit)
    ; Registerlar: CH (Cylinder 0-7), CL (Sector 0-5, Cylinder 8-9)
    ; DH (Head 0-7), DL (Drive 0-7)
    ; CX'teki 16-bit cylinder degerinin dusuk 8 bitini CH'ye koy
    mov ch, cl         ; CL'deki sector bilgisini gecici olarak kaydet
    mov cl, dh         ; DH'deki head bilgisini CL'ye tasi (yedekle)
    mov dh, bh         ; BH'deki head bilgisini DH'a tasi (DH = Head)
    mov bh, 0          ; BH'i temizle
    mov ch, cx         ; CX'in dusuk 8 bitini CH'ye tasi (CH = Cylinder 0-7)

    ; CX'in yuksek 2 bitini (cylinder 8-9) CL'nin yuksek 2 bitine tasi (CL = Sector 0-5, Cylinder 8-9)
    shl cx, 2          ; CX'i 2 bit sola kaydir (cylinder 8-9 bitleri CX'in en yuksek bitlerine gelir)
    or cl, ch          ; CH'deki sektor bilgilerini CL'nin kalan bitleriyle birlestir (CL = Sector 0-5 | Cylinder 8-9)
                       ; Simdi CX = (Cylinder 8-9 << 8) | (Cylinder 0-7)
                       ; Ve CL = (Cylinder 8-9 << 6) | Sector 0-5
                       ; C'den gelen cylinder degeri 16-bit, BIOS ise 10-bit bekler.
                       ; Yukaridaki hesaplamada hata var. Yeniden duzenleyelim:
    mov ax, [bp+8]     ; AX = cylinder (16-bit)
    mov bl, [bp+12]    ; BL = sector (8-bit)
    mov cl, bl         ; CL = sector (simdilik 6 biti onemli)
    mov ch, al         ; CH = cylinder (dusuk 8 bit)
    shr ax, 8          ; AX = cylinder (yuksek 8 bit)
    and ax, 0x03       ; AX = cylinder (8. ve 9. bitleri)
    shl ax, 6          ; AX = (cylinder 8-9) << 6
    or cl, al          ; CL = Sector 0-5 | (cylinder 8-9 << 6)
    mov dh, [bp+10]    ; DH = head

    ; int 13h cagrisi
    int 0x13

    ; Donus degerini hazirla
    ; AH hata kodunu dondurecegiz. Carry flag set ise hata var.
    ; Carry flag set olmasa bile AH 0 olmayabilir (ornegin warning).
    ; int 13h cagrisindan sonra AH hata kodunu icerir, AL ise transfer edilen sektor sayisini.
    ; Biz sadece hata kodunu dondurelim (AH registeri).
    movzx ax, ah       ; AH'yi AX'e zero-extend yap (uint8_t donus degeri icin AH yeterli ama AX kullanilir genelde)

    pop es             ; Segment registerlarini geri yukle
    pop ds
    pop bp
    ret                ; Fonksiyondan don