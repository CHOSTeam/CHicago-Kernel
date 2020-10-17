/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 01 of 2020, at 16:12 BRT
 * Last edited on August 25 of 2020, at 13:20 BRT */

#ifndef __CHICAGO_STATUS_HXX__
#define __CHICAGO_STATUS_HXX__

#include <chicago/types.hxx>

/* With C++, we can use an "enum class", instead of using a bunch of defines, this makes
 * everything a bit prettier, and also easier to relocate the codes up and down/add new
 * ones lol. */

enum class Status {
	/* Used on the arch-specific syscall handler. */
	
	InvalidSyscall = -2, WrongHandle = -1,
	
	/* Two status codes that doesn't go anywhere else. */
	
	Success = 0, InvalidArg, Unsupported, NotFound, DoesntExist, AlreadyExists,
	
	/* Memory errors. */
	
	OutOfMemory, MapError, UnmapError, NotMapped, AlreadyMapped, MapFileFailed,
	UnmapFileFailed,
	
	/* File errors (that is, status codes used on file functions. */
	
	InvalidFile, NotDirectory, NotFile, NotMounted, AlreadyMounted, CantRead, CantWrite,
	CantExec, MountFailed, UmountFailed, OpenFailed, ReadFailed, WriteFailed, ControlFailed,
	InvalidControlCmd, InvalidFs, EOF,
	
	/* Elf loading errors. */
	
	NotElf, WrongElf, CorruptedElf, InvalidArch, AlreadyLoaded,
	
	/* Three in one: Errors from the process functions, error from the port functions,
	 * and errors form the shared memory functions. */
	
	InvalidProcess, InvalidThread, ActionDenied, InvalidPort, NotOwner, InvalidShmSect
};

#endif
