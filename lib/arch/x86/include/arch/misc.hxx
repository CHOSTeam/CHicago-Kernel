/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 19 of 2021, at 09:53 BRT
 * Last edited on July 19 of 2021 at 09:53 BRT */

#pragma once
#define ARCH_PAUSE() asm volatile("pause" ::: "memory")
