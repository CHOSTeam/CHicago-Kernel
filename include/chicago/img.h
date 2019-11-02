// File author is Ítalo Lima Marconato Matias
//
// Created on April 17 of 2019, at 21:57 BRT
// Last edited on September 06 of 2019, at 18:27 BRT

#ifndef __CHICAGO_IMG_H__
#define __CHICAGO_IMG_H__

#include <chicago/types.h>

#define BITBLIT_MODE_COPY 0x01
#define BITBLIT_MODE_BLEND 0x02
#define BITBLIT_MODE_INVERT 0x04

#define FontStart (&_binary_font_psf_start)
#define FontEnd (&_binary_font_psf_end)

typedef struct {
	UInt8 b;
	UInt8 m;
	UInt32 sz;
	UInt16 res1;
	UInt16 res2;
	UInt32 off;
	UInt32 size;
	UInt32 width;
	UInt32 height;
	UInt16 planes;
	UInt16 bpp;
	UInt32 compression;
	UInt32 image_size;
	UInt32 xpels_per_meter;
	UInt32 ypels_per_meter;
	UInt32 clr_used;
	UInt32 clr_important;
} Packed BmpHeader, *PBmpHeader;

typedef struct {
	UInt32 magic;
	UInt32 version;
	UInt32 hdr_size;
	UInt32 flags;
	UInt32 num_glyph;
	UInt32 bytes_per_glyph;
	UInt32 height;
	UInt32 width;
} PCScreenFont, *PPCScreenFont;

typedef struct {
	UIntPtr width;
	UIntPtr height;
	UInt8 bpp;
	UIntPtr buf;
	Boolean user;
	Boolean align;
} Image, *PImage;

extern Char _binary_font_psf_start;
extern Char _binary_font_psf_end;

PImage ImgCreate(UIntPtr width, UIntPtr height, UInt8 bpp, Boolean user, Boolean align);
PImage ImgCreateBuf(UIntPtr width, UIntPtr height, UInt8 bpp, UIntPtr buf, Boolean user, Boolean align);
PImage ImgLoadBMPBuf(PUInt8 buf, Boolean user, Boolean align);
PImage ImgRescale(PImage src, UIntPtr width, UIntPtr height);
Void ImgExtractRGB(UIntPtr c, PUInt8 r, PUInt8 g, PUInt8 b);
Void ImgExtractARGB(UIntPtr c, PUInt8 a, PUInt8 r, PUInt8 g, PUInt8 b);
UIntPtr ImgCreateRGB(UInt8 r, UInt8 g, UInt8 b);
UIntPtr ImgCreateARGB(UInt8 a, UInt8 r, UInt8 g, UInt8 b);
UIntPtr ImgInvertColorEndian(UIntPtr c);
Void ImgClear(PImage img, UIntPtr c);
Void ImgScroll(PImage img, UIntPtr c);
UIntPtr ImgGetPixel(PImage img, UIntPtr x, UIntPtr y);
Void ImgPutPixel(PImage img, UIntPtr x, UIntPtr y, UIntPtr c);
Void ImgDrawLine(PImage img, UIntPtr x0, UIntPtr y0, UIntPtr x1, UIntPtr y1, UIntPtr c);
Void ImgDrawRectangle(PImage img, UIntPtr x, UIntPtr y, UIntPtr w, UIntPtr h, UIntPtr c);
Void ImgFillRectangle(PImage img, UIntPtr x, UIntPtr y, UIntPtr w, UIntPtr h, UIntPtr c);
Void ImgDrawRoundedRectangle(PImage img, UIntPtr x, UIntPtr y, UIntPtr w, UIntPtr h, UIntPtr r, UIntPtr c);
Void ImgFillRoundedRectangle(PImage img, UIntPtr x, UIntPtr y, UIntPtr w, UIntPtr h, UIntPtr r, UIntPtr c);
UIntPtr ImgBlendColors(UIntPtr a, UIntPtr b);
Void ImgBitBlit(PImage img, PImage src, UIntPtr srcx, UIntPtr srcy, UIntPtr x, UIntPtr y, UIntPtr w, UIntPtr h, UInt8 mode);
Void ImgWriteCharacterPixel(PImage img, PUIntPtr x, PUIntPtr y, UIntPtr bg, UIntPtr fg, WChar data);
Void ImgWriteCharacter(PImage img, Boolean cursor, PUIntPtr x, PUIntPtr y, UIntPtr bg, UIntPtr fg, WChar data);
Void ImgWriteStringPixel(PImage img, PUIntPtr x, PUIntPtr y, UIntPtr bg, UIntPtr fg, PWChar data);
Void ImgWriteString(PImage img, Boolean cursor, PUIntPtr x, PUIntPtr y, UIntPtr bg, UIntPtr fg, PWChar data);
Void ImgWriteIntegerPixel(PImage img, PUIntPtr x, PUIntPtr y, UIntPtr bg, UIntPtr fg, UIntPtr data, UInt8 base);
Void ImgWriteInteger(PImage img, Boolean cursor, PUIntPtr x, PUIntPtr y, UIntPtr bg, UIntPtr fg, UIntPtr data, UInt8 base);
Void ImgWriteFormatedPixel(PImage img, PUIntPtr x, PUIntPtr y, UIntPtr bg, UIntPtr fg, PWChar data, ...);
Void ImgWriteFormated(PImage img, Boolean cursor, PUIntPtr x, PUIntPtr y, UIntPtr bg, UIntPtr fg, PWChar data, ...);

#endif		// __CHICAGO_IMG_H__
