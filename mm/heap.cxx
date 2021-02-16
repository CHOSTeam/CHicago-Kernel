/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 14 of 2021, at 23:45 BRT
 * Last edited on February 16 of 2021, at 10:31 BRT */

#include <mm.hxx>
#include <panic.hxx>

using namespace CHicago;

Boolean Heap::Initialized = False;
UIntPtr Heap::Start = 0, Heap::End = 0, Heap::Current = 0, Heap::CurrentAligned = 0;

Void Heap::Initialize(UIntPtr Start, UIntPtr End) {
    ASSERT(!Initialized);
    ASSERT(Start != End);
    ASSERT(Start >= PhysMem::GetMinAddress() && End >= PhysMem::GetMinAddress());

    Heap::Start = Current = CurrentAligned = Start;
    Heap::End = End;
    Initialized = True;
}

Status Heap::Increment(UIntPtr Amount) {
	/* We need to check for two things: First, if the Amount is even valid (if it isn't, return InvalidArg), second, if
	 * we aren't trying to expand beyond the heap limit. */

    if (!Amount) {
        return Status::InvalidArg;
    } else if ((Current + Amount) < Current || (Current + Amount) >= End) {
        return Status::OutOfMemory;
    }

	/* Now, we need to expand the heap, but this is not just a matter of increasing the Heap::Current variable, as we
	 * need to make sure that the space we're expanding into is going to be mapped into the memory. */
	
	UIntPtr nw = Current + Amount, phys;
	Status status;

	for (; CurrentAligned < nw; CurrentAligned += PAGE_SIZE) {
		if ((status = PhysMem::ReferenceSingle(0, phys)) != Status::Success) {
			return status;
		} else if ((status = VirtMem::Map(CurrentAligned, phys, PAGE_SIZE, MAP_RW)) != Status::Success) {
			PhysMem::DereferenceSingle(phys);
			return status;
		}
	}

    return Current = nw, Status::Success;
}

Status Heap::Decrement(UIntPtr Amount) {
    if (!Amount || (Current - Amount) > Current || (Current - Amount) >= End) {
        return Status::InvalidArg;
    }

    /* Decrease all the variables, and free all the physical pages that we allocated. */

    Current -= Amount;

    for (; CurrentAligned - PAGE_SIZE >= Current;) {
        UInt32 flags;
        UIntPtr phys;

        CurrentAligned -= PAGE_SIZE;

        if (VirtMem::Query(CurrentAligned, phys, flags) == Status::Success) {
            PhysMem::DereferenceSingle(phys);
        }
    }

    return Status::Success;
}
