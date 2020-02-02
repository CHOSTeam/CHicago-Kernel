// File author is Ítalo Lima Marconato Matias
//
// Created on October 20 of 2018, at 15:20 BRT
// Last edited on January 02 of 2020, at 13:35 BRT

#include <chicago/alloc.h>
#include <chicago/display.h>
#include <chicago/process.h>
#include <chicago/string.h>

PThread ConLockOwner = Null;
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

static Void ConLock(Void) {
	if (PsCurrentThread == Null) {																									// If threading isn't initialized, we don't need to do anything
		return;
	}
	
	PsWaitForAddress((UIntPtr)&ConLockOwner, 0, PsCurrentThread, 0);																// Wait until the ConLockOwner is Null
}

static Void ConUnlock(Void) {
	if (PsCurrentThread != Null && ConLockOwner == PsCurrentThread) {																// Is threading initialized? If yes, are we the owners?
		ConLockOwner = Null;																										// Yeah, set it to Null, and wake any thread that is waiting for us
		PsWakeAddress((UIntPtr)&ConLockOwner);
	}
}

Void ConAcquireLock(Void) {
	ConLockOwner = Null;																											// Reset the lock
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
	
	ConLock();																														// Lock
	
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
	
	ConUnlock();																													// Unlock
}

Void ConSetRefresh(Boolean s) {
	ConLock();																														// Lock
	ConRefresh = s;																													// Set the automatic refresh prop
	ConUnlock();																													// Unlock
}

Boolean ConGetRefresh(Void) {
	ConLock();																														// Lock
	Boolean s = ConRefresh;																											// Save the automatic refresh prop
	ConUnlock();																													// Unlock
	return s;																														// Return it
}

Void ConSetCursorEnabled(Boolean e) {
	ConLock();																														// Lock
	
	if (e && !ConCursorEnabled) {																									// Draw the new cursor?
		ImgFillRectangle(ConSurface, ConCursorX * 8, ConCursorY * 16, 8, 16, ConForeColor);											// Yes
	} else if (!e && ConCursorEnabled) {																							// Erase the old cursor?
		ImgFillRectangle(ConSurface, ConCursorX * 8, ConCursorY * 16, 8, 16, ConBackColor);											// Yes
	}
	
	ConCursorEnabled = e;																											// Set the cursor enabled prop
	
	ConUnlock();																													// Unlock
}

Boolean ConGetCursorEnabled(Void) {
	ConLock();																														// Lock
	Boolean s = ConCursorEnabled;																									// Save the cursor enabled prop
	ConUnlock();																													// Unlock
	return s;																														// Return it
}

Void ConSetColor(UIntPtr bg, UIntPtr fg) {
	ConLock();																														// Lock
	ConBackColor = bg;																												// Set the background of the console
	ConForeColor = fg;																												// Set the foreground of the console
	ConUnlock();																													// Unlock
}

Void ConSetBackground(UIntPtr c) {
	ConLock();																														// Lock
	ConBackColor = c;																												// Set the background of the console
	ConUnlock();																													// Unlock
}

Void ConSetForeground(UIntPtr c) {
	ConLock();																														// Lock
	ConForeColor = c;																												// Set the foreground of the console
	ConUnlock();																													// Unlock
}

Void ConGetColor(PUIntPtr bg, PUIntPtr fg) {
	ConLock();																														// Lock
	
	if (bg != Null) {																												// We should save the bg?
		*bg = ConBackColor;																											// Yes
	}
	
	if (fg != Null) {																												// And the fg?
		*fg = ConForeColor;																											// Yes
	}
	
	ConUnlock();																													// Unlock
}

UIntPtr ConGetBackground(Void) {
	ConLock();																														// Lock
	IntPtr bg = ConBackColor;																										// Save the bg
	ConUnlock();																													// Unlock
	return bg;																														// Return it
}

UIntPtr ConGetForeground(Void) {
	ConLock();																														// Lock
	IntPtr fg = ConForeColor;																										// Save the fg
	ConUnlock();																													// Unlock
	return fg;																														// Return it
}

