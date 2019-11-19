// File author is Ítalo Lima Marconato Matias
//
// Created on April 23 of 2019, at 18:04 BRT
// Last edited on April 26 of 2019, at 16:40 BRT

#ifndef __CHICAGO_CONFIG_H__
#define __CHICAGO_CONFIG_H__

#include <chicago/list.h>

typedef struct {
	PWChar name;
	PWChar value;
} ConfFieldAttribute, *PConfFieldAttribute;

typedef struct {
	PWChar name;
	PWChar value;
	PList attrs;
} ConfField, *PConfField;

PList ConfLoad(PWChar name);
Void ConfFreeField(PConfField field);
Void ConfFree(PList conf);
PConfField ConfGetField(PList conf, PWChar name);
PConfFieldAttribute ConfGetFieldAttribute(PConfField field, PWChar name);

#endif
