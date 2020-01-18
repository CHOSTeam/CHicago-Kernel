// File author is Ítalo Lima Marconato Matias
//
// Created on April 18 of 2019, at 19:03 BRT
// Last edited on January 07 of 2020, at 10:41 BRT

#define __CHICAGO_DISPLAY__

#include <chicago/alloc.h>
#include <chicago/arch.h>
#include <chicago/debug.h>
#include <chicago/display.h>
#include <chicago/mm.h>
#include <chicago/string.h>

PImage DispBackBuffer = Null;
UIntPtr DispFrameBuffer = 0;
UInt32 DispProgressBar = 0;

UIntPtr DispGetFrameBuffer(Void) {
	return DispFrameBuffer;
}

UInt8 DispGetBytesPerPixel(Void) {
	return DispBackBuffer->bpp;
}

UIntPtr DispGetWidth(Void) {
	return DispBackBuffer->width;
}

UIntPtr DispGetHeight(Void) {
	return DispBackBuffer->height;
}

Void DispRefresh(Void) {
	StrCopyMemory((PUInt8)DispFrameBuffer, (PUInt8)DispBackBuffer->buf, DispBackBuffer->width * DispBackBuffer->height * DispBackBuffer->bpp);			// Copy the backbuffer into the framebuffer
}

Void DispClearScreen(UIntPtr c) {
	ImgClear(DispBackBuffer, c);																														// Redirect to ImgClear
}

Void DispScrollScreen(UIntPtr c) {
	ImgScroll(DispBackBuffer, c);																														// Redirect to ImgScroll
}

UIntPtr DispGetPixel(UIntPtr x, UIntPtr y) {
	return ImgGetPixel(DispBackBuffer, x, y);																											// Redirect to ImgGetPixel
}

Void DispPutPixel(UIntPtr x, UIntPtr y, UIntPtr c) {
	ImgPutPixel(DispBackBuffer, x, y, c);																												// Redirect to ImgPutPixel
}

Void DispDrawLine(UIntPtr x0, UIntPtr y0, UIntPtr x1, UIntPtr y1, UIntPtr c) {
	ImgDrawLine(DispBackBuffer, x0, y0, x1, y1, c);																										// Redirect to ImgDrawLine
}

Void DispDrawRectangle(UIntPtr x, UIntPtr y, UIntPtr w, UIntPtr h, UIntPtr c) {
	ImgDrawRectangle(DispBackBuffer, x, y, w, h, c);																									// Redirect to ImgDrawRectangle
}

Void DispFillRectangle(UIntPtr x, UIntPtr y, UIntPtr w, UIntPtr h, UIntPtr c) {
	ImgFillRectangle(DispBackBuffer, x, y, w, h, c);																									// Redirect to ImgFillRectangle
}

Void DispDrawRoundedRectangle(UIntPtr x, UIntPtr y, UIntPtr w, UIntPtr h, UIntPtr r, UIntPtr c) {
	ImgDrawRoundedRectangle(DispBackBuffer, x, y, w, h, r, c);																							// Redirect to ImgDrawRoundedRectangle
}

Void DispFillRoundedRectangle(UIntPtr x, UIntPtr y, UIntPtr w, UIntPtr h, UIntPtr r, UIntPtr c) {
	ImgFillRoundedRectangle(DispBackBuffer, x, y, w, h, r, c);																							// Redirect to ImgFillRoundedRectangle
}

Void DispBitBlit(PImage src, UIntPtr srcx, UIntPtr srcy, UIntPtr x, UIntPtr y, UIntPtr w, UIntPtr h, UInt8 mode) {
	ImgBitBlit(DispBackBuffer, src, srcx, srcy, x, y, w, h, mode);																						// Redirect to ImgBitBlit
}

