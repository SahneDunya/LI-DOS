; inst_head.S
; Lİ-DOS Kurulum Programı Başlangıç Noktası
; Yazar: Sahne Dünya
; Hedef: 16-bit Real Mode, Intel 8086+
; Amac: Boot edilebilir başlangıç, temel kurulum ve C koduna geçiş.

.code16                ; 16-bit Real Mode kodu
.global _start         ; Bootloader/Entry point label
.global c_installer_main ; C ana fonksiyonu

.text
.org 0x7C00            ; Boot sektörü başlangıç adresi

_start:
    ; Temel registerları ve segmentleri ayarla
    ; BIOS zaten ES, DS, SS, SP'yi kurmuş olabilir ama guvenli olsun
    xor ax, ax         ; AX = 0
    mov ds, ax         ; DS = 0
    mov es, ax         ; ES = 0 (IVT erisimi icin veya VRAM)
    mov ss, ax         ; SS = 0
    mov sp, 0x7C00     ; Stack pointeri boot sektorunun basina kur (kod yukariya buyuyecek)
                       ; Veya 0x9000 gibi daha yuksek bir adres de stack icin kullanilabilir.
                       ; Ornek: mov sp, 0x9000

    ; Kurulum programının yüklendiği segmenti bilmek önemli olabilir
    ; CS otomatik olarak boot sektoru segmentine ayarlanir (0x07C0 eger 0x7C00'e yuklendiyse)
    ; Eger veri segmentini CS ile ayni yapacaksak:
    ; mov ax, cs
    ; mov ds, ax

    ; Kesmeleri devre disi birak (Kurulum sirasinda kesmeler istenmeyebilir)
    cli

    ; BIOS Disk Sürücüsü Numarasını al (hangi sürücüden boot edildi)
    ; BIOS DL registerinda boot edilen sürücü numarasını sağlar (floppy 0 veya 1, HDD 0x80+)
    ; Bu bilgi, kurulum programının hangi diskten kendisini okuduğunu bilmesi için önemlidir.
    ; Bunu bir değişkene kaydet veya C'ye argüman olarak geç. C'ye geçirmek C89'da zor olabilir.
    ; Global bir değişkene kaydedelim.
    mov [boot_drive_id], dl ; Boot drive ID'yi kaydet

    ; C ana fonksiyonunu çağır
    call c_installer_main

    ; C fonksiyonu döndüğünde (ki dönmemesi beklenir) veya kurulum bitince
    ; Sistemi durdur.
    cli                ; Kesmeleri tekrar kapat
    hlt                ; CPU'yu durdur

; Veri Bolumu
.section .data
    boot_drive_id db 0 ; Boot edilen sürücünün ID'si

; boot sektoru imzasi (0xAA55) - Boot edilebilir yapmak icin
.org 0x7DFE            ; 0x7C00 + 0x1FE = 0x7DFE
    .word 0xAA55       ; Boot Signature

; inst_head.S sonu