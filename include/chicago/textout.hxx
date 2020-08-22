/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 26 of 2020, at 13:16 BRT
 * Last edited on August 22 of 2020, at 11:51 BRT */

#ifndef __CHICAGO_TEXTOUT_HXX_
#define __CHICAGO_TEXTOUT_HXX_

#include <chicago/string.hxx>

/* What would our kernel be without any way to display informations for the user
 * (our for debugging)? TextOutput is CHicago's kernel default interface for anything
 * that can output text to the screen (for now only the Debug functions). It is the
 * equivalent of the ->Append functions from the String class, BUT it doesn't try
 * allocating anything (any memory).
 * To inherit this class you only need to implement the ->WriteInt(Char) function. */

class TextOutput {
public:
	Void Write(Char);
	Void Write(const String&);
	Void Write(const Char*, ...);
	Void Write(IntPtr, UInt8, UIntPtr = 0, Char = ' ');
	Void Write(UIntPtr, UInt8, UIntPtr = 0, Char = ' ');
private:
	virtual Void AfterWrite(Void) { }
	virtual Void WriteInt(Char) = 0;
	
	Void WriteInt(const Char*, UIntPtr, Char);
	Void WriteInt(IntPtr, UInt8, UIntPtr = 0, Char = ' ');
	Void WriteInt(UIntPtr, UInt8, UIntPtr = 0, Char = ' ');
};

extern TextOutput *Debug;
extern TextOutput *Console;

#endif
