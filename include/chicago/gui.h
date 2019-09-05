// File author is Ítalo Lima Marconato Matias
//
// Created on September 04 of 2019, at 18:07 BRT
// Last edited on September 05 of 2019, at 14:21 BRT

#ifndef __CHICAGO_GUI_H__
#define __CHICAGO_GUI_H__

#include <chicago/gui/window.h>
#include <chicago/process.h>

#define GuiDefaultThemeImage (&_binary_theme_bmp_start)

extern UInt8 _binary_theme_bmp_start;

typedef struct {
	PGuiWindow window;
	PProcess proc;
	UIntPtr dir;
} GuiWindowInt, *PGuiWindowInt;

Void GuiAddWindow(PGuiWindow window);
Void GuiRemoveWindow(PGuiWindow window);
Void GuiRemoveProcess(PProcess proc);
Void GuiRefresh(Void);
Void GuiInit(Void);

#endif		// __CHICAGO_GUI_H__
