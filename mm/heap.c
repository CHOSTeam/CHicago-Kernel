// File author is Ítalo Lima Marconato Matias
//
// Created on June 29 of 2018, at 22:30 BRT
// Last edited on February 02 of 2020, at 13:04 BRT

#include <chicago/mm.h>

UIntPtr HeapEnd = 0;
UIntPtr HeapStart = 0;
UIntPtr HeapCurrent = 0;
UIntPtr HeapCurrentAligned = 0;

UIntPtr HeapGetCurrent(Void) {
	PsLockTaskSwitch(old);																	// Lock task switching
	UIntPtr ret = HeapCurrent;
	PsUnlockTaskSwitch(old);																// Unlock task switching
	return ret;
}

UIntPtr HeapGetStart(Void) {
	return HeapStart;
}

UIntPtr HeapGetEnd(Void) {
	return HeapEnd;
}

Boolean HeapIncrement(UIntPtr amount) {
	if (amount <= 0) {																		// The increment is 0?
		return False;																		// Yes, so it's invalid
	} else if ((HeapCurrent + amount) >= HeapEnd) {											// Trying to expand beyond the limit?
		return False;																		// Yes
	}
	
	PsLockTaskSwitch(old);																	// Lock task switching
	
	UIntPtr new = HeapCurrent + amount;
	UIntPtr old = HeapCurrentAligned;
	
	for (; HeapCurrentAligned < new; HeapCurrentAligned += MM_PAGE_SIZE) {
		UIntPtr phys;																		// Allocate one new page
		
		if (MmReferenceSinglePage(0, &phys) != STATUS_SUCCESS) {							// Failed?
			for (UIntPtr i = old; i < HeapCurrentAligned; i += MM_PAGE_SIZE) {				// Yes, undo everything
				MmDereferenceSinglePage(MmGetPhys(i));
				MmUnmap(i);
			}
			
			HeapCurrentAligned = old;
			
			PsUnlockTaskSwitch(old);
			
			return False;
		}
		
		if (MmMap(HeapCurrentAligned, phys, MM_MAP_KDEF) != STATUS_SUCCESS) {				// Now try to map it
			MmDereferenceSinglePage(phys);													// Failed, so undo everything
			
			for (UIntPtr i = old; i < HeapCurrentAligned; i += MM_PAGE_SIZE) {
				MmDereferenceSinglePage(MmGetPhys(i));
				MmUnmap(i);
			}
			
			HeapCurrentAligned = old;
			
			PsUnlockTaskSwitch(old);
			
			return False;
		}
	}
	
	HeapCurrent = new;
	
	PsUnlockTaskSwitch(old);																// Unlock task switching
	
	return True;
}

Boolean HeapDecrement(UIntPtr amount) {
	if (amount <= 0) {																		// The decrement is 0?
		return False;																		// Yes, so it's zero
	} else if ((HeapCurrent - amount) < HeapStart) {										// Trying to decrement too much?
		return False;																		// Yes
	}
	
	PsLockTaskSwitch(old);																	// Lock task switching
	
	HeapCurrent -= amount;																	// Decrement the unaligned pointer
	
	while ((HeapCurrentAligned - MM_PAGE_SIZE) > HeapCurrent) {								// And the aligned one
		HeapCurrentAligned -= MM_PAGE_SIZE;
		MmDereferenceSinglePage(MmGetPhys(HeapCurrentAligned));								// Dereference the physical page
		MmUnmap(HeapCurrentAligned);														// And unmap
	}
	
	PsUnlockTaskSwitch(old);																// Unlock task switching
	
	return True;
}

Void HeapInit(UIntPtr start, UIntPtr end) {
	if ((start % MM_PAGE_SIZE) != 0) {														// Align the start addr
		start += MM_PAGE_SIZE - (start % MM_PAGE_SIZE);
	}
	
	if ((end % MM_PAGE_SIZE) != 0) {														// Align the end addr
		end += MM_PAGE_SIZE - (end % MM_PAGE_SIZE);
	}
	
	HeapStart = HeapCurrent = HeapCurrentAligned = start;									// Set the start, curr and aligned curr vars
	HeapEnd = end;																			// And the end var
}
