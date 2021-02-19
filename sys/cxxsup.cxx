/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 25 of 2020, at 09:22 BRT
 * Last edited on February 18 of 2021, at 10:47 BRT */

#include <mm.hxx>

using namespace CHicago;

/* Those are the 4 ::new operators that we have to implement: two for normal allocations, and two for aligned
 * allocations. GCC for some reason used long unsigned int for this. */

Void *operator new(long unsigned int Size) { return Heap::Allocate(Size); }
Void *operator new[](long unsigned int Size) { return Heap::Allocate(Size); }
Void *operator new(long unsigned int Size, AlignAlloc Align) { return Heap::Allocate(Size, (UIntPtr)Align); }
Void *operator new[](long unsigned int Size, AlignAlloc Align) { return Heap::Allocate(Size, (UIntPtr)Align); }

/* On the ::delete side, we also have 8 operators to implement... */

Void operator delete(Void *Address)  { Heap::Deallocate(Address); }
Void operator delete[](Void *Address) { Heap::Deallocate(Address); }
Void operator delete(Void *Address, long unsigned int) { Heap::Deallocate(Address); }
Void operator delete[](Void *Address, long unsigned int) { Heap::Deallocate(Address); }
Void operator delete(Void *Address, AlignAlloc) { Heap::Deallocate(Address); }
Void operator delete[](Void *Address, AlignAlloc) { Heap::Deallocate(Address); }
Void operator delete(Void *Address, long unsigned int, AlignAlloc) { Heap::Deallocate(Address); }
Void operator delete[](Void *Address, long unsigned int, AlignAlloc) { Heap::Deallocate(Address); }
