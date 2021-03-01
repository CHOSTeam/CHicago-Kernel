/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 28 of 2021, at 14:01 BRT
 * Last edited on March 01 of 2021 at 11:41 BRT */

#pragma once

#include <list.hxx>
#include <string.hxx>

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

namespace CHicago {

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
    File();
    File(File&&);
    File(const File&);
    File(const String&, UInt8, const FsImpl&, UInt64, const Void*, UInt64);
    ~File();

    File &operator =(File&&);
    File &operator =(const File&);

    /* This is just an whole class is just an easy accessor for all the fields that we need to read/write/manipulate
     * files. Only the FileSys class and the kernel entry (probably) call the non-empty & non-copy constructor. */

    Status Read(UInt64, UInt64, Void*, UInt64&) const;
    Status Write(UInt64, UInt64, const Void*, UInt64&) const;
    Status ReadDirectory(UIntPtr, String&) const;
    Status Search(const String&, UInt8, File&) const;
    Status Create(const String&, UInt8) const;
    Status Control(UIntPtr, const Void*, Void*) const;
    Status Unmount() const;
    Void Close();

    inline const String &GetName() const { return Name; }
    inline UInt64 GetLength() const { return Length; }
    inline UInt8 GetFlags() const { return Flags; }
private:
    String Name;
    UInt8 Flags;
    FsImpl Fs;
    const Void *Priv;
    UIntPtr *References;
    UInt64 Length, INode;
};

class MountPoint {
public:
    MountPoint() : Root(), Path() { }
    MountPoint(const MountPoint &Source) = default;
    MountPoint(const String &Path, const File &Root) : Root(Root), Path(Path) { }
    MountPoint(MountPoint &&Source) : Root(move(Source.Root)), Path(move(Source.Path)) { }

    MountPoint &operator =(MountPoint&&);
    MountPoint &operator =(const MountPoint&);

    inline const File &GetRoot() const { return Root; }
    inline const String &GetPath() const { return Path; }
private:
    File Root;
    String Path;
};

class FileSys {
public:
    static List<String> TokenizePath(const String&);
    static String CanonicalizePath(const String&, const String& = "");

    static Status Register(const FsImpl&);
    static Status CheckMountPoint(const String&);
    static Status CreateMountPoint(const String&, const File&);

    static Status Open(const String&, UInt8, File&);
    static Status Mount(const String&, const String&, UInt8);
    static Status Unmount(const String&);
private:
    static const FsImpl &GetFileSys(const String&);
    static const MountPoint &GetMountPoint(const String&, String&);

    static const FsImpl EmptyFs;
    static const MountPoint EmptyMp;
    static List<FsImpl> FileSystems;
    static List<MountPoint> MountPoints;
};

}
