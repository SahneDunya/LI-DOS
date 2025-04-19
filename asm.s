; asm.S
; Lİ-DOS Genel Assembly Yardımcı Fonksiyonları
; Yazar: Sahne Dünya
; Hedef: 16-bit Real Mode, Intel 8086+
; Kernel'in C kodundan cagrilacak dusuk seviyeli fonksiyonlar.

.code16                ; 16-bit Real Mode kodu oldugunu belirtir

; Bu fonksiyonlari C kodundan erisebilmek icin global yap
.global inb            ; Byte oku (porttan)
.global outb           ; Byte yaz (porta)
.global inw            ; Word oku (porttan)
.global outw           ; Word yaz (porta)
.global insw           ; Word serisi oku (porttan)
.global outsw          ; Word serisi yaz (porta)
.global cli            ; Kesmeleri kapat
.global sti            ; Kesmeleri ac
.global hlt            ; CPU'yu durdur
.global io_delay       ; Kisa bir G/Ç gecikmesi

.text                  ; Kod bolumu

; unsigned char inb(unsigned short port)
; Bir I/O portundan 1 byte okur.
; Parametreler (stack'te): [bp+4] = port (16-bit)
; Donus degeri: AL (8-bit) / AX (16-bit, AL'nin zero-extended hali)
inb:
    push bp            ; BP registerini stack'e kaydet
    mov bp, sp         ; BP'yi guncel SP'ye ayarla (stack frame olustur)
    mov dx, [bp+4]     ; Port adresini stack'ten DX'e yukle
    in al, dx          ; DX portundan 1 byte oku, sonucu AL'e koy
    movzx ax, al       ; AL'yi AX'e zero-extend yap (C 16-bit bekleyebilir)
    pop bp             ; BP registerini stack'ten geri yukle
    ret                ; Fonksiyondan don

; void outb(unsigned short port, unsigned char value)
; Bir I/O portuna 1 byte yazar.
; Parametreler (stack'te): [bp+6] = port (16-bit), [bp+4] = value (8-bit)
outb:
    push bp            ; BP'yi stack'e kaydet
    mov bp, sp         ; BP'yi ayarla
    mov dx, [bp+6]     ; Port adresini DX'e yukle
    mov al, [bp+4]     ; Yazilacak degeri AL'e yukle
    out dx, al         ; AL degerini DX portuna yaz
    pop bp             ; BP'yi geri yukle
    ret                ; Fonksiyondan don

; unsigned short inw(unsigned short port)
; Bir I/O portundan 1 word okur.
; Parametreler (stack'te): [bp+4] = port (16-bit)
; Donus degeri: AX (16-bit)
inw:
    push bp
    mov bp, sp
    mov dx, [bp+4]     ; Port adresini DX'e yukle
    in ax, dx          ; DX portundan 1 word oku, sonucu AX'e koy
    pop bp
    ret

; void outw(unsigned short port, unsigned short value)
; Bir I/O portuna 1 word yazar.
; Parametreler (stack'te): [bp+6] = port (16-bit), [bp+4] = value (16-bit)
outw:
    push bp
    mov bp, sp
    mov dx, [bp+6]     ; Port adresini DX'e yukle
    mov ax, [bp+4]     ; Yazilacak degeri AX'e yukle
    out dx, ax         ; AX degerini DX portuna yaz
    pop bp
    ret

; void insw(unsigned short port, void *buffer, unsigned short count)
; Bir I/O portundan 'count' adet word okur ve 'buffer' adresine yazar.
; Buffer adresi 16-bit bir FAR pointer olmalidir (segment:offset).
; Parametreler (stack'te): [bp+8] = port (16-bit), [bp+6] = buffer_segment (16-bit), [bp+4] = buffer_offset (16-bit), [bp+2] = count (16-bit) -- (Bu parametre sirasi C'de tersten itildigini varsayar)
insw:
    push bp
    mov bp, sp
    push ds            ; DS'yi kaydet (ES'ye ihtiyacimiz var, DS bozulmasin)

    mov dx, [bp+8]     ; Port adresini DX'e yukle
    mov es, [bp+6]     ; Buffer segmentini ES'e yukle
    mov di, [bp+4]     ; Buffer offsetini DI'a yukle (ES:DI hedef adres)
    mov cx, [bp+2]     ; Okunacak word sayisini CX'e yukle

    ; STD veya CLD bayraklarina dikkat edin. String islemleri icin genellikle CLD (artan adres) kullanilir.
    cld                ; DI'yi artirma yonunde ayarla

    rep insw           ; CX sayisi kadar, DX portundan ES:DI'ya word oku
                       ; Her okumadan sonra CX azalir, DI artar.

    pop ds             ; Kaydedilen DS'yi geri yukle
    pop bp
    ret

; void outsw(unsigned short port, const void *buffer, unsigned short count)
; Bir I/O portuna 'count' adet word yazar, veriyi 'buffer' adresinden alir.
; Buffer adresi 16-bit bir FAR pointer olmalidir (segment:offset).
; Parametreler (stack'te): [bp+8] = port (16-bit), [bp+6] = buffer_segment (16-bit), [bp+4] = buffer_offset (16-bit), [bp+2] = count (16-bit)
outsw:
    push bp
    mov bp, sp
    push es            ; ES'yi kaydet (DS'ye ihtiyacimiz var, ES bozulmasin)

    mov dx, [bp+8]     ; Port adresini DX'e yukle
    mov ds, [bp+6]     ; Buffer segmentini DS'e yukle
    mov si, [bp+4]     ; Buffer offsetini SI'a yukle (DS:SI kaynak adres)
    mov cx, [bp+2]     ; Yazilacak word sayisini CX'e yukle

    ; STD veya CLD bayraklarina dikkat edin. String islemleri icin genellikle CLD (artan adres) kullanilir.
    cld                ; SI'yi artirma yonunde ayarla

    rep outsw          ; CX sayisi kadar, DS:SI'dan DX portuna word yaz
                       ; Her yazmadan sonra CX azalir, SI artar.

    pop es             ; Kaydedilen ES'yi geri yukle
    pop bp
    ret

; void cli(void)
; Kesmeleri kapatir.
cli:
    cli                ; Disable interrupts
    ret

; void sti(void)
; Kesmeleri acar.
sti:
    sti                ; Enable interrupts
    ret

; void hlt(void)
; CPU'yu durdurur (kesme gelene kadar veya reset).
hlt:
    hlt                ; Halt CPU
    ret

; void io_delay(void)
; Kisa bir G/Ç gecikmesi saglar. Bazi donanimlarla iletisim kurarken gereklidir.
; Bos bir islem veya guvenli bir port okuma kullanilabilir.
; Ornek: ACPI PM Timer portu (0x80) veya basit bir jmp.
; 8086 ile uyumlu olmasi icin bos dongu daha guvenlidir.
io_delay:
    push cx            ; CX'i kaydet (dongu sayaci kullanacagiz)
    mov cx, 1          ; Kisa bir gecikme icin kucuk bir sayi
.loop:
    loop .loop         ; CX sıfır olana kadar döner
    pop cx             ; CX'i geri yükle
    ret

; context_switch fonksiyonu (ornegin asm.S icine eklenir)
; Eski gorevin baglamini kaydeder, yeni gorevin baglamini yukler.
; Yazar: Gemini (Orenk Kod)
; Hedef: 16-bit Real Mode

.code16

.global context_switch
; void context_switch(struct task_context *old_ctx, struct task_context *new_ctx)
; Parametreler (stack'te, C'den tersten itildigi varsayilir):
; [bp+6]: new_ctx (pointer, 16-bit offset)
; [bp+4]: old_ctx (pointer, 16-bit offset)
; Not: Kernel-only oldugu icin segment registerlari (DS, ES) ayni olabilir.
; Ancak genellestirmek icin segmentleri de kaydedip yukleyecegiz.
; Pointerlarin ayni segmentte oldugunu varsayiyoruz (DS veya SS gibi).
; Gercek implementasyonda pointerlarin segmenti de stack'ten alinmalidir.
; Bu ornekte basitlestermek icin BP+n offsetler dogrudan kullanildi,
; struct pointerinin segmenti DS veya ES gibi bir registerda olmali.
; Daha dogru C cagri kuralinda far pointerlar segment ve offset olarak gelir.

context_switch:
    push bp            ; BP'yi kaydet
    mov bp, sp         ; Stack frame olustur

    ; old_ctx pointerini al (stack'ten)
    mov bx, [bp+4]     ; BX = old_ctx offseti (Segment DS veya baska bir seg reg olabilir)

    ; Mevcut gorevin registerlarini ve flaglerini stack'e kaydet (schedule'dan onceki hali degil, context_switch'e giristeki hali)
    ; pushf          ; Flags registerini kaydet - IRET ile doneceksek stackte olmali.
    push es            ; ES segment registerini kaydet
    push ds            ; DS segment registerini kaydet
    push ax            ; AX registerini kaydet
    push cx            ; CX registerini kaydet
    push dx            ; DX registerini kaydet
    push bx            ; BX registerini kaydet
    push sp            ; SP registerini kaydet (Bu save edilen SP, bu noktadan sonraki stack'i gosterir)
    push bp            ; BP registerini kaydet (Bu save edilen BP, context_switch'in stack frame'ini gosterir)
    push si            ; SI registerini kaydet
    push di            ; DI registerini kaydet
    ; NOT: pusha 80186+ bir komuttur, 8086/8088 icin tek tek push yapilmalidir.
    ; Ornek kodda 8086 uyumluluk icin tek tek push kullanildi.

    ; SP ve SS registerlarini old_ctx yapisina kaydet
    mov word [bx + offsetof(struct task_context, sp)], sp ; Guncel SP'yi kaydet
    mov word [bx + offsetof(struct task_context, ss)], ss ; Guncel SS'yi kaydet

    ; Diger registerlari old_ctx yapisina kaydet (stack'ten simdi push ettiklerimiz)
    ; Stack'ten pop sirasinin tersi sirayla alip yapiya yaz.
    ; BP'yi stack'ten alirken dikkatli olmali, cunku 'mov bp, sp' ile BP degisti.
    ; Stack'in o anki tepesindeki degerler kaydedilen registerlardir.
    ; Veya pusha kullanip sonra stack'ten okuyup yapiya yazilabilir (80186+).
    ; 8086 uyumluluk icin push yaptiktan sonra stack pointeri kullanarak stack'teki degerleri alalim:
    mov word [bx + offsetof(struct task_context, di)], [sp + 0*2] ; DI stack'in tepesi
    mov word [bx + offsetof(struct task_context, si)], [sp + 1*2] ; SI
    mov word [bx + offsetof(struct task_context, bp)], [sp + 2*2] ; BP
    ; SP stack'e kaydedildi: [sp + 3*2]
    mov word [bx + offsetof(struct task_context, bx)], [sp + 4*2] ; BX
    mov word [bx + offsetof(struct task_context, dx)], [sp + 5*2] ; DX
    mov word [bx + offsetof(struct task_context, cx)], [sp + 6*2] ; CX
    mov word [bx + offsetof(struct task_context, ax)], [sp + 7*2] ; AX
    mov word [bx + offsetof(struct task_context, ds)], [sp + 8*2] ; DS
    mov word [bx + offsetof(struct task_context, es)], [sp + 9*2] ; ES
    ; Flags stack'e kaydedildi (pushf yapildiysa): [sp + 10*2]
    ; CS ve IP stack'te (call context_switch tarafindan): [sp + 11*2], [sp + 12*2]

    ; CS ve IP'yi old_ctx yapisina kaydet (call context_switch'in return addressi)
    ; Call instruction stack'e IP sonra CS koyar. SP'nin o anki konumu dikkate alinmalidir.
    ; pusha+pushf yapildigini varsayarsak, CS ve IP SP+22 ve SP+24'te olur.
    ; pusha yapmadan tek tek push yaptiysak ve pushf yapmadiysak, durum farkli olur.
    ; Yukarıdaki push'lara Flags, CS, IP ekleyelim stack'te olsunlar:
    ; pushf ; Flags
    ; call_instruction_return_address = CS:IP -- bu call context_switch tarafindan stack'e konur.
    ; Stack simdi soyle (SP'den yukariya): DI, SI, BP, SP_val, BX, DX, CX, AX, DS_val, ES_val, Flags_val, IP_ret, CS_ret
    ; Stack'in tepesi pusha'dan sonraki SP'dir.
    ; C'den call yapildiginda donus adresi (CS:IP) stack'e konur. Sonra 'push bp' vb. gelir.
    ; context_switch fonksiyonunun stack frame'i soyle baslar:
    ; [bp+4]  -> old_ctx pointer
    ; [bp+6]  -> new_ctx pointer
    ; [bp+2]  -> Saved BP (onceki stack frame'in BP'si)
    ; [bp+0]  -> Return IP (call'dan gelen IP)
    ; [-2]    -> Return CS (call'dan gelen CS)
    ; [-4..]  -> Pushlanmis registerlar (pusha / tek tek push)

    ; Stack layoutini dogru anlayip CS:IP'yi alalim.
    ; C'den 'call context_switch' yapildiginda:
    ; Stack: ... | arg_new_ctx | arg_old_ctx | ret_IP | ret_CS | saved_BP | saved_DI | ... | saved_AX | saved_DS | saved_ES | (saved_Flags?) | ... (pusha/pushf sonrasi)
    ; BP simdi saved_BP'yi isaret ediyor. Stack tepesi (SP) en son push yapilan yer.
    ; ret_CS ve ret_IP, saved_BP'nin 2 byte (word) altinda ve 4 byte altinda.
    mov word [bx + offsetof(struct task_context, cs)], [bp-2] ; Saved CS'yi kaydet
    mov word [bx + offsetof(struct task_context, ip)], [bp-4] ; Saved IP'yi kaydet
    ; saved_BP kendisi [bp+0] adresinde. Saved_DI [bp-6], Saved_SI [bp-8] ... saved_AX [bp-20]
    ; saved_ES [bp-22], saved_DS [bp-24]. Flags [bp-26] (eger pushf yapildiysa).

    ; old_ctx yapisindaki register alanlarini stack'ten alip dolduralim:
    mov word [bx + offsetof(struct task_context, di)], [bp-6]
    mov word [bx + offsetof(struct task_context, si)], [bp-8]
    mov word [bx + offsetof(struct task_context, bx)], [bp-14] ; BP, SP atlandi
    mov word [bx + offsetof(struct task_context, dx)], [bp-16]
    mov word [bx + offsetof(struct task_context, cx)], [bp-18]
    mov word [bx + offsetof(struct task_context, ax)], [bp-20]
    mov word [bx + offsetof(struct task_context, ds)], [bp-24]
    mov word [bx + offsetof(struct task_context, es)], [bp-22]
    ; Flags (eger pushf yapildiysa)
    ; mov word [bx + offsetof(struct task_context, flags)], [bp-26]


    ; new_ctx pointerini al (stack'ten)
    mov bx, [bp+6]     ; BX = new_ctx offseti

    ; Yeni gorevin SS ve SP registerlarini yukle
    mov sp, [bx + offsetof(struct task_context, sp)] ; SP'yi yukle
    mov ss, [bx + offsetof(struct task_context, ss)] ; SS'yi yukle

    ; Yeni gorevin diger registerlarini ve flaglerini stack'ten (yeni stack) geri yukle
    ; Kaydedilenleri ters sirayla pop yapacagiz.
    ; Yeni stack'in tepesi, yeni gorev en son durdugunda kaydedilenlerin oldugu yer olmali.
    ; context_switch'in sonunda 'retf' ile donecegiz, bu da yeni stack'ten IP ve CS'yi cekecektir.
    ; Yani yeni stack'in tepesinde Flags, sonra CS, sonra IP olmali.
    ; Ardindan pushlanan registerlar olmali (popa sirasinin tersi).
    ; yeni_ctx yapisindaki degerleri kullanarak yeni stack'i kurduk (veya create_task'te kurmaliydik).

    ; Simdi yeni stack'ten registerlari poplayalim
    ; popf ; Flags
    pop es             ; ES segment registerini geri yukle
    pop ds             ; DS segment registerini geri yukle
    pop ax             ; AX registerini geri yukle
    pop cx             ; CX registerini geri yukle
    pop dx             ; DX registerini geri yukle
    pop bx             ; BX registerini geri yukle
    pop sp             ; SP registerini geri yukle (Bu, kaydedilen SP degeri olur, yani context_switch oncesi SP)
    pop bp             ; BP registerini geri yukle
    pop si             ; SI registerini geri yukle
    pop di             ; DI registerini geri yukle

    ; Flags registerini yukle (eger pushf yapildiysa ve popf yapilmadiysa)
    ; mov ax, [bx + offsetof(struct task_context, flags)] ; Flags degerini al
    ; push ax            ; Flags degerini stack'e koy
    ; popf               ; Stack'ten flags'i yukle

    ; Yeni gorevin CS ve IP'sini yukle ve atla.
    ; 'retf' (far return) komutu stack'ten IP'yi sonra CS'yi ceker ve o adrese atlar.
    ; Bu nedenle, yeni gorevin stack'inin tepesinde (pop'lar yapildiktan sonra),
    ; context_switch cagrisindan onceki durumu temsil eden Flags, CS, IP olmalidir.
    ; Bunlar context_switch tarafindan kaydedildi ve yeni stack'e yuklendi (veya create_task'te ayarlandi).

    ; Yeni gorevin CS ve IP'sini stack'e koy
    push word [bx + offsetof(struct task_context, cs)] ; CS'yi stack'e it
    push word [bx + offsetof(struct task_context, ip)] ; IP'yi stack'e it

    ; Yeni gorevin Flags registerini stack'e koy (eger pushf yapilmadiysa)
    ; mov ax, [bx + offsetof(struct task_context, flags)] ; Flags degerini al
    ; push ax            ; Flags degerini stack'e koy

    retf               ; Stack'teki CS:IP'ye atla (Far Return)

; struct task_context alanlarinin offsetlerini hesaplamak icin yardimci macro/offset
; Bu ofsetler, struct task_context'in C'deki tanimina baglidir.
; Ornekteki struct tanimina gore (pusha sirasi + segmentler + CS:IP + SS):
; di     offset = 0
; si     offset = 2
; bp     offset = 4
; sp     offset = 6
; bx     offset = 8
; dx     offset = 10
; cx     offset = 12
; ax     offset = 14
; es     offset = 16
; ds     offset = 18
; flags  offset = 20
; ip     offset = 22
; cs     offset = 24
; ss     offset = 26  <-- struct tanimindaki siraya gore degisir.
; YUKARIDAKI ASSEMBLY KODU BU OFFSETLERE GORE DUZELTILMELIDIR!
; Ornek struct tanimindaki sira kullanilarak duzeltilmis offsetler:
.equ offsetof_di = 0
.equ offsetof_si = 2
.equ offsetof_bp = 4
.equ offsetof_sp = 6
.equ offsetof_bx = 8
.equ offsetof_dx = 10
.equ offsetof_cx = 12
.equ offsetof_ax = 14
.equ offsetof_es = 16
.equ offsetof_ds = 18
.equ offsetof_flags = 20
.equ offsetof_ip = 22
.equ offsetof_cs = 24
.equ offsetof_ss = 26

; Yukaridaki Assembly kodu bu offsetleri kullanmaliydi. Ornekte dogrudan stack offsetleri kullanilmis gibi duruyor.
; Duzeltilmis Assembly kodu (offsetof makrolari kullanarak):

context_switch_corrected:
    push bp            ; BP'yi kaydet
    mov bp, sp         ; Stack frame olustur

    ; old_ctx pointerini al (stack'ten)
    mov bx, [bp+4]     ; BX = old_ctx offseti (Segment DS veya baska bir seg reg olabilir)

    ; Mevcut gorevin registerlarini stack'e kaydet
    pushf              ; Flags
    push es            ; ES
    push ds            ; DS
    push ax            ; AX
    push cx            ; CX
    push dx            ; DX
    push bx            ; BX
    push sp            ; SP (context_switch'e giristeki SP, yani buraya kadar push yapildiktan sonraki SP)
    push bp            ; BP (cagiranin BP'si, [bp+0] adresinde)
    push si            ; SI
    push di            ; DI
    ; Stack simdi (SP'den yukariya): DI, SI, BP_caller, SP_before_this, BX, DX, CX, AX, DS_val, ES_val, Flags_val, IP_caller, CS_caller (call tarafindan)

    ; Stack pointeri SP'yi guncelle (push'lardan sonra)
    ; BP su an saved_BP'yi gosteriyor. Stack'teki kaydedilmis degerlere BP-offset ile erisebiliriz.
    ; Veya SP'nin o anki degeri kullanilabilir. SP su an en son push yapilan DI'yi gosteriyor.
    ; SP = Current_SP_after_all_pushes
    ; Saved BP is at [BP + 0]
    ; Return IP is at [BP + 2]
    ; Return CS is at [BP + 4]
    ; Parameters new_ctx is at [BP + 6], old_ctx is at [BP + 8]

    ; Stack layoutini yeniden incele. C'den 'call context_switch' yapildiginda:
    ; ... | arg_new_ctx | arg_old_ctx | Return IP | Return CS | Saved BP | <start of pusha/manual pushes>
    ;                                                     ^-- current BP points here
    ;                                                              ^-- current SP points here after 'push bp'

    ; Dogru stack layout ve kayit/yukleme sirasi (BP ve SP'nin C89 çağrısı sonrası konumu dikkate alınarak):
    ; Fonksiyon basinda: push bp; mov bp, sp
    ; Stack: ... | param2 | param1 | Ret IP | Ret CS | Caller BP | ...
    ;                                            ^-- BP su an burayı isaret ediyor

    ; Mevcut gorevin registerlarini kaydedin (stack'e veya yapiya dogrudan)
    ; Yapiya dogrudan kaydetmek icin BX=old_ctx pointer
    mov bx, [bp+4]     ; BX = old_ctx pointer (offset)

    mov word [bx + offsetof_cs], [bp+4] ; Ret CS (call tarafindan stacke kondu)
    mov word [bx + offsetof_ip], [bp+2] ; Ret IP (call tarafindan stacke kondu)
    mov word [bx + offsetof_bp], [bp+0] ; Caller BP

    ; Diger registerlari tek tek kaydet (pusha yerine 8086 uyumluluk)
    push ax; mov word [bx + offsetof_ax], [sp]; pop ax
    push cx; mov word [bx + offsetof_cx], [sp]; pop cx
    push dx; mov word [bx + offsetof_dx], [sp]; pop dx
    push bx; mov word [bx + offsetof_bx], [sp]; pop bx
    push si; mov word [bx + offsetof_si], [sp]; pop si
    push di; mov word [bx + offsetof_di], [sp]; pop di
    push es; mov word [bx + offsetof_es], [sp]; pop es
    push ds; mov word [bx + offsetof_ds], [sp]; pop ds
    pushf;  mov word [bx + offsetof_flags], [sp]; popf ; Flags

    ; SP ve SS'i kaydedin (BP'yi kurduktan sonraki guncel SP)
    mov word [bx + offsetof_sp], sp ; Guncel SP
    mov word [bx + offsetof_ss], ss ; Guncel SS

    ; new_ctx pointerini al
    mov bx, [bp+6]     ; BX = new_ctx pointer (offset)

    ; Yeni gorevin SS ve SP registerlarini yukle
    mov ss, [bx + offsetof_ss]
    mov sp, [bx + offsetof_sp]

    ; Yeni gorevin diger registerlarini yukle
    mov ax, [bx + offsetof_ax]; push ax; popa sirasinin tersi
    mov cx, [bx + offsetof_cx]; push cx
    mov dx, [bx + offsetof_dx]; push dx
    mov bx, [bx + offsetof_bx]; push bx
    mov si, [bx + offsetof_si]; push si
    mov di, [bx + offsetof_di]; push di
    mov es, [bx + offsetof_es]; push es
    mov ds, [bx + offsetof_ds]; push ds
    mov ax, [bx + offsetof_flags]; push ax; popf icin stacke koy
    mov bp, [bx + offsetof_bp]; push bp ; Stack frame'i kurmak icin BP

    ; Simdi stack'in tepesi, Flags, CS, IP olmali
    ; Flags'i yukle
    popf

    ; Stack'ten kaydedilen diger registerlari popla (BP hariç)
    pop bp ; BP
    pop di ; DI
    pop si ; SI
    pop bx ; BX
    pop dx ; DX
    pop cx ; CX
    pop ax ; AX
    pop ds ; DS
    pop es ; ES

    ; Yeni gorevin CS ve IP'sini stack'e koy ve retf ile atla
    push word [bx + offsetof_cs]
    push word [bx + offsetof_ip]

    retf ; Stack'teki CS:IP'ye atla

; context_switch_corrected sonu - YUKARIDAKI Assembly KODU BU SEKLINDE OLMALI
; offsetof'larin dogru oldugundan emin olun.
; asm.S sonu