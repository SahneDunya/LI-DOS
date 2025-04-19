; boot.S
; Lİ-DOS Partition Boot Sector (PBS)
; Yazar: Sahne Dünya
; Hedef: 16-bit Real Mode, Intel 8086+
; Amac: Diskten Lİ-DOS Çekirdeğini yüklemek ve başlattırmak.
; NOT: Bu kod, kurulum programı tarafından bir partition'in ilk sektörüne (PBS) yazılır.

.code16                        ; 16-bit Real Mode kodu
.global _start                 ; Bootloader giriş noktası
.global vbpb_start             ; VBPB başlangıcı etiketi

.text
.org 0x7C00                    ; BIOS'un boot sektörü yüklediği adres

_start:
    cli                        ; Kesmeleri devre dışı bırak (güvenlik)
    xor ax, ax                 ; AX = 0
    mov ds, ax                 ; DS = 0
    mov es, ax                 ; ES = 0
    mov ss, ax                 ; SS = 0
    mov sp, 0x7C00             ; Stack pointeri geçici olarak boot sektorunun basina kur
                               ; (Kernel yuklenene kadar yeterli olmali)
    ; Veya stacki daha yuksek bir adrese kurun, örn: mov sp, 0x9000

    mov [boot_drive_id], dl    ; BIOS'un DL'de verdiği boot sürücü ID'sini kaydet

    ; VBPB'den geometri bilgilerini oku
    movzx ax, word [vbpb_start + 0x18] ; AX = sectors per track (Offset 24)
    mov [boot_spt], ax
    movzx ax, word [vbpb_start + 0x1A] ; AX = number of heads (Offset 26)
    mov [boot_heads], ax

    ; Kurulum programının yamaladığı çekirdek konumu ve boyutu bilgilerini oku
    ; BOOT_SECTOR_KERNEL_LBA_OFFSET ve BOOT_SECTOR_KERNEL_SIZE_OFFSET
    ; offsetleri boot/boot.S kodunuzda belirlediğiniz yerlere göre ayarlanmalıdır!
    mov eax, dword [boot_sector_code + BOOT_SECTOR_KERNEL_LBA_OFFSET]   ; EAX = Kernel Başlangıç LBA (32-bit)
    mov dword [kernel_start_lba], eax
    mov eax, dword [boot_sector_code + BOOT_SECTOR_KERNEL_SIZE_OFFSET]  ; EAX = Kernel Boyutu (Byte)
    mov dword [kernel_size_bytes], eax

    ; Yükleme bilgilerini ayarla
    mov ax, KERNEL_LOAD_SEGMENT ; ES = Yüklenecek segment (örn. 0x1000)
    mov es, ax
    xor bx, bx                 ; BX = 0 (Offset 0) -> ES:BX hedef adres (0x1000:0x0000)

    mov ecx, dword [kernel_size_bytes] ; ECX = Yüklenecek toplam byte sayisi
    xor edx, edx               ; EDX = 0
    mov word ptr [temp_sector_size], SECTOR_SIZE ; Sektor boyutu
    mov ebx, dword [temp_sector_size] ; EBX = Sektor boyutu
    div ebx                    ; EAX = Toplam sektor sayisi (byte/sektor), EDX = Kalan byte
    movzx cx, ax               ; CX = Yüklenecek toplam sektor sayisi
    test edx, edx              ; Kalan byte var mi?
    jz .sector_count_ok
    inc cx                     ; Varsa bir sektor daha yukle
.sector_count_ok:
    mov [sectors_to_load], cx  ; Yüklenecek sektor sayisini kaydet

    ; Yükleme döngüsü
    mov esi, dword [kernel_start_lba] ; ESI = Yuklenecek ilk sektorun LBA adresi (32-bit)
    movzx edi, word [sectors_to_load] ; EDI = Yüklenecek kalan sektor sayisi

