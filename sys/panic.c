// File author is Ítalo Lima Marconato Matias
//
// Created on October 27 of 2018, at 21:37 BRT
// Last edited on November 10 of 2019, at 11:10 BRT

#include <chicago/console.h>
#include <chicago/nls.h>
#include <chicago/panic.h>

PWChar PanicStrings[PANIC_COUNT] = {
	L"MM_READWRITE_TO_NONPRESENT_AREA",
	L"MM_WRITE_TO_READONLY_AREA",
	L"BOOTDEV_MOUNT_FAILED",
	L"KERNEL_PROCESS_CLOSED",
	L"KERNEL_MAIN_THREAD_CLOSED",
	L"OSMNGR_START_FAILED",
	L"OSMNGR_PROCESS_CLOSED",
	L"KERNEL_UNEXPECTED_ERROR",
	L"KERNEL_INIT_FAILED"
};

Void PanicInt(UInt32 err, Boolean perr) {
	if (!perr) {																														// Print the "Sorry" message?
		ConSetCursorEnabled(False);																										// Yes, disable the cursor
		ConSetColor(0xFF8B0000, 0xFFFFFFFF);																							// Red background, white foreground
		ConClearScreen();																												// Clear the screen
		ConWriteFormated(NlsGetMessage(NLS_PANIC_SORRY));																				// And print the "Sorry" message
	} else {																															// Print the error code?
		ConWriteFormated(NlsGetMessage(NLS_PANIC_ERRCODE), err < PANIC_COUNT ? PanicStrings[err] : L"UNKNOWN_ERROR");					// Yes
	}
}