Void DispIncrementProgessBar(Void) {
	if (DispProgressBar == 0) {																															// This first one is different
		DispFillRectangle(DispBackBuffer->width / 2 - 100 + (DispProgressBar * 2) + 3, DispBackBuffer->height - 24, 1, 9, 0xFF0D3D56);
		DispFillRectangle(DispBackBuffer->width / 2 - 100 + (DispProgressBar * 2) + 4, DispBackBuffer->height - 25, 1, 11, 0xFF0D3D56);
		DispFillRectangle(DispBackBuffer->width / 2 - 100 + (DispProgressBar * 2) + 5, DispBackBuffer->height - 26, 1, 13, 0xFF0D3D56);
		DispFillRectangle(DispBackBuffer->width / 2 - 100 + (DispProgressBar * 2) + 6, DispBackBuffer->height - 27, 11, 15, 0xFF0D3D56);
		DispProgressBar += 10;
	} else if (DispProgressBar < 90) {
		DispFillRectangle(DispBackBuffer->width / 2 - 100 + (DispProgressBar * 2), DispBackBuffer->height - 27, 17, 15, 0xFF0D3D56);					// The others are... normal
		DispProgressBar += 10;
	} else if (DispProgressBar < 100) {
		DispFillRectangle(DispBackBuffer->width / 2 - 100 + (DispProgressBar * 2), DispBackBuffer->height - 27, 15, 15, 0xFF0D3D56);					// The last one is also different
		DispFillRectangle(DispBackBuffer->width / 2 - 100 + (DispProgressBar * 2) + 15, DispBackBuffer->height - 26, 1, 13, 0xFF0D3D56);
		DispFillRectangle(DispBackBuffer->width / 2 - 100 + (DispProgressBar * 2) + 16, DispBackBuffer->height - 25, 1, 11, 0xFF0D3D56);
		DispFillRectangle(DispBackBuffer->width / 2 - 100 + (DispProgressBar * 2) + 17, DispBackBuffer->height - 24, 1, 9, 0xFF0D3D56);
		DispProgressBar += 10;
	}
	
	DispRefresh();
}

Void DispFillProgressBar(Void) {
	while (DispProgressBar < 100) {																														// Just fill the progress bar until it's 100%
		DispIncrementProgessBar();
	}
}

#ifndef VERBOSE_BOOT
static Void DispDrawBitmap(PUInt8 bmp, UIntPtr x, UIntPtr y) {
	PImage img = ImgLoadBMPBuf(bmp, False);																												// Load the bmp into a Image struct
	
	if (img != Null) {																																	// Failed?
		DispBitBlit(img, 0, 0, x, y, img->width, img->height, BITBLIT_MODE_COPY);																		// Nope, so copy it into the screen
		MemFree((UIntPtr)img);																															// And free the Image struct
	}
}

static Void DispWriteString(UIntPtr x, UIntPtr y, UIntPtr bg, UIntPtr fg, PWChar data) {
	ImgWriteString(DispBackBuffer, False, &x, &y, bg, fg, data);																						// Redirect to ImgWriteString
}
#endif

Void DispDrawProgessBar(Void) {
	if ((ArchBootOptions & BOOT_OPTIONS_VERBOSE) != BOOT_OPTIONS_VERBOSE) {																				// Verbose boot?
		DispWriteString((DispBackBuffer->width / 2 - 56) / 8, 1, 0x000000, 0xFFFFFF, L"Starting up...");												// No, draw the "Starting..." text
		DispDrawBitmap(DispBootSplashImage, DispBackBuffer->width / 2 - 150, DispBackBuffer->height / 2 - 50);											// Draw the logo
	}
	
	DispDrawRoundedRectangle(DispBackBuffer->width / 2 - 100, DispBackBuffer->height - 30, 201, 21, 7, 0xFFFFFFFF);										// Draw the progress bar border
	DispRefresh();
}

Void DispInit(UIntPtr w, UIntPtr h, UIntPtr bpp, UIntPtr fb) {
	if ((bpp != 3) && (bpp != 4)) {																														// We only support 24 and 32 bpp
		DbgWriteFormated("PANIC! Couldn't init the display\r\n");
		ArchHalt();																																		// Halt
	}
	
	DispFrameBuffer = MemAAllocate(w * h * bpp, MM_PAGE_SIZE);																							// Alloc some virt space for the frame buffer
	DispBackBuffer = ImgCreate(w, h, bpp, False);
	
	if (DispFrameBuffer == 0 || DispBackBuffer == Null) {
		DbgWriteFormated("PANIC! Couldn't init the display\r\n");																						// Failed...
		ArchHalt();																																		// Halt
	}
	
	for (UIntPtr i = 0; i < w * h * bpp; i += MM_PAGE_SIZE) {																							// Let's map the frame buffer to the virtual memory!
		MmDereferencePage(MmGetPhys(DispFrameBuffer + i));																								// MemAAllocate allocated some phys addr as well, free it
		
		if (!MmMap(DispFrameBuffer + i, fb + i, MM_MAP_KDEF)) {
			DbgWriteFormated("PANIC! Couldn't init the display\r\n");
			ArchHalt();																																	// Halt
		}
	}
	
	DispClearScreen(0xFF000000);																														// Clear the screen
	DispRefresh();
}
