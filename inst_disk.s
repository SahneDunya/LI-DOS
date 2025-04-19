; inst_disk.S
; Lİ-DOS Kurulum Programı Düşük Seviye Disk G/Ç (Güncellenmiş)
; Yazar: Sahne Dünya
; Hedef: 16-bit Real Mode, Intel 8086+
; Amac: BIOS int 13h kullanarak sektor okuma/yazma ve geometri okuma.
; LBA -> CHS çevrimi detect edilen geometri ile yapilir veya LBA uzantisi kullanilir.

.code16

.global inst_read_sectors
.global inst_write_sectors
.global inst_get_drive_params

.text

; --- Sabitler ---
.equ BIOS_READ_SECTORS  = 0x02
.equ BIOS_WRITE_SECTORS = 0x03
.equ BIOS_GET_DRIVE_PARAMS = 0x08 ; Yeni: Sürücü parametrelerini oku

; --- Global Geometri Bilgileri ---
; inst_get_drive_params tarafindan doldurulacak, diger fonksiyonlar tarafindan kullanilacak.
; Her surucu icin ayri tutulmasi gerekebilir, basitlik icin tek set.
.section .data
.global detected_spt
.global detected_heads
.global detected_cylinders ; Toplam silindir sayisi (isteğe bağlı)

detected_spt dw 0
detected_heads dw 0
detected_cylinders dw 0

