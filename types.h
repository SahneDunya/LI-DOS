// types.h
// Lİ-DOS Temel Veri Tipi Tanımları
// Yazar: Sahne Dünya
// Hedef: 16-bit Real Mode, Intel 8086+, C89
// Amac: Mimariye ozel sabit genislikli integer tipleri ve size_t tanimi.

#ifndef _LIDOS_TYPES_H
#define _LIDOS_TYPES_H

/*
 * Temel integer tipleri.
 * 16-bit Real Mode x86 mimarisinde genellikle:
 * char = 8 bit
 * short = 16 bit
 * int = 16 bit
 * long = 32 bit
 */

typedef unsigned char  uint8_t;
typedef signed char    int8_t;
typedef unsigned short uint16_t;
typedef signed short   int16_t;
typedef unsigned long  uint32_t;
typedef signed long    int32_t;

/*
 * size_t tipi.
 * sizeof() operatorunun dondurdugu isaretsiz tiptir.
 * 16-bit mimaride genellikle 16-bit'tir.
 */
typedef unsigned short size_t;

/*
 * NULL tanimi.
 * Boş pointer sabiti.
 */
#ifndef NULL
#define NULL ((void *)0)
#endif

#endif // _LIDOS_TYPES_H