// kernel_image_bin.h
// Lİ-DOS Çekirdek İmajı İkili Verisi
// BU DOSYA GENELLIKLE BUILD SÜRECINDE OTOMATIK OLARAK OLUŞTURULUR!
// Yazar: Sahne Dünya
// Hedef: Kurulum programi tarafindan kullanilmak.

#ifndef _KERNEL_IMAGE_BIN_H
#define _KERNEL_IMAGE_BIN_H

// Temel tipler (kurulum programi bunlari include etmeli)
#include "inst.h" // inst.h icinde uint8_t, size_t vb. tanimli olmali.
// #include "types.h" // Eger inst.h ayri types.h'i include ediyorsa veya direkt buraya konur.

// Lİ-DOS Çekirdek İmajının Boyutu (Byte Cinsinden)
// Bu boyut, kernelin derlenmis bin dosyasinin tam boyutudur.
// Build scripti tarafindan otomatik hesaplanmali ve buraya yazilmalidir.
#define KERNEL_IMAGE_SIZE 16384 // Örnek boyut (16 KB), gercek boyuta göre degisir. 64KB'i aşamaz.

// Lİ-DOS Çekirdek İmajının Ham İkili Verisi
// Bu dizi, kernelin derlenmis bin dosyasinin byte byte icerigidir.
// Build scripti tarafindan kernel.bin dosyasindan okunarak doldurulmalidir.
const unsigned char kernel_image_data[KERNEL_IMAGE_SIZE] = {
    // Örnek placeholder ikili veri. Gerçek veriler buraya gelecek.
    // Bu byte'lar, çekirdek kodunuzun, verinizin, stack'inizin derlenmiş halidir.
    0xEB, 0xFE,       // Jump back (infinite loop - ornek)
    0x90, 0x90,       // NOPs
    // ... cekirdek kodunuzun baslangici ...
    0x31, 0xC0,       // xor ax, ax
    0x8E, 0xD8,       // mov ds, ax
    0x8E, 0xC0,       // mov es, ax
    0xBE, 0x00, 0x80, // mov si, 0x8000 (ornek adres)
    0xB9, 0xFF, 0x7F, // mov cx, 0x7FFF (ornek boyut)
    0xF2, 0xAE,       // rep movsb (ornek memory move)
    // ... kernelinizin kod ve veri bolumlerinin devam eden byte'lari ...
    // Build scripti tum kernel bin dosyasini byte byte okuyup buraya yazar.
    // Ornegin 16KB (16384 byte) kernel imaji icin 16384 adet hex byte degeri:
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    // ... Devam eden satirlar ...
    // Array boyutu KERNEL_IMAGE_SIZE ile eslesmelidir.
    // Son byte'lar cekirdek verisi veya BSS sonu olabilir.
    // ... Son byte'lar ...
    0xAA, 0x55 // Ornek son 2 byte (imza gibi) - kernelin kendisinde imza olmasi gerekmez.
               // Boot sektoru 0xAA55 imzasina bakar.

    // Array boyutu KERNEL_IMAGE_SIZE'a tam eslesmelidir.
};

#endif // _KERNEL_IMAGE_BIN_H