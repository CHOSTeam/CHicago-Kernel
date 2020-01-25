// File author is Ítalo Lima Marconato Matias
//
// Created on October 27 of 2018, at 14:19 BRT
// Last edited on January 25 of 2020, at 14:16 BRT

#include <chicago/console.h>
#include <chicago/debug.h>
#include <chicago/process.h>

Lock DbgLock = { 0, False, Null };
Boolean DbgRedirect = False;

Void DbgSetRedirect(Boolean red) {
	PsLock(&DbgLock);														// Lock
	DbgRedirect = red;														// Set the redirect prop
	PsUnlock(&DbgLock);														// Unlock
}

Boolean DbgGetRedirect(Void) {
	PsLock(&DbgLock);														// Lock
	Boolean red = DbgRedirect;												// Save the redirect prop
	PsUnlock(&DbgLock);														// Unlock
	return red;
}

static Void DbgWriteCharacterInt2(Char data) {
	DbgWriteCharacterInt(data);												// Write the character
	
	if (DbgRedirect) {														// Redirect to the console (maybe)
		ConWriteCharacter(data);
	}
}

Void DbgWriteCharacter(Char data) {
	PsLock(&DbgLock);														// Lock
	DbgWriteCharacterInt2(data);											// Write the character
	PsUnlock(&DbgLock);														// Unlock
}

static Void DbgWriteStringInt(PChar data) {
	for (UInt32 i = 0; data[i] != '\0'; i++) {
		DbgWriteCharacterInt2(data[i]);
	}
}

Void DbgWriteString(PChar data) {
	PsLock(&DbgLock);														// Lock!
	
	Boolean refresh = ConGetRefresh();
	
	if (DbgRedirect && refresh) {											// Redirect to the console?
		ConSetRefresh(False);												// Yes, disable the refresh
	}
	
	DbgWriteStringInt(data);												// Write the string
	
	if (DbgRedirect && refresh) {
		ConSetRefresh(True);												// Enable the refresh
		ConRefreshScreen();													// And refresh
	}
	
	PsUnlock(&DbgLock);														// Unlock
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
	PsLock(&DbgLock);														// Lock!
	
	Boolean refresh = ConGetRefresh();
	
	if (DbgRedirect && refresh) {											// Redirect to the console?
		ConSetRefresh(False);												// Yes, disable the refresh
	}
	
	DbgWriteIntegerInt(data, base);											// Write the integer
	
	if (DbgRedirect && refresh) {
		ConSetRefresh(True);												// Enable the refresh
		ConRefreshScreen();													// And refresh
	}
	
	PsUnlock(&DbgLock);														// Unlock
}

Void DbgWriteFormated(PChar data, ...) {
	PsLock(&DbgLock);														// Lock!
	
	VariadicList va;
	VariadicStart(va, data);												// Let's start our va list with the arguments provided by the user (if any)
	
	Boolean refresh = ConGetRefresh();
	
	if (DbgRedirect && refresh) {											// Redirect to the console?
		ConSetRefresh(False);												// Yes, disable the refresh
	}
	
	for (UInt32 i = 0; data[i] != '\0'; i++) {
		if (data[i] != '%') {												// It's an % (integer, string, character or other)?
			DbgWriteCharacterInt2(data[i]);									// Nope
		} else {
			switch (data[++i]) {											// Yes! So let's parse it!
			case 's': {														// String
				DbgWriteStringInt((PChar)VariadicArg(va, PChar));
				break;
			}
			case 'c': {														// Character
				DbgWriteCharacterInt2((Char)VariadicArg(va, Int));
				break;
			}
			case 'd': {														// Decimal Number
				DbgWriteIntegerInt((UIntPtr)VariadicArg(va, UIntPtr), 10);
				break;
			}
			case 'x': {														// Hexadecimal Number
				DbgWriteIntegerInt((UIntPtr)VariadicArg(va, UIntPtr), 16);
				break;
			}
			default: {														// None of the others...
				DbgWriteCharacterInt2(data[i]);
				break;
			}
			}
		}
	}
	
	if (DbgRedirect && refresh) {
		ConSetRefresh(True);												// Enable the refresh
		ConRefreshScreen();													// And refresh
	}
	
	VariadicEnd(va);
	PsUnlock(&DbgLock);														// Unlock
}
