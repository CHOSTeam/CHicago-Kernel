// File author is Ítalo Lima Marconato Matias
//
// Created on October 27 of 2018, at 14:19 BRT
// Last edited on February 02 of 2020, at 13:14 BRT

#include <chicago/console.h>
#include <chicago/debug.h>
#include <chicago/process.h>

PThread DbgLockOwner = Null;
Boolean DbgRedirect = False;

static Void DbgLock(Void) {
	if (PsCurrentThread == Null) {													// If threading isn't initialized, we don't need to do anything
		return;
	}
	
	PsWaitForAddress((UIntPtr)&DbgLockOwner, 0, (UIntPtr)PsCurrentThread, 0);		// Wait until the DbgLockOwner is Null
}

static Void DbgUnlock(Void) {
	if (PsCurrentThread != Null && DbgLockOwner == PsCurrentThread) {				// Is threading initialized? If yes, are we the owners?
		DbgLockOwner = Null;														// Yeah, set it to Null, and wake any thread that is waiting for us
		PsWakeAddress((UIntPtr)&DbgLockOwner);
	}
}

Void DbgSetRedirect(Boolean red) {
	DbgLock();																		// Lock
	DbgRedirect = red;																// Set the redirect prop
	DbgUnlock();																	// Unlock
}

Boolean DbgGetRedirect(Void) {
	DbgLock();																		// Lock
	Boolean red = DbgRedirect;														// Save the redirect prop
	DbgUnlock();																	// Unlock
	return red;
}

static Void DbgWriteCharacterInt2(Char data) {
	DbgWriteCharacterInt(data);														// Write the character
	
	if (DbgRedirect) {																// Redirect to the console (maybe)
		ConWriteCharacter(data);
	}
}

Void DbgWriteCharacter(Char data) {
	DbgLock();																		// Lock
	DbgWriteCharacterInt2(data);													// Write the character
	DbgUnlock();																	// Unlock
}

static Void DbgWriteStringInt(PChar data) {
	for (UInt32 i = 0; data[i] != '\0'; i++) {
		DbgWriteCharacterInt2(data[i]);
	}
}

Void DbgWriteString(PChar data) {
	DbgLock();																		// Lock!
	
	Boolean refresh = ConGetRefresh();
	
	if (DbgRedirect && refresh) {													// Redirect to the console?
		ConSetRefresh(False);														// Yes, disable the refresh
	}
	
	DbgWriteStringInt(data);														// Write the string
	
	if (DbgRedirect && refresh) {
		ConSetRefresh(True);														// Enable the refresh
		ConRefreshScreen();															// And refresh
	}
	
	DbgUnlock();																	// Unlock
}

static Void DbgWriteIntegerInt(UIntPtr data, UInt8 base) {
	if (data == 0) {
		DbgWriteCharacterInt2('0');
		return;
	}
	
	static Char buf[32] = { 0 };
	Int i = 30;
	
	for (; data && i; i--, data /= base) {
		buf[i] = "0123456789ABCDEF"[data % base];
	}
	
	DbgWriteStringInt(&buf[i + 1]);
}

Void DbgWriteInteger(UIntPtr data, UInt8 base) {
	DbgLock();																		// Lock!
	
	Boolean refresh = ConGetRefresh();
	
	if (DbgRedirect && refresh) {													// Redirect to the console?
		ConSetRefresh(False);														// Yes, disable the refresh
	}
	
	DbgWriteIntegerInt(data, base);													// Write the integer
	
	if (DbgRedirect && refresh) {
		ConSetRefresh(True);														// Enable the refresh
		ConRefreshScreen();															// And refresh
	}
	
	DbgUnlock();																	// Unlock
}

Void DbgWriteFormated(PChar data, ...) {
	DbgLock();																		// Lock!
	
	VariadicList va;
	VariadicStart(va, data);														// Let's start our va list with the arguments provided by the user (if any)
	
	Boolean refresh = ConGetRefresh();
	
	if (DbgRedirect && refresh) {													// Redirect to the console?
		ConSetRefresh(False);														// Yes, disable the refresh
	}
	
	for (UInt32 i = 0; data[i] != '\0'; i++) {
		if (data[i] != '%') {														// It's an % (integer, string, character or other)?
			DbgWriteCharacterInt2(data[i]);											// Nope
		} else {
			switch (data[++i]) {													// Yes! So let's parse it!
			case 's': {																// String
				DbgWriteStringInt((PChar)VariadicArg(va, PChar));
				break;
			}
			case 'c': {																// Character
				DbgWriteCharacterInt2((Char)VariadicArg(va, Int));
				break;
			}
			case 'd': {																// Decimal Number
				DbgWriteIntegerInt((UIntPtr)VariadicArg(va, UIntPtr), 10);
				break;
			}
			case 'x': {																// Hexadecimal Number
				DbgWriteIntegerInt((UIntPtr)VariadicArg(va, UIntPtr), 16);
				break;
			}
			default: {																// None of the others...
				DbgWriteCharacterInt2(data[i]);
				break;
			}
			}
		}
	}
	
	if (DbgRedirect && refresh) {
		ConSetRefresh(True);														// Enable the refresh
		ConRefreshScreen();															// And refresh
	}
	
	VariadicEnd(va);
	DbgUnlock();																	// Unlock
}
