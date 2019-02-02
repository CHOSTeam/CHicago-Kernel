// File author is Ítalo Lima Marconato Matias
//
// Created on October 20 of 2018, at 15:20 BRT
// Last edited on February 02 of 2019, at 11:37 BRT

#include <chicago/display.h>
#include <chicago/process.h>
#include <chicago/string.h>

Lock ConLock = { False, Null };
UIntPtr ConCursorX = 0;
UIntPtr ConCursorY = 0;
Boolean ConRefresh = True;
Boolean ConCursorEnabled = True;
UIntPtr ConBackColor = 0xFF000000;
UIntPtr ConForeColor = 0xFFAAAAAA;

Void ConAcquireLock(Void) {
	ConLock.locked = False;																											// Reset the lock
	ConLock.owner = Null;
	ConRefresh = True;																												// And the console attributes
	ConCursorEnabled = True;
	ConBackColor = 0xFF000000;
	ConForeColor = 0xFFAAAAAA;
}

Void ConSetRefresh(Boolean s) {
	PsLock(&ConLock);																												// Lock
	ConRefresh = s;																													// Set the automatic refresh prop
	PsUnlock(&ConLock);																												// Unlock
}

Boolean ConGetRefresh(Void) {
	PsLock(&ConLock);																												// Lock
	Boolean s = ConRefresh;																											// Save the automatic refresh prop
	PsUnlock(&ConLock);																												// Unlock
	return s;																														// Return it
}

Void ConSetCursorEnabled(Boolean e) {
	PsLock(&ConLock);																												// Lock
	
	if (e && !ConCursorEnabled) {																									// Draw the new cursor?
		DispFillRectangle(ConCursorX * 8, ConCursorY * 16, 8, 16, ConForeColor);													// Yes
	} else if (!e && ConCursorEnabled) {																							// Erase the old cursor?
		DispFillRectangle(ConCursorX * 8, ConCursorY * 16, 8, 16, ConBackColor);													// Yes
	}
	
	ConCursorEnabled = e;																											// Set the cursor enabled prop
	
	PsUnlock(&ConLock);																												// Unlock
}

Boolean ConGetCursorEnabled(Void) {
	PsLock(&ConLock);																												// Lock
	Boolean s = ConCursorEnabled;																									// Save the cursor enabled prop
	PsUnlock(&ConLock);																												// Unlock
	return s;																														// Return it
}

Void ConSetColor(UIntPtr bg, UIntPtr fg) {
	PsLock(&ConLock);																												// Lock
	ConBackColor = bg;																												// Set the background of the console
	ConForeColor = fg;																												// Set the foreground of the console
	PsUnlock(&ConLock);																												// Unlock
}

Void ConSetBackground(UIntPtr c) {
	PsLock(&ConLock);																												// Lock
	ConBackColor = c;																												// Set the background of the console
	PsUnlock(&ConLock);																												// Unlock
}

Void ConSetForeground(UIntPtr c) {
	PsLock(&ConLock);																												// Lock
	ConForeColor = c;																												// Set the foreground of the console
	PsUnlock(&ConLock);																												// Unlock
}

Void ConGetColor(PUIntPtr bg, PUIntPtr fg) {
	PsLock(&ConLock);																												// Lock
	
	if (bg != Null) {																												// We should save the bg?
		*bg = ConBackColor;																											// Yes
	}
	
	if (fg != Null) {																												// And the fg?
		*fg = ConForeColor;																											// Yes
	}
	
	PsUnlock(&ConLock);																												// Unlock
}

UIntPtr ConGetBackground(Void) {
	PsLock(&ConLock);																												// Lock
	IntPtr bg = ConBackColor;																										// Save the bg
	PsUnlock(&ConLock);																												// Unlock
	return bg;																														// Return it
}

UIntPtr ConGetForeground(Void) {
	PsLock(&ConLock);																												// Lock
	IntPtr fg = ConForeColor;																										// Save the fg
	PsUnlock(&ConLock);																												// Unlock
	return fg;																														// Return it
}

Void ConSetCursor(UIntPtr x, UIntPtr y) {
	PsLock(&ConLock);																												// Lock
	ConCursorX = x;																													// Set the x position of the cursor
	ConCursorY = y;																													// Set the y position of the cursor
	PsUnlock(&ConLock);																												// Unlock
}

Void ConSetCursorX(UIntPtr pos) {
	PsLock(&ConLock);																												// Lock
	ConCursorX = pos;																												// Set the x position of the cursor
	PsUnlock(&ConLock);																												// Unlock
}