.load_sector_loop:
    cmp edi, 0                 ; Tüm sektorler yuklendi mi?
    je .load_done              ; Evet ise bitir

    push esi                   ; ESI'yi (LBA) stack'e kaydet

    ; LBA -> CHS Çevrimi yap (Assembly'de, VBPB'den okunan geometri ile)
    ; Registers on entry: EAX = LBA, DL = Drive ID (from saved boot_drive_id)
    ; Returns: CX = (Cyl high bits << 6) | Sector (1 based), DH = Head (0 based), DL = Drive ID
    mov eax, dword [esi]       ; EAX = Güncel LBA
    mov dl, [boot_drive_id]    ; DL = Boot Drive ID

    ; LBA -> CHS macro'su (inst_disk.S'tekine benzer)
    ; Dikkat: Bu macro'nun global SPT/Heads yerine buradaki [boot_spt]/[boot_heads] kullanmasi gerekir.
    ; Basitlik icin macro icinde o global adresler yerine bu adresleri kullanacagini varsayalim.
    ; VEYA CHS cevirimini C'de (installer.c) yapip kernel LBA/size yerine kernel CHS/count yamanir.
    ; LBA yamalamak daha yaygın, bu nedenle Assembly'de CHS çevrimi daha olası.

    ; LBA -> CHS Çevrim (Manuel Assembly)
    push eax edx ebx           ; Kaydet
    mov ebx, dword [boot_spt]  ; EBX = Sectors Per Track
    test ebx, ebx ; SPT 0 ise hata olabilir
    jz .load_error
    xor edx, edx               ; EDX = 0
    div ebx                    ; EAX = LBA / SPT, EDX = LBA % SPT

    mov cl, dl                 ; CL = EDX (Sector 0 based)
    inc cl                     ; CL = Sector number (1 based)

    mov ebx, dword [boot_heads] ; EBX = Number of Heads
    test ebx, ebx ; Heads 0 ise hata olabilir
    jz .load_error
    xor edx, edx               ; EDX = 0
    div ebx                    ; EAX = (LBA / SPT) / Heads (Cyl), EDX = (LBA / SPT) % Heads (Head)
    mov dh, dl                 ; DH = EDX (Head number)

    mov ch, al                 ; CH = EAX (Cylinder lower 8 bits)
    shr eax, 8                 ; Shift EAX right by 8
    and eax, 0x03              ; Keep only bits 8 and 9 of Cylinder
    shl eax, 6                 ; Shift these 2 bits to bit 6 and 7
    or cl, al                  ; Combine with CL (Sector 1-6, Cylinder 8-9)

    mov dl, [boot_drive_id]    ; DL = Boot Drive ID
    pop ebx edx eax            ; Geri yukle (EAX tekrar LBA oldu)


    ; BIOS int 13h AH=02h cagrisi
    mov ah, 0x02               ; AH = Okuma fonksiyon kodu
    mov al, 1                  ; AL = Okunacak sektör sayısı (her seferinde 1 sektör)
                               ; count degeri tek seferde okunacak sektor sayisi olabilir.
                               ; Simdi tek tek sektor okuyalim.
                               ; AL = [sectors_to_load] ; Tüm sektörleri tek seferde yüklemek için.
                               ; Eğer tüm sektörler tek seferde okunacaksa döngüye gerek kalmazdı.
                               ; Tek sektör okuyup target buffer offseti artırmak daha esnek.

    int 0x13                   ; BIOS Disk Servisi

    jnc .read_ok               ; Carry Flag set değilse (hata yok)
    ; Hata durumunda
.load_error:
    ; Basit hata mesaji goster ve dur
    mov si, load_error_msg     ; SI = Mesaj stringinin offseti (DS=0 oldugu varsayilir)
    call print_string_0_7C00   ; Mesaji ekrana yaz (0x7C00 segmentinde calisan fonksiyon)
    jmp .halt                  ; Dur

.read_ok:
    ; Yükleme başarılı, yükleme adresini ve kalan sektör sayısını güncelle
    add bx, SECTOR_SIZE        ; BX = hedef buffer offsetini SECTOR_SIZE kadar artir (ES:BX)
    dec word ptr [sectors_to_load] ; Kalan sektör sayısını azalt
    inc esi                    ; ESI = Bir sonraki LBA'ya geç
    jmp .load_sector_loop      ; Döngüye devam et

.load_done:
    ; Yükleme tamamlandı, kernela atla (0x1000:0x0000)
    ; DS, ES, SS, SP şu anda 0x0000:0x7C00 veya 0x9000 civarında olabilir.
    ; Kernel, head.S içinde kendi stackini ve segmentlerini kuracak.
    mov ax, KERNEL_LOAD_SEGMENT ; AX = Kernel segmenti (0x1000)
    push ax                    ; Segmenti stack'e it
    xor ax, ax                 ; AX = 0 (Offset)
    push ax                    ; Offseti stack'e it
    retf                       ; Stack'ten segment:offset alıp long jump yap.
                               ; -> CS=0x1000, IP=0x0000 (Kernelin baslangici)


.halt:
    cli
    hlt                        ; Sistemi durdur

; --- Yardımcı Fonksiyonlar (Bootloader içinde basit G/Ç) ---
; int 10h kullanarak string yazdırma
print_string_0_7C00:
    push ax bx es si bp
    mov bp, sp
    ; DS'in 0 olduğu varsayılır, SI string offseti
    mov bx, 0x0007 ; Page 0, Attribute 7 (light grey on black)
    ; mov es, ax ; DS de 0 varsayildigi icin gerek yok

