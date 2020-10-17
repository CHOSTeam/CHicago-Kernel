/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 15 of 2020, at 23:15 BRT
 * Last edited on October 09 of 2020, at 23:17 BRT */

#ifndef __CHICAGO_SIAFS_HXX__
#define __CHICAGO_SIAFS_HXX__

#include <chicago/fs.hxx>

/* The SIA file format is what we use for storing the initrd (that is, the drivers and the some other
 * arch-specific things). While we can load a SIA file from some already loaded filesystem, we're probably
 * going to need to load it from the memory (for the initrd), so we need some way to access the memory as
 * if it was a file. This "MemFs" is not avaliable as an actual FS, so you can't just ::Mount some random
 * virtual memory location. */

#define SIA_MAGIC 0xC4051AF0
#define SIA_INFO_KERNEL 0x01
#define SIA_INFO_FIXED 0x02
#define SIA_FLAGS_DIRECTORY 0x01
#define SIA_FLAGS_READ 0x02
#define SIA_FLAGS_WRITE 0x04
#define SIA_FLAGS_EXEC 0x08

class MemFs {
public:
	MemFs(Void*, UInt64);
	
	static Status MakeFile(Void*, UInt64, UInt8, File&);
private:
	/* We still need the close function, as we need to free the priv data that MakeFile allocated. */
	
	static Void Close(const Void*, UInt64);
	static Status Read(const Void*, UInt64, UInt64, UInt64, Void*, UInt64*);
	static Status Write(const Void*, UInt64, UInt64, UInt64, const Void*, UInt64*);
	
	static const FsImpl Impl;
	Void *Location;
	UInt64 Length;
};

class SiaFs {
private:
	/* All the headers are directly copied from the sia.h file (of the x86 bootloader), except that it is adapted
	 * to C++ and to the CHicago type naming system... except for the PrivData struct, this was created only for
	 * the kernel SIA file(system) implementation. */
	
	struct packed Header {
		UInt32 Magic;
		UInt16 Info;
		UInt8 UUID[16];
		UInt64 FreeFileCount, FreeFileOffset;
		UInt64 FreeDataCount, FreeDataOffset;
		UInt64 KernelOffset, RootOffset;
	};
	
	struct packed FileHeader {
		UInt64 Next;
		UInt16 Flags;
		UInt64 Size;
		UInt64 Offset;
		Char Name[64];
	};
	
	struct packed DataHeader {
		UInt64 Next;
		UInt8 Contents[504];
	};
	
	struct PrivData {
		SiaFs *Fs;
		UInt64 Offset;
		FileHeader Hdr;
	};
public:
	/* Ok, I know what you're thinking, that the ::Header field is private and etc, but this constructor is only
	 * supposed to be called by the Mount function (and by the MountRamFs function), so we don't care about the
	 * user having problems with it. */
	
	SiaFs(const File&, const Header&, Void*, UIntPtr);
	~SiaFs(Void);
	
	static Status Register(Void);
	
	static Status MountRamFs(const String&, Void*, UInt64);
private:
	static Status CheckHeader(const Header&, UInt64 Length);
	static Status CheckPrivData(const Void*, const PrivData*&);
	static Status CheckPrivData(const Void*, PrivData*&);
	
	static Status Open(const Void*, UInt64, UInt8);
	static Void Close(const Void*, UInt64);
	static Status Read(const Void*, UInt64, UInt64, UInt64, Void*, UInt64*);
	static Status Write(const Void*, UInt64, UInt64, UInt64, const Void*, UInt64*);
	static Status ReadDirectory(const Void*, UInt64, UIntPtr, Char**);
	static Status Search(const Void*, UInt64, const Char*, Void**, UInt64*, UInt64*);
	static Status Create(const Void*, UInt64, const Char*, UInt8);
	static Status Mount(const Void*, Void**, UInt64*);
	static Status Unmount(const Void*, UInt64);
	
	/* Finally, some non-static functions, they are used for allocating new file/data entries, we could implement
	 * them as static functions, but it's easier to just do this. */
	
	Status GoToOffset(FileHeader&, UInt64, UInt64, UInt64&, Boolean = False);
	Status AllocDataEntry1(UInt64);
	Status AllocDataEntry2(UInt64&);
	Status AllocFileEntry1(const String&, UInt8, UInt64);
	Status AllocFileEntry2(const String&, UInt8, UInt64&);
	Status LinkFileEntry(FileHeader&, UInt64, UInt64);
	
	static const FsImpl Impl;
	const File Source;
	Header Hdr;
	Void *ToFree;
	UIntPtr ExpandLoc;
};

#endif