Void ConSetCursorY(UIntPtr pos) {
	PsLock(&ConLock);																												// Lock
	ConCursorY = pos;																												// Set the y position of the cursor
	PsUnlock(&ConLock);																												// Unlock
}

Void ConGetCursor(PUIntPtr x, PUIntPtr y) {
	PsLock(&ConLock);																												// Lock
	
	if (x != Null) {																												// We should save the x?
		*x = ConCursorX;																											// Yes
	}
	
	if (y != Null) {																												// And the y?
		*y = ConCursorY;																											// Yes
	}
	
	PsUnlock(&ConLock);																												// Unlock
}

UIntPtr ConGetCursorX(Void) {
	PsLock(&ConLock);																												// Lock
	IntPtr x = ConCursorX;																											// Save the x
	PsUnlock(&ConLock);																												// Unlock
	return x;																														// Return it
}

UIntPtr ConGetCursorY(Void) {
	PsLock(&ConLock);																												// Lock
	IntPtr y = ConCursorY;																											// Save the y
	PsUnlock(&ConLock);																												// Unlock
	return y;																														// Return it
}

Void ConClearScreen(Void) {
	PsLock(&ConLock);																												// Lock
	DispClearScreen(ConBackColor);																									// Clear the screen
	ConCursorX = ConCursorY = 0;																									// Move the cursor to 0, 0
	
	if (ConCursorEnabled) {																											// Draw the cursor?
		DispFillRectangle(0, 0, 8, 16, ConForeColor);																				// Yes
	}
	
	if (ConRefresh) {
		DispRefresh();																												// Refresh the screen
	}
	
	PsUnlock(&ConLock);																												// Unlock
}

static Void ConWriteCharacterInt(WChar data) {
	switch (data) {
	case '\b': {																													// Backspace
		if (ConCursorX > 0) {																										// We have any more characters to delete in this line?
			ConCursorX--;																											// Yes
		} else if (ConCursorY > 0) {																								// We have any more lines?																										
			ConCursorY--;																											// Yes
			ConCursorX = DispGetWidth() / 8 - 1;
		}
		
		DispFillRectangle(ConCursorX * 8, ConCursorY * 16, 8, 16, ConBackColor);
		
		break;
	}
	case '\n': {																													// Line feed
		ConCursorY++;
		break;
	}
	case '\r': {																													// Carriage return
		ConCursorX = 0;
		break;
	}
	case '\t': {																													// Tab
		ConCursorX = (ConCursorX + 4) & ~3;
		break;
	}
	default: {																														// Character
		PPCScreenFont font = (PPCScreenFont)DispFontStart;
		PUInt8 glyph = (PUInt8)(DispFontStart + font->hdr_size + (data * font->bytes_per_glyph));
		
		for (UIntPtr i = 0; i < 16; i++) {
			UIntPtr mask = 1 << 7;
			
			for (UIntPtr j = 0; j < 8; j++) {
				DispPutPixel((ConCursorX * 8) + j, (ConCursorY * 16) + i, (*glyph & mask) ? ConForeColor : ConBackColor);
				mask >>= 1;
			}
			
			glyph++;
		}
		
		ConCursorX++;
		
		break;
	}
	}
	
	if (ConCursorX >= DispGetWidth() / 8) {																							// Go to the next line?
		ConCursorX = 0;																												// Yes
		ConCursorY++;
	}
	
	if (ConCursorY >= DispGetHeight() / 16) {																						// Scroll up?
		DispScrollScreen(ConBackColor);																								// Yes
		ConCursorY = (DispGetHeight() / 16) - 1;
	}
}

Void ConWriteCharacter(WChar data) {
	PsLock(&ConLock);																												// Lock
	
	if (ConCursorEnabled) {																											// Erase the old cursor?
		DispFillRectangle(ConCursorX * 8, ConCursorY * 16, 8, 16, ConBackColor);													// Yes
	}
	
	ConWriteCharacterInt(data);																										// Write the character
	
	if (ConCursorEnabled) {																											// Draw the new cursor?
		DispFillRectangle(ConCursorX * 8, ConCursorY * 16, 8, 16, ConForeColor);													// Yes
	}
	
	if (ConRefresh) {
		DispRefresh();																												// Refresh the screen
	}
	
	PsUnlock(&ConLock);																												// Unlock
}