.loop_print:
    mov al, [si]   ; Karakteri al
    cmp al, 0      ; Null terminator mu?
    je .done_print ; Evet ise bitir
    mov ah, 0x0E   ; BIOS int 10h Function 0Eh: Write char in Teletype mode
    int 0x10       ; BIOS Video Service
    inc si         ; Sonraki karaktere geç
    jmp .loop_print

.done_print:
    pop bp si es bx ax
    ret


; --- VBPB Alanları ---
; MS-DOS uyumluluğu için standart VBPB yapısı (installer tarafından kullanılır/yamalanır)
vbpb_start:
    .byte 0xEB, 0x3C, 0x90 ; Jump instruction (örn. JMP SHORT + 3C, NOP)
    .ascii "LIDOS   "     ; OEM Name (8 bytes)
    .word SECTOR_SIZE      ; Bytes Per Sector (512)
    .byte 1                ; Sectors Per Cluster (Genellikle 1 disketlerde, HD'de degisir)
    .word 1                ; Reserved Sectors (Genellikle 1 - Boot Sector)
    .byte 2                ; Number of FATs (Genellikle 2)
    .word 224              ; Root Entry Count (Disketlerde 224, HD'de VBPB'den alinir)
    .word 2880             ; Total Sectors 16 (1.44MB disket icin, HD'de VBPB'den alinir)
    .byte 0xF0             ; Media Descriptor (1.44MB disket icin)
    .word 9                ; Sectors Per FAT 16 (Disketlerde 9)
    .word 18               ; Sectors Per Track (Disketlerde 18) -> YAMALANACAK!
    .word 2                ; Number of Heads (Disketlerde 2) -> YAMALANACAK!
    .long 0                ; Hidden Sectors (Bu bir partition boot sector, gizli sektor 0'dir veya partition LBA'sidir - Disk LBA 0 ise 0)
                            ; Installer tarafindan partition LBA'si ile yamalanabilir.
    .long 0                ; Total Sectors 32 (Eger Total Sectors 16=0 ise gecerli)

    ; Extended Boot Record (Offset 36'dan baslar FAT16 icin)
    .byte 0x00             ; BIOS Drive Number (Installer yazar veya 0x80 olur)
    .byte 0                ; Flags
    .byte 0x29             ; Extended Boot Signature (0x28 or 0x29)
    .long 0x12345678       ; Volume Serial Number (Ornek)
    .ascii "LIDOS VOLUME" ; Volume Label (11 bytes)
    .ascii "FAT12   "     ; File System Type Label (8 bytes) - "FAT16   " olabilir.

    ; --- Bootloader Kodu ve Verisi ---
    ; VBPB alanlari bittikten sonra baslar.
    boot_sector_code:
    ; BOOT_SECTOR_KERNEL_LBA_OFFSET = . - _start
    ; BOOT_SECTOR_KERNEL_SIZE_OFFSET = . - _start
    ; Bu offsetler, kodun VBPB sonrasi baslangicina gore hesaplanmali.
    ; Baslangic _start etiketi 0x7C00'dur. VBPB 0x7C00'de baslar.
    ; Kod VBPB sonrasinda baslar.
    ; VBPB boyutu (Extended kisim dahil) yaklasik 62 byte civarindadir.
    ; Kod yaklasik 0x7C3E'den baslar.
    ; Offset = (Current Address) - 0x7C00

    ; Buraya loader kodunuz gelecek. Yukaridaki loader kod ornegi buraya tasinmali.
    ; Ornegin, yukaridaki loader kodunu kopyalayıp bu comment alanına yapıştırın.
    ; Ve BOOT_SECTOR_KERNEL_LBA_OFFSET / SIZE_OFFSET'i kodun icindeki yerlerine gore hesaplayin.

    ; Installer'in yamalayacagi alanlari belirle
    kernel_start_lba:
        .long 0x00000000   ; Yüklenecek çekirdeğin başlangıç LBA'sı (Yamalanacak!)
    kernel_size_bytes:
        .long 0x00000000   ; Yüklenecek çekirdeğin boyutu (Byte) (Yamalanacak!)

    ; Loader kodunun geri kalanı...
    ; ... (Yukaridaki loader kod ornegi buraya devam edecek) ...

    ; Loader kodunun kullandigi veri
    boot_drive_id db 0         ; Boot edilen sürücü ID'si
    boot_spt dw 0              ; VBPB'den okunan SPT
    boot_heads dw 0            ; VBPB'den okunan Heads
    sectors_to_load dw 0       ; Yuklenecek toplam sektor sayisi
    temp_sector_size dw SECTOR_SIZE ; Sektor boyutu (hesaplama icin)

    load_error_msg db "Kernel yukleme hatasi!", 0 ; Hata mesajı


    ; Yeterli alan bırakarak 510. byte'a kadar dolgu yap
    . = _start + 510
    .word 0xAA55               ; Boot Signature (Offset 510)

; boot.S sonu