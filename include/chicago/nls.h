// File author is Ítalo Lima Marconato Matias
//
// Created on December 09 of 2018, at 19:26 BRT
// Last edited on August 27 of 2019, at 17:38 BRT

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
#define NLS_SHELL_USAGE1 0x06
#define NLS_SHELL_CAT_ERR1 0x07
#define NLS_SHELL_CAT_ERR2 0x08
#define NLS_SHELL_CD_ERR1 0x09
#define NLS_SHELL_CD_ERR2 0x0A
#define NLS_SHELL_HELP 0x0B
#define NLS_SHELL_LANG_LIST 0x0C
#define NLS_SHELL_LANG_INVALID 0x0D
#define NLS_SHELL_LS_ERR 0x0E
#define NLS_SHELL_PING_REPLY1 0x0F
#define NLS_SHELL_PING_REPLY2 0x10
#define NLS_SHELL_PS_UNAMED 0x11
#define NLS_SHELL_PS 0x12
#define NLS_SHELL_SETIP_USAGE 0x13
#define NLS_SHELL_SETIP_ERR1 0x14
#define NLS_SHELL_SETIP_ERR2 0x15
#define NLS_SHELL_SETIP_ERR3 0x16
#define NLS_SHELL_SETNET_USAGE 0x17
#define NLS_SHELL_DNDNOTSET 0x18
#define NLS_SHELL_LSNET_NOTCUR 0x19
#define NLS_SHELL_LSNET_CUR 0x1A
#define NLS_SHELL_TEST_SUMMARY 0x1B
#define NLS_SHELL_INVALID 0x1C

PWChar NlsGetMessage(UIntPtr msg);
PWChar NlsGetLanguages(Void);
Void NlsSetLanguage(UIntPtr lang);
UIntPtr NlsGetLanguage(PWChar lang);

#endif		// __CHICAGO_NLS_H__