Void ConSetCursor(UIntPtr x, UIntPtr y) {
	ConLock();																														// Lock
	ConCursorX = x;																													// Set the x position of the cursor
	ConCursorY = y;																													// Set the y position of the cursor
	ConUnlock();																													// Unlock
}

Void ConSetCursorX(UIntPtr pos) {
	ConLock();																														// Lock
	ConCursorX = pos;																												// Set the x position of the cursor
	ConUnlock();																													// Unlock
}

Void ConSetCursorY(UIntPtr pos) {
	ConLock();																														// Lock
	ConCursorY = pos;																												// Set the y position of the cursor
	ConUnlock();																													// Unlock
}

Void ConGetCursor(PUIntPtr x, PUIntPtr y) {
	ConLock();																														// Lock
	
	if (x != Null) {																												// We should save the x?
		*x = ConCursorX;																											// Yes
	}
	
	if (y != Null) {																												// And the y?
		*y = ConCursorY;																											// Yes
	}
	
	ConUnlock();																													// Unlock
}

UIntPtr ConGetCursorX(Void) {
	ConLock();																														// Lock
	IntPtr x = ConCursorX;																											// Save the x
	ConUnlock();																													// Unlock
	return x;																														// Return it
}

UIntPtr ConGetCursorY(Void) {
	ConLock();																														// Lock
	IntPtr y = ConCursorY;																											// Save the y
	ConUnlock();																													// Unlock
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
	ConLock();																														// Lock
	ImgClear(ConSurface, ConBackColor);																								// Clear the screen
	ConCursorX = ConCursorY = 0;																									// Move the cursor to 0, 0
	
	if (ConCursorEnabled) {																											// Draw the cursor?
		ImgFillRectangle(ConSurface, 0, 0, 8, 16, ConForeColor);																	// Yes
	}
	
	ConRefreshScreen();																												// Refresh the screen
	ConUnlock();																													// Unlock
}

static Void ConWriteCharacterInt(WChar data, Boolean cursor) {
	ImgWriteCharacter(ConSurface, cursor, &ConCursorX, &ConCursorY, ConBackColor, ConForeColor, data);								// Redirect to ImgWriteCharacter
}

Void ConWriteCharacter(WChar data) {
	ConLock();																														// Lock
	
	if (data != '\n' && ConCursorEnabled) {																							// Erase the old cursor?
		ImgFillRectangle(ConSurface, ConCursorX * 8, ConCursorY * 16, 8, 16, ConBackColor);											// Yes
	}
	
	ConWriteCharacterInt(data, ((data != '\n') && (data != '\r')) ? ConCursorEnabled : False);										// Write the character
	
	if (data == '\n' && ConCursorEnabled) {																							// Draw the new cursor?
		ImgFillRectangle(ConSurface, ConCursorX * 8, ConCursorY * 16, 8, 16, ConForeColor);											// Yes
	}
	
	ConRefreshScreen();																												// Refresh the screen
	ConUnlock();																													// Unlock
}

static Void ConWriteStringInt(PWChar data, Boolean cursor) {
	if (data == Null) {																												// Sanity check
		return;
	}
	
	ImgWriteString(ConSurface, cursor, &ConCursorX, &ConCursorY, ConBackColor, ConForeColor, data);									// Redirect to ImgWriteString
}

Void ConWriteString(PWChar data) {
	ConLock();																														// Lock
	ConWriteStringInt(data, ConCursorEnabled);																						// Write the string
	ConRefreshScreen();																												// Refresh the screen
	ConUnlock();																													// Unlock
}

static Void ConWriteIntegerInt(UIntPtr data, UInt8 base, Boolean cursor) {
	ImgWriteInteger(ConSurface, cursor, &ConCursorX, &ConCursorY, ConBackColor, ConForeColor, data, base);							// Redirect to ImgWriteIntegerr
}

Void ConWriteInteger(UIntPtr data, UInt8 base) {
	ConLock();																														// Lock
	ConWriteIntegerInt(data, base, ConCursorEnabled);																				// Write the integer
	ConRefreshScreen();																												// Refresh the screen
	ConUnlock();																													// Unlock
}

Void ConWriteFormated(PWChar data, ...) {
	if (data == Null) {
		return;
	}
	
	ConLock();																														// Lock
	
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
	ConUnlock();																													// Unlock
}
