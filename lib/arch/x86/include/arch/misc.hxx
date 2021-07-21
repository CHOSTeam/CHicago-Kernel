/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 19 of 2021, at 09:53 BRT
 * Last edited on July 21 of 2021 at 15:43 BRT */

#pragma once

#define ARCH_PAUSE() asm volatile("pause" ::: "memory")
#define ARCH_SENSITIVE_END() if (Context & 0x200) asm volatile("sti")

#ifdef __i386__
#define ARCH_SENSITIVE_START() asm volatile("pushfl; pop %0; cli" : "=r"(Context) :: "cc")
#else
#define ARCH_SENSITIVE_START() asm volatile("pushfq; pop %0; cli" : "=r"(Context) :: "cc")
#endif