; --- LBA -> CHS Çevrim Yardimcisi (Assembly'de) ---
; C kodundan LBA alinip, global geometry bilgisi kullanilarak CHS hesaplanir.
; Parameters: EAX = LBA (32-bit)
; Outputs: CX = (Cyl high bits << 6) | Sector (1 based), DH = Head (0 based), DL = Drive (input DL'den)
; Reads from global detected_spt, detected_heads
.macro LBA_TO_CHS lba_eax_reg, drive_dl_reg
    push eax edx ebx   ; Kaydet
    mov ebx, dword [detected_spt] ; EBX = Sectors Per Track
    test ebx, ebx ; SPT 0 ise hata olabilir
    jz .lba_to_chs_error
    xor edx, edx       ; EDX = 0
    div ebx            ; EAX = LBA / SPT, EDX = LBA % SPT

    mov cl, dl         ; CL = EDX (Sector 0 based)
    inc cl             ; CL = Sector number (1 based)

    mov ebx, dword [detected_heads] ; EBX = Number of Heads
    test ebx, ebx ; Heads 0 ise hata olabilir
    jz .lba_to_chs_error
    xor edx, edx       ; EDX = 0
    div ebx            ; EAX = (LBA / SPT) / Heads (Cyl), EDX = (LBA / SPT) % Heads (Head)
    mov dh, dl         ; DH = EDX (Head number)

    mov ch, al         ; CH = EAX (Cylinder lower 8 bits)
    shr eax, 8         ; Shift EAX right by 8
    and eax, 0x03      ; Keep only bits 8 and 9 of Cylinder
    shl eax, 6         ; Shift these 2 bits to bit 6 and 7
    or cl, al          ; Combine with CL (Sector 1-6, Cylinder 8-9)

    mov dl, \drive_dl_reg ; DL = Input Drive ID
    pop ebx edx eax    ; Geri yukle
    jmp .lba_to_chs_done

.lba_to_chs_error:
    ; Hata durumu. Gecersiz CHS döndürebilir. Çağıran hata kontrolü yapmalı.
    ; Basitlik icin simdi 0 dondurelim.
    xor cx, cx
    xor dh, dh
    mov dl, \drive_dl_reg
    pop ebx edx eax
.lba_to_chs_done:
.endm


; --- BIOS int 13h Fonksiyon Implementasyonlari ---

; unsigned char inst_read_sectors(uint8_t drive, uint32_t lba, uint8_t count, uint16_t buffer_segment, uint16_t buffer_offset)
; BIOS int 13h kullanarak sektor okur. Detect edilen geometriyi kullanir.
inst_read_sectors:
    push bp
    mov bp, sp
    push bx cx dx es   ; Registerlari kaydet

    mov dl, [bp+14]    ; DL = drive
    mov al, [bp+9]     ; AL = count
    mov es, [bp+6]     ; ES = buffer_segment
    mov bx, [bp+4]     ; BX = buffer_offset (ES:BX hedef)

    ; LBA adresini al (uint32_t)
    mov eax, dword [bp+10] ; EAX = LBA (32-bit)

    ; LBA -> CHS Çevrimini yap (LBA_TO_CHS macro kullan)
    LBA_TO_CHS eax, dl

    mov ah, BIOS_READ_SECTORS ; AH = Okuma fonksiyon kodu
    int 0x13               ; BIOS Disk Servisi

    ; Donus degeri: AH = Hata kodu (0 = basari), Carry Flag set ise hata.
    movzx ax, ah       ; AH'yi AX'e zero-extend yap

    pop es dx cx bx    ; Registerlari geri yukle
    pop bp
    ret

; unsigned char inst_write_sectors(uint8_t drive, uint32_t lba, uint8_t count, uint16_t buffer_segment, uint16_t buffer_offset)
; BIOS int 13h kullanarak sektor yazar. Detect edilen geometriyi kullanir.
inst_write_sectors:
    push bp
    mov bp, sp
    push bx cx dx es   ; Registerlari kaydet

    mov dl, [bp+14]    ; DL = drive
    mov al, [bp+9]     ; AL = count
    mov es, [bp+6]     ; ES = buffer_segment
    mov bx, [bp+4]     ; BX = buffer_offset (ES:BX kaynak)

    ; LBA adresini al (uint32_t)
    mov eax, dword [bp+10] ; EAX = LBA (32-bit)

    ; LBA -> CHS Çevrimini yap (LBA_TO_CHS macro kullan)
    LBA_TO_CHS eax, dl

    mov ah, BIOS_WRITE_SECTORS ; AH = Yazma fonksiyon kodu
    int 0x13               ; BIOS Disk Servisi

    ; Donus degeri: AH = Hata kodu (0 = basari), Carry Flag set ise hata.
    movzx ax, ah       ; AH'yi AX'e zero-extend yap

    pop es dx cx bx
    pop bp
    ret

; unsigned char inst_get_drive_params(uint8_t drive)
; BIOS int 13h AH=08h kullanarak sürücü parametrelerini (geometri) okur.
; Global degiskenleri doldurur: detected_spt, detected_heads, detected_cylinders.
; Parametre (stack): [bp+4]: drive (uint8)
; Donus degeri: AH = Hata kodu (0 = basari), Carry Flag set ise hata.
inst_get_drive_params:
    push bp
    mov bp, sp
    push bx cx dx ax es ; Registerlari kaydet

    mov dl, [bp+4]     ; DL = drive ID
    mov ah, BIOS_GET_DRIVE_PARAMS ; AH = Get Drive Parameters

    int 0x13           ; BIOS Disk Servisi

    ; Donus: AH=hata, AL=0 ise hata yok; CHS bilgisi CX, DH, DL'de.
    ; CX: Bits 0-5 = Sectors per Track, Bits 6-7 = Cylinder high bits.
    ; DH: Max Head number (0 based) -> Num Heads = DH + 1.
    ; DL: Number of drives.
    ; BX: Disk type (ATAPI etc.)

    ; Hata kontrolü
    jnc .params_ok     ; Carry Flag set değilse (hata yok)

    ; Hata durumunda global bilgileri sıfırla
    xor word ptr [detected_spt], [detected_spt]
    xor word ptr [detected_heads], [detected_heads]
    xor word ptr [detected_cylinders], [detected_cylinders]
    ; Donus: AH (hata kodu)
    movzx ax, ah
    jmp .params_done

.params_ok:
    ; CHS bilgilerini al
    movzx eax, cx      ; EAX = CX (SPT + Cyl high bits)
    movzx ebx, dx      ; EBX = DX (Heads + Drives)

    ; detected_spt = CX & 0x3F (Bits 0-5)
    mov word ptr [detected_spt], ax
    and word ptr [detected_spt], 0x3F

    ; detected_heads = DH + 1
    movzx dx, dh       ; DX = DH
    inc dx             ; DX = DH + 1
    mov word ptr [detected_heads], dx

    ; detected_cylinders = CH | ((CX & 0xC0) << 2) + 1 (Max Cylinder 0 based + 1)
    ; Max Cylinder = CH | ((CX & 0xC0) << 2)
    movzx cx, ch       ; CX = CH
    mov word ptr [detected_cylinders], cx
    movzx eax, ax      ; EAX = AX (CX)
    and eax, 0xC0      ; Keep only high bits of cylinder in CX
    shl eax, 2         ; Shift them to bit 8 and 9
    or word ptr [detected_cylinders], ax ; Combine with CH
    inc word ptr [detected_cylinders] ; Toplam silindir sayısı (0-based Max Cyl + 1)

    ; Donus: 0 (başarı)
    xor ax, ax


.params_done:
    pop es ax dx cx bx
    pop bp
    ret

; Veri Bolumu (Assembly'de kullanilan gecici degiskenler)
; .section .data // Zaten yukarida tanimli


; inst_disk.S sonu