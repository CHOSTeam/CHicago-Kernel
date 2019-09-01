// File author is Ítalo Lima Marconato Matias
//
// Created on October 12 of 2018, at 23:10 BRT
// Last edited on September 01 of 2019, at 15:03 BRT

#include <chicago/arch/idt.h>
#include <chicago/arch/port.h>

#include <chicago/console.h>
#include <chicago/debug.h>
#include <chicago/device.h>
#include <chicago/panic.h>
#include <chicago/process.h>

static Boolean KbdCtrl = False;
static Boolean KbdShft = False;
static Boolean KbdCaps = False;
static Boolean KbdAlt = False;

static Char KbdKeymapNormal[128] = {
	0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
	'\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
	0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
	0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
	'*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0,
	0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static Char KbdKeymapShift[128] = {
	0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
	'\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
	0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
	0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
	'*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0,
	0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static Void KeyboardHandler(PRegisters regs) {
	(Void)regs;
	RawKeyboardDeviceWrite(PortInByte(0x60));														// Read the key and write it to the raw keyboard device
}

static Void KeyboardThread(Void) {
	UInt8 sc;
	
	ConsoleDeviceClearKeyboard();																	// Clear the buffers
	RawKeyboardDeviceClear();
	
	while (True) {
		RawKeyboardDeviceRead(1, &sc);																// Read the key
		
		if (sc & 0x80) {
			if ((sc == 0xAA) || (sc == 0xB6)) {														// Yes! Shift?
				KbdShft = !KbdShft;
			} else if (sc == 0xB8) {																// Alt?
				KbdAlt = !KbdAlt;
			} else if (sc == 0x9D) {																// Control?
				KbdCtrl = !KbdCtrl;
			}
		} else if ((sc == 0x2A) || (sc == 0x36)) {													// Shift (key press)?
			KbdShft = !KbdShft;
		} else if (sc == 0x3A) {																	// Caps Lock?
			KbdCaps = !KbdCaps;
		} else if (sc == 0x38) {																	// Alt (key press)?
			KbdAlt  = !KbdAlt;
		} else if (sc == 0x1D) {																	// Control (key press)?
			KbdCtrl = !KbdCtrl;
		} else {
			Boolean shft = KbdShft ^ KbdCaps;
			Char ch = shft ? KbdKeymapShift[sc] : KbdKeymapNormal[sc];								// Ok, it's a normal key... get the character from the keymap

			if (ch == '\b') {																		// Backspace?
				if (ConsoleDeviceBackKeyboard()) {													// Yes, do it!
					ConWriteCharacter(ch);
				}
			} else if (ch == '\n') {																// New line?
				ConsoleDeviceWriteKeyboard(ch);														// Yes, convert the \n to \r\n
				ConWriteFormated(L"\r\n");
			} else if ((ch != '\t') && (KbdCtrl && shft)) {											// Control+Key (with shift)?
				ConsoleDeviceWriteKeyboard(ch - 'A');												// Yes
				ConWriteFormated(L"^%c", ch);
			} else if (ch != '\t' && KbdCtrl) {														// Control+Key (without shift)?
				ConsoleDeviceWriteKeyboard(ch - 'a');												// Yes
				ConWriteFormated(L"^%c", ch - 0x20);
			} else {
				ConsoleDeviceWriteKeyboard(ch);														// Normal character!
				ConWriteFormated(L"%c", ch);
			}
		}
	}
}

Void ArchInitKeyboard(Void) {
	IDTRegisterIRQHandler(1, KeyboardHandler);														// Register the keyboard IRQ handler
}

Void ArchFinishKeyboard(Void) {
	PThread th = PsCreateThread((UIntPtr)KeyboardThread, 0, False);									// Try to create the keyboard thread
	
	if (th == Null) {
		DbgWriteFormated("PANIC! Couldn't create the keyboard thread\r\n");							// Failed :(
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	PsAddThread(th);																				// Add it
}
