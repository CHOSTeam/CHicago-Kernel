/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 25 of 2020, at 09:22 BRT
 * Last edited on December 02 of 2020, at 08:38 BRT */

#include <string.hxx>

/* We should probably move this def outside later, but, for now, we only use it here, and I don't think we
 * need to move it. */

#ifdef ARCH_64
#define UIntSize ULong
#else
#define UIntSize UInt32
#endif

extern "C" Void __cxa_pure_virtual(Void) {
	/* GCC emits __cxa_pure_virtual calls when you try to call pure virtual functions (they aren't supposed
	 * to be called), later, we're supposed to error out here, but for now, just don't do anything. */
}

extern "C" Int atexit(Void (*)(Void)) {
	/* Clang seems to call atexit anyways, so let's just dummy implement it. */
	
	return 1;
}

extern "C" Void memset(Void *Pointer, UInt8 Data, UIntPtr Count) {
	/* Again, clang calls memset anyways, let's redirect into our StrSetMemory. */
	
	StrSetMemory(Pointer, Data, Count);
}

extern "C" Void *memcpy(Void *Dest, const Void *Source, UIntPtr Size) {
	/* Same thing as memset, but call StrCopyMemory this time. */
	
	StrCopyMemory(Dest, Source, Size);
	return Dest;
}

/* Those are the 4 ::new operators that we have to implement: two for normal allocations, and two for aligned
 * allocations. GCC for some reason used long unsigned int for this. */

Void *operator new(UIntSize Size) { return Heap::Allocate(Size); }
Void *operator new[](UIntSize Size) { return Heap::Allocate(Size); }
Void *operator new(UIntSize Size, AlignAlloc Align) { return Heap::Allocate(Size, (UIntPtr)Align); }
Void *operator new[](UIntSize Size, AlignAlloc Align) { return Heap::Allocate(Size, (UIntPtr)Align); }

/* On the ::delete side, we also have 8 operators to implement... */

Void operator delete(Void *Address) { Heap::Deallocate(Address); }
Void operator delete[](Void *Address) { Heap::Deallocate(Address); }
Void operator delete(Void *Address, UIntSize) { Heap::Deallocate(Address); }
Void operator delete[](Void *Address, UIntSize) { Heap::Deallocate(Address); }
Void operator delete(Void *Address, AlignAlloc) { Heap::Deallocate(Address); }
Void operator delete[](Void *Address, AlignAlloc) { Heap::Deallocate(Address); }
Void operator delete(Void *Address, UIntSize, AlignAlloc) { Heap::Deallocate(Address); }
Void operator delete[](Void *Address, UIntSize, AlignAlloc) { Heap::Deallocate(Address); }
