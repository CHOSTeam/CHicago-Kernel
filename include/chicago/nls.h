// File author is Ítalo Lima Marconato Matias
//
// Created on December 09 of 2018, at 19:26 BRT
// Last edited on October 27 of 2019, at 16:14 BRT

#ifndef __CHICAGO_NLS_H__
#define __CHICAGO_NLS_H__

#include <chicago/types.h>

#define NLS_LANG_EN 0x00
#define NLS_LANG_BR 0x01

#define NLS_PANIC_SORRY 0x00
#define NLS_PANIC_ERRCODE 0x01
#define NLS_OS_NAME 0x02
#define NLS_OS_CODENAME 0x03
#define NLS_OS_VSTR 0x04
#define NLS_SEGFAULT 0x05

PWChar NlsGetMessage(UIntPtr msg);
PWChar NlsGetLanguages(Void);
Void NlsSetLanguage(UIntPtr lang);
UIntPtr NlsGetLanguage(PWChar lang);

#endif		// __CHICAGO_NLS_H__
