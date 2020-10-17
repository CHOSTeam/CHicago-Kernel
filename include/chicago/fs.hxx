/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 06 of 2020, at 10:01 BRT
 * Last edited on October 10 of 2020, at 11:38 BRT */

#ifndef __CHICAGO_FS_HXX__
#define __CHICAGO_FS_HXX__

#include <chicago/list.hxx>
#include <chicago/string.hxx>

#define OPEN_DIR 0x01
#define OPEN_READ 0x02
#define OPEN_WRITE 0x04
#define OPEN_EXEC 0x08
#define OPEN_RW (OPEN_READ | OPEN_WRITE)
#define OPEN_RX (OPEN_READ | OPEN_EXEC)
#define OPEN_RWX (OPEN_RW | OPEN_EXEC)
#define OPEN_CREATE 0x10
#define OPEN_RECUR_CREATE 0x20
#define OPEN_ONLY_CREATE 0x40

#define FILE_FLAGS_MASK (OPEN_DIR | OPEN_READ | OPEN_WRITE | OPEN_EXEC)

/* You may be wondering: Why are we using typedefs for those things, why not use pure virtual classes?
 * Portability, we want those interfaces to be portable across compilers/compiler versions, so it's better
 * to use packed C structs. */

struct packed FsImpl {
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

class File {
public:
	File(Void);
	File(const String&, UInt8, const FsImpl*, UInt64, const Void*, UInt64);
	File(const File&);
	~File(Void);
	
	File &operator =(const File&);
	
	/* This is just an whole class is just an easy accessor for all the fields that we need to
	 * read/write/manipulate files. Only the FileSys class and the kernel entry (probably) call the
	 * non-empty & non-copy constructor. There is no Close function, as the destructor already calls
	 * Fs->Close. */
	
	Status Read(UInt64, UInt64, Void*, UInt64&) const;
	Status Write(UInt64, UInt64, const Void*, UInt64&) const;
	Status ReadDirectory(UIntPtr, String&) const;
	Status Search(const String&, UInt8, File&) const;
	Status Create(const String&, UInt8) const;
	Status Control(UIntPtr, const Void*, Void*) const;
	Status Unmount(Void) const;
	
	const String &GetName(Void) const { return Name; }
	UInt64 GetLength(Void) const { return Length; }
	UInt8 GetFlags(Void) const { return Flags; }
	UIntPtr GetRefs(Void) const { return *References; }
private:
	String Name;
	UInt8 Flags;
	const FsImpl *Fs;
	UInt64 Length;
	const Void *Priv;
	UInt64 INode;
	UIntPtr *References;
};

class MountPoint {
public:
	MountPoint(const String &Path, const File &Root) : Path(Path), Root(Root) { }
	
	const String &GetPath(Void) const { return Path; }
	const File &GetRoot(Void) const { return Root; }
private:
	const String Path;
	const File Root;
};

class FileSys {
public:
	/* Some easy path manipulation functions. We return pointers here, and we expect that the user will
	 * ranged-for delete everything in the list, and delete the pointers themselves. */
	
	static List<String> TokenizePath(const String&);
	static String CanonicalizePath(const String&, const String& = "");
	
	/* If you didn't got it untill now, everything that is accessable from the kernel drivers (and that
	 * the driver can manipulate/add new "handlers") have a ::Register function. */
	
	static Status Register(const FsImpl*);
	static Status CheckMountPoint(const String&);
	static Status CreateMountPoint(const String&, const File&);
	
	static Status Open(const String&, UInt8, File&);
	static Status Mount(const String&, const String&, UInt8);
	static Status Unmount(const String&);
private:
	static const FsImpl *GetFileSys(const String&);
	static const MountPoint *GetMountPoint(const String&, String&);
	
	static List<const FsImpl*> FileSystems;
	static List<const MountPoint*> MountPoints;
};

#endif
