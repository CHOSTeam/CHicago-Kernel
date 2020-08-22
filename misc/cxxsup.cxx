/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 25 of 2020, at 09:22 BRT
 * Last edited on August 02 of 2020, at 15:56 BRT */

#include <chicago/mm.hxx>

extern "C" Void __cxa_pure_virtual(Void) {
	/* GCC emits __cxa_pure_virtual calls when you try to call pure virtual functions (they aren't supposed to be called), later,
	 * we're supposed to error out here, but for now, just don't do anything. */
}

/* Those are the 4 ::new operators that we have to implement: two for normal allocations, and two for aligned allocations.
 * You maybe wondering, WTF is Long UInt32? Heap::Allocate takes UIntPtr, right? Yeah, you're right, but the ::new and the ::delete
 * operators take size_t (long unsigned int), which here, using our custom defines, is Long UInt32). */

Void *operator new(Long UInt32 Size) { return Heap::Allocate(Size); }
Void *operator new[](Long UInt32 Size) { return Heap::Allocate(Size); }
Void *operator new(Long UInt32 Size, AlignAlloc Align) { return Heap::Allocate(Size, (UIntPtr)Align); }
Void *operator new[](Long UInt32 Size, AlignAlloc Align) { return Heap::Allocate(Size, (UIntPtr)Align); }

/* On the ::delete side, we also have 8 operators to implement... */

Void operator delete(Void *Address) { Heap::Deallocate(Address); }
Void operator delete[](Void *Address) { Heap::Deallocate(Address); }
Void operator delete(Void *Address, Long UInt32 Size) { (Void)Size; Heap::Deallocate(Address); }
Void operator delete[](Void *Address, Long UInt32 Size) { (Void)Size; Heap::Deallocate(Address); }
Void operator delete(Void *Address, AlignAlloc Align) { (Void)Align; Heap::Deallocate(Address); }
Void operator delete[](Void *Address, AlignAlloc Align) { (Void)Align; Heap::Deallocate(Address); }
Void operator delete(Void *Address, Long UInt32 Size, AlignAlloc Align) { (Void)Size; (Void)Align; Heap::Deallocate(Address); }
Void operator delete[](Void *Address, Long UInt32 Size, AlignAlloc Align) { (Void)Size; (Void)Align; Heap::Deallocate(Address); }
