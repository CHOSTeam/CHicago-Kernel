// File author is Ítalo Lima Marconato Matias
//
// Created on October 20 of 2018, at 15:20 BRT
// Last edited on January 01 of 2020, at 18:59 BRT

#include <chicago/alloc.h>
#include <chicago/display.h>
#include <chicago/process.h>
#include <chicago/string.h>

Lock ConLock = { 0, False, Null };
UIntPtr ConCursorX = 0;
UIntPtr ConCursorY = 0;
UIntPtr ConSurfaceX = 0;
UIntPtr ConSurfaceY = 0;
PImage ConSurface = Null;
Boolean ConRefresh = True;
Boolean ConSurfaceDisp = False;
Boolean ConSurfaceFree = False;
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
	
	if (ConSurfaceFree) {																											// Free the old surface?
		MemFree((UIntPtr)ConSurface);																								// Yes
	}
	
	ConSurface = DispBackBuffer;																									// Set the default surface
}

Void ConSetSurface(PImage img, Boolean disp, Boolean free, UIntPtr x, UIntPtr y) {
	if (img == Null) {																												// Sanity check
		return;
	}
	
	PsLock(&ConLock);																												// Lock
	
	if (ConSurfaceFree) {																											// Free the old surface?
		MemFree((UIntPtr)ConSurface);																								// Yes
	}
	
	ConSurface = img;																												// Set the surface
	ConSurfaceX = x;
	ConSurfaceY = y;
	ConRefresh = True;																												// And reset the console attributes
	ConSurfaceDisp = disp;
	ConSurfaceFree = free;
	ConCursorEnabled = True;
	ConBackColor = 0xFF000000;
	ConForeColor = 0xFFAAAAAA;
	
	PsUnlock(&ConLock);																												// Unlock
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
		ImgFillRectangle(ConSurface, ConCursorX * 8, ConCursorY * 16, 8, 16, ConForeColor);											// Yes
	} else if (!e && ConCursorEnabled) {																							// Erase the old cursor?
		ImgFillRectangle(ConSurface, ConCursorX * 8, ConCursorY * 16, 8, 16, ConBackColor);											// Yes
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

Void ConRefreshScreen(Void) {
	if ((ConSurface == Null) || !ConRefresh) {																						// Check if we should refresh
		return;
	} else if (ConSurface != DispBackBuffer && !ConSurfaceDisp) {																	// Our surface is the backbuffer?
		DispBitBlit(ConSurface, 0, 0, ConSurfaceX, ConSurfaceY, ConSurface->width, ConSurface->height, BITBLIT_MODE_COPY);			// Nope, copy it to the backbuffer
	}
	
	DispRefresh();																													// Refresh the screen (copy the backbuffer to the front buffer)
}

Void ConClearScreen(Void) {
	PsLock(&ConLock);																												// Lock
	ImgClear(ConSurface, ConBackColor);																								// Clear the screen
	ConCursorX = ConCursorY = 0;																									// Move the cursor to 0, 0
	
	if (ConCursorEnabled) {																											// Draw the cursor?
		ImgFillRectangle(ConSurface, 0, 0, 8, 16, ConForeColor);																	// Yes
	}
	
	ConRefreshScreen();																												// Refresh the screen
	PsUnlock(&ConLock);																												// Unlock
}

static Void ConWriteCharacterInt(WChar data, Boolean cursor) {
	ImgWriteCharacter(ConSurface, cursor, &ConCursorX, &ConCursorY, ConBackColor, ConForeColor, data);								// Redirect to ImgWriteCharacter
}

Void ConWriteCharacter(WChar data) {
	PsLock(&ConLock);																												// Lock
	
	if (data != '\n' && ConCursorEnabled) {																							// Erase the old cursor?
		ImgFillRectangle(ConSurface, ConCursorX * 8, ConCursorY * 16, 8, 16, ConBackColor);											// Yes
	}
	
	ConWriteCharacterInt(data, ((data != '\n') && (data != '\r')) ? ConCursorEnabled : False);										// Write the character
	
	if (data == '\n' && ConCursorEnabled) {																							// Draw the new cursor?
		ImgFillRectangle(ConSurface, ConCursorX * 8, ConCursorY * 16, 8, 16, ConForeColor);											// Yes
	}
	
	ConRefreshScreen();																												// Refresh the screen
	PsUnlock(&ConLock);																												// Unlock
}

static Void ConWriteStringInt(PWChar data, Boolean cursor) {
	if (data == Null) {																												// Sanity check
		return;
	}
	
	ImgWriteString(ConSurface, cursor, &ConCursorX, &ConCursorY, ConBackColor, ConForeColor, data);									// Redirect to ImgWriteString
}

Void ConWriteString(PWChar data) {
	PsLock(&ConLock);																												// Lock
	ConWriteStringInt(data, ConCursorEnabled);																						// Write the string
	ConRefreshScreen();																												// Refresh the screen
	PsUnlock(&ConLock);																												// Unlock
}

static Void ConWriteIntegerInt(UIntPtr data, UInt8 base, Boolean cursor) {
	ImgWriteInteger(ConSurface, cursor, &ConCursorX, &ConCursorY, ConBackColor, ConForeColor, data, base);							// Redirect to ImgWriteIntegerr
}

Void ConWriteInteger(UIntPtr data, UInt8 base) {
	PsLock(&ConLock);																												// Lock
	ConWriteIntegerInt(data, base, ConCursorEnabled);																				// Write the integer
	ConRefreshScreen();																												// Refresh the screen
	PsUnlock(&ConLock);																												// Unlock
}

Void ConWriteFormated(PWChar data, ...) {
	if (data == Null) {
		return;
	}
	
	PsLock(&ConLock);																												// Lock
	
	if (ConCursorEnabled) {																											// Erase the old cursor?
		ImgFillRectangle(ConSurface, ConCursorX * 8, ConCursorY * 16, 8, 16, ConBackColor);											// Yes
	}
	
	VariadicList va;
	VariadicStart(va, data);																										// Let's start our va list with the arguments provided by the user (if any)
	
	UIntPtr oldbg = ConBackColor;																									// Save the bg and the fg
	UIntPtr oldfg = ConForeColor;
	
	for (UIntPtr i = 0; i < StrGetLength(data); i++) {
		if (data[i] != '%') {																										// It's an % (integer, string, character or other)?
			ConWriteCharacterInt(data[i], False);																					// No
		} else {
			switch (data[++i]) {																									// Yes, let's parse it!
			case 's': {																												// String
				ConWriteStringInt((PWChar)VariadicArg(va, PWChar), False);
				break;
			}
			case 'c': {																												// Character
				ConWriteCharacterInt((Char)VariadicArg(va, Int), False);
				break;
			}
			case 'd': {																												// Decimal Number
				ConWriteIntegerInt((UIntPtr)VariadicArg(va, UIntPtr), 10, False);
				break;
			}
			case 'x': {																												// Hexadecimal Number
				ConWriteIntegerInt((UIntPtr)VariadicArg(va, UIntPtr), 16, False);
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
				ConWriteCharacterInt(data[i], False);
				break;
			}
			}
		}
	}
	
	VariadicEnd(va);
	
	if (ConCursorEnabled) {																											// Draw the new cursor?
		ImgFillRectangle(ConSurface, ConCursorX * 8, ConCursorY * 16, 8, 16, ConForeColor);											// Yes
	}
	
	ConRefreshScreen();																												// Refresh the screen
	PsUnlock(&ConLock);																												// Unlock
}
