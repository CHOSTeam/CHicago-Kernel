// File author is Ítalo Lima Marconato Matias
//
// Created on September 04 of 2019, at 18:11 BRT
// Last edited on September 04 of 2019, at 19:24 BRT

#ifndef __CHICAGO_GUI_WINDOW_H__
#define __CHICAGO_GUI_WINDOW_H__

#include <chicago/img.h>

typedef struct {
	PWChar title;
	UIntPtr x;
	UIntPtr y;
	UIntPtr w;
	UIntPtr h;
} GuiWindow, *PGuiWindow;

PGuiWindow GuiCreateWindow(PWChar title, UIntPtr x, UIntPtr y, UIntPtr w, UIntPtr h);
Void GuiFreeWindow(PGuiWindow window);

#endif		// __CHICAGO_GUI_WINDOW_H__