static Void ConWriteStringInt(PWChar data) {
	if (data == Null) {
		return;
	}
	
	for (UIntPtr i = 0; i < StrGetLength(data); i++) {																				// Write all the characters from the string
		ConWriteCharacterInt(data[i]);
	}
}

Void ConWriteString(PWChar data) {
	PsLock(&ConLock);																												// Lock
	
	if (ConCursorEnabled) {																											// Erase the old cursor?
		DispFillRectangle(ConCursorX * 8, ConCursorY * 16, 8, 16, ConBackColor);													// Yes
	}
	
	ConWriteStringInt(data);																										// Write the string
	
	if (ConCursorEnabled) {																											// Draw the new cursor?
		DispFillRectangle(ConCursorX * 8, ConCursorY * 16, 8, 16, ConForeColor);													// Yes
	}
	
	if (ConRefresh) {
		DispRefresh();																												// Refresh the screen
	}
	
	PsUnlock(&ConLock);																												// Unlock
}

static Void ConWriteIntegerInt(UIntPtr data, UInt8 base) {
	if (data == 0) {
		ConWriteCharacterInt('0');
		return;
	}
	
	static WChar buf[32] = { 0 };
	Int i = 30;
	
	for (; data && i; i--, data /= base) {
		buf[i] = L"0123456789ABCDEF"[data % base];
	}
	
	ConWriteStringInt(&buf[i + 1]);
}

Void ConWriteInteger(UIntPtr data, UInt8 base) {
	PsLock(&ConLock);																												// Lock
	
	if (ConCursorEnabled) {																											// Erase the old cursor?
		DispFillRectangle(ConCursorX * 8, ConCursorY * 16, 8, 16, ConBackColor);													// Yes
	}
	
	ConWriteIntegerInt(data, base);																									// Write the integer
	
	if (ConCursorEnabled) {																											// Draw the new cursor?
		DispFillRectangle(ConCursorX * 8, ConCursorY * 16, 8, 16, ConForeColor);													// Yes
	}
	
	if (ConRefresh) {
		DispRefresh();																												// Refresh the screen
	}
	
	PsUnlock(&ConLock);																												// Unlock
}

Void ConWriteFormated(PWChar data, ...) {
	if (data == Null) {
		return;
	}
	
	PsLock(&ConLock);																												// Lock
	
	if (ConCursorEnabled) {																											// Erase the old cursor?
		DispFillRectangle(ConCursorX * 8, ConCursorY * 16, 8, 16, ConBackColor);													// Yes
	}
	
	VariadicList va;
	VariadicStart(va, data);																										// Let's start our va list with the arguments provided by the user (if any)
	
	UIntPtr oldbg = ConBackColor;																									// Save the bg and the fg
	UIntPtr oldfg = ConForeColor;
	
	for (UIntPtr i = 0; i < StrGetLength(data); i++) {
		if (data[i] != '%') {																										// It's an % (integer, string, character or other)?
			ConWriteCharacterInt(data[i]);																							// No
		} else {
			switch (data[++i]) {																									// Yes, let's parse it!
			case 's': {																												// String
				ConWriteStringInt((PWChar)VariadicArg(va, PWChar));
				break;
			}
			case 'c': {																												// Character
				ConWriteCharacterInt((Char)VariadicArg(va, Int));
				break;
			}
			case 'd': {																												// Decimal Number
				ConWriteIntegerInt((UIntPtr)VariadicArg(va, UIntPtr), 10);
				break;
			}
			case 'x': {																												// Hexadecimal Number
				ConWriteIntegerInt((UIntPtr)VariadicArg(va, UIntPtr), 16);
				break;
			}
			case 'b': {																												// Change the background color
				ConBackColor = (UIntPtr)VariadicArg(va, UIntPtr);
				break;
			}
			case 'f': {																												// Change the foreground color
				ConForeColor = (UIntPtr)VariadicArg(va, UIntPtr);
				break;
			}
			case 'r': {																												// Reset the bg and the fg
				ConBackColor = oldbg;
				ConForeColor = oldfg;
				break;
			}
			default: {																												// Probably it's another % (probably)
				ConWriteCharacterInt(data[i]);
				break;
			}
			}
		}
	}
	
	VariadicEnd(va);
	
	if (ConCursorEnabled) {																											// Draw the new cursor?
		DispFillRectangle(ConCursorX * 8, ConCursorY * 16, 8, 16, ConForeColor);													// Yes
	}
	
	if (ConRefresh) {
		DispRefresh();																												// Refresh the screen
	}
	
	PsUnlock(&ConLock);																												// Unlock
}
