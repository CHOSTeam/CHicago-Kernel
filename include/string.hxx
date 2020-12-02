/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 25 of 2020, at 11:19 BRT
 * Last edited on October 24 of 2020, at 14:35 BRT */

#ifndef __STRING_HXX__
#define __STRING_HXX__

#include <list.hxx>

class ByteHelper {
public:
	static Void Convert(Float, Char&, UIntPtr&, UIntPtr&);
};

class String {
public:
	/* Constructors: One that takes no argument, and you can later just .Append anything you want, one that
	 * takes how many characters we should pre-allocate, one that takes a reference to another String, and
	 * one that takes a normal C string. */
	
	String();
	String(UIntPtr);
	String(const String&);
	String(const Char*, Boolean = False);
	
	/* We need a destructor as well, for (if required) freeing the Value buffer. */
	
	~String(Void);
	
	/* And the copy operators... Which frees the whole string, and call the constructor (actually ::Append)
	 * again. */
	
	String &operator =(const String&);
	String &operator =(const Char*);
	
	/* You maybe wondering: Hey, why doesn't the constructors allow initializing the string with formatted
	 * input? Well, for this we have the Format function! And yeah, we actually need to make their body here
	 * as well, because of all the template<> stuff. */
	
	template<typename... Args> static String Format(const Char *Format, Args... VaArgs) {
		/* First, let's create the string itself, we're going to use the 0-args initializer, as the Append
		 * function is going to dynamically allocate the memory we need. */
		
		String str;
		
		/* Now we just need to call the Append function. */
		
		if (Format == Null) {
			return str;
		} else if (str.Append(Format, VaArgs...) != Status::Success) {
			return String();
		}
		
		return str;
	}
	
	/* Let's give the user an easy way to clear the string. */
	
	Void Clear(Void);
	
	/* Append functions: One for a single character, one for signed numbers, one for unsigned numbers, one for
	 * CHicago strings, and one for C strings. */
	
	Status Append(Char);
	Status Append(IntPtr, UInt8, UIntPtr = 0, Char = ' ');
	Status Append(UIntPtr, UInt8, UIntPtr = 0, Char = ' ');
	Status Append(const String&);
	Status Append(const Char*, ...);
	
	/* The tokenize and compare functions only have one form, that takes a String, if you pass a Char*, it will
	 * be auto converted into String&. */
	
	Boolean Compare(const String&) const;
	List<String> Tokenize(const String&) const;
	
	/* The remaining functions are: ToCString, which just returns ->Value; ToMutCString, which makes sure that the
	 * user can use/change the returned C string, and that we're not going to overwrite it/delete it, And GetLength,
	 * which, as you may think, returns ->Length. */
	
	const Char *ToCString(Void) const { return Value; }
	Char *ToMutCString(Void) const;
	
	UIntPtr GetLength(Void) const { return Length; }
	UIntPtr GetSize(Void) const { return Allocated; }
	
	/* Now, let's allow us to use ranged-for and access it as an array. */
	
	Char *begin(Void) { return &Value[0]; }
	Char *begin(Void) const { return &Value[0]; }
	Char *end(Void) { return &Value[Length]; }
	Char *end(Void) const { return &Value[Length]; }
	
	Char operator [](UIntPtr Index) const { return Value[Index]; }
private:
	Status AppendInt(const Char*, UIntPtr, Char);
	
	Char *Value;
	UIntPtr Length;
	UIntPtr Allocated;
};

Void StrCopyMemory(Void*, const Void*, UIntPtr);
Void StrCopyMemory32(Void*, const Void*, UIntPtr);
Void StrSetMemory(Void*, UInt8, UIntPtr);
Void StrSetMemory32(Void*, UInt32, UIntPtr);
Void StrMoveMemory(Void*, const Void*, UIntPtr);
Boolean StrCompareMemory(const Void *const, const Void *const, UIntPtr);

#endif
