/* File author is Ítalo Lima Marconato Matias
 *
 * Created on August 25 of 2018, at 10:52 BRT
 * Last edited on October 09 of 2020, at 20:50 BRT */

#ifndef __CHICAGO_DRIVER_HXX__
#define __CHICAGO_DRIVER_HXX__

#include <chicago/status.hxx>

/* The most basic functions, that should ALWAYS be avaliable: Halt, GetInterface, RegisterInterface and
 * UnregisterInterface. They are inside of the Kernel interface. They allow us to manipulate other interfaces
 * (get interfaces that other drivers or that the kernel registered). */

struct packed KernelInterface {
	Void (*Halt)(Void);
	const Void *(*GetInterface)(const Char*);
	Status (*RegisterInterface)(const Char*, const Void*, UIntPtr);
	Status (*UnregisterInterface)(const Char*);
};

/* Now the other main interfaces (that can be get as we need them): Display, PhysMem, VirtMem, Heap and FileSys.
 * They aren't integrated on the main kernel interface, and you need to ->GetInterface them. */

struct packed DisplayInterface {
	struct packed Mode {
		UIntPtr Width, Height;
		Void *Buffer;
	};
	
	struct packed Impl {
		Status (*SetResolution)(UIntPtr, UIntPtr, Mode*);
	};
	
	Status (*Register)(const Impl*, const Mode*, const Mode*);
	const Mode *(*GetMode)(Void);
};

struct packed PhysMemInterface {
	Status (*AllocSingle)(UIntPtr*);
	Status (*AllocContig)(UIntPtr, UIntPtr*);
	Status (*AllocNonContig)(UIntPtr, UIntPtr*);
	
	Status (*FreeSingle)(UIntPtr);
	Status (*FreeContig)(UIntPtr, UIntPtr);
	Status (*FreeNonContig)(UIntPtr*, UIntPtr);
	
	Status (*ReferenceSingle)(UIntPtr, UIntPtr*);
	Status (*ReferenceContig)(UIntPtr, UIntPtr, UIntPtr*);
	Status (*ReferenceNonContig)(UIntPtr*, UIntPtr, UIntPtr*);
	
	Status (*DereferenceSingl)(UIntPtr);
	Status (*DereferenceContig)(UIntPtr, UIntPtr);
	Status (*DereferenceNonContig)(UIntPtr*, UIntPtr);
	
	UInt8 (*GetReferences)(UIntPtr);
	UInt64 (*GetSize)(Void);
	UInt64 (*GetUsage)(Void);
};

struct packed VirtMemInterface {
	Status (*GetPhys)(Void*, UIntPtr*);
	Status (*Query)(Void*, UInt32*);
	Status (*MapTemp)(UIntPtr, UInt32, Void**);
	Status (*Map)(Void*, UIntPtr, UInt32);
	Status (*Unmap)(Void*);
	Status (*CreateDirectory)(UIntPtr*);
	Status (*FreeDirectory)(UIntPtr);
	UIntPtr (*GetCurrentDirectory)(Void);
	Void (*SwitchDirectory)(UIntPtr);
};

struct packed HeapInterface {
	Status (*Increment)(UIntPtr);
	Status (*Decrement)(UIntPtr);
	
	Void *(*Allocate)(UIntPtr);
	Void *(*AllocateAlign)(UIntPtr, UIntPtr);
	Void (*Deallocate)(Void*);
	
	UIntPtr (*GetStart)(Void);
	UIntPtr (*GetEnd)(Void);
	UIntPtr (*GetCurrent)(Void);
};

struct packed FileSysInterface {
	struct packed Impl {
		Status (*Open)(const Void*, UInt64, UInt8);
		Void (*Close)(const Void*, UInt64);
		Status (*Read)(const Void*, UInt64, UInt64, UInt64, Void*, UInt64*);
		Status (*Write)(const Void*, UInt64, UInt64, UInt64, const Void*, UInt64*);
		Status (*ReadDirectory)(const Void*, UInt64, UIntPtr, Char**);
		Status (*Search)(const Void*, UInt64, const Char*, Void**, UInt64*, UInt64*);
		Status (*Create)(const Void*, UInt64, const Char*, UInt8);
		Status (*Control)(const Void*, UInt64, UIntPtr, const Void*, Void*);
		Status (*Mount)(const Void*, Void**, UInt64*);
		Status (*Unmount)(const Void*, UInt64);
		const Char *Name;
	};
	
	Status (*Register)(const Impl*);
	
	Status (*Open)(const Char*, UInt8, Void**);
	Status (*Mount)(const Char*, const Char*, UInt8);
	Status (*Umount)(const Char*);
	
	Status (*Read)(Void*, UInt64, UInt64, Void*, UInt64*);
	Status (*Write)(Void*, UInt64, UInt64, const Void*, UInt64*);
	Status (*ReadDirectory)(Void*, UIntPtr, Char**);
	Status (*Search)(Void*, const Char*, UInt8, Void**);
	Status (*Create)(Void*, const Char*, UInt8);
	Status (*Control)(Void*, UIntPtr, const Void*, Void*);
};

/* And now one extra interface, which we're probably going to remove later... The Debug interface, which allows us
 * to output text to the serial/debug port. */

struct packed DebugInterface {
	Void (*Write)(Char);
};

#endif
