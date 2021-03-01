/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 28 of 2021, at 14:02 BRT
 * Last edited on March 01 of 2021 at 12:29 BRT */

#include <fs.hxx>

using namespace CHicago;

const FsImpl FileSys::EmptyFs {};
const MountPoint FileSys::EmptyMp;
List<FsImpl> FileSys::FileSystems;
List<MountPoint> FileSys::MountPoints;

File::File() : Name(), Flags(0), Fs(), Priv(Null), References(Null), Length(0), INode(0) { }
File::File(File &&Source)
        : Name(Move(Source.Name)), Flags { Exchange(Source.Flags, 0) }, Fs { Exchange(Source.Fs, {}) },
          Priv { Exchange(Source.Priv, Null) }, References { Exchange(Source.References, Null) },
          Length { Exchange(Source.Length, 0) }, INode { Exchange(Source.INode, 0) } { }

File::File(const File &Source)
        : Name(Source.Name), Flags(Source.Flags), Fs(Source.Fs), Priv(Source.Priv), References(Source.References),
          Length(Source.Length), INode(Source.INode) {
    if (References != Null) {
        (*References)++;
    }
}

File::File(const String &Name, UInt8 Flags, const FsImpl &Fs, UInt64 Length, const Void *Priv, UInt64 INode)
    : Name(Name), Flags(Flags), Fs(Fs), Priv(Priv), References(new UIntPtr), Length(Length), INode(INode) {
    if (References != Null) {
        (*References)++;
    }
}

File::~File() {
    Close();
}

Void File::Close() {
    /* Remove a bit of redundancy. */

    if (References == Null || !--(*References)) {
        if (References != Null) {
            delete References;
        }

        if (Fs.Close != Null) {
            Fs.Close(Priv, INode);
        }
    }

    Name = {};
    Flags = 0;
    Priv = References = Null;
    Length = INode = 0;
    SetMemory(&Fs, 0, sizeof(FsImpl));
}

File &File::operator =(File &&Source) {
    if (this != &Source) {
        Name = Move(Source.Name);
        Flags = Exchange(Source.Flags, 0);
        Fs = Exchange(Source.Fs, {});
        Priv = Exchange(Source.Priv, Null);
        References = Exchange(Source.References, Null);
        Length = Exchange(Source.Length, 0);
        INode = Exchange(Source.INode, 0);
    }

    return *this;
}

File &File::operator =(const File &Source) {
    if (this != &Source) {
        Close();

        Name = Source.Name;
        Flags = Source.Flags;
        Fs = Source.Fs;
        Length = Source.Length;
        Priv = Source.Priv;
        INode = Source.INode;
        References = Source.References;

        if (References != Null) {
            (*References)++;
        }
    }

    return *this;
}

/* The File class stores all the private data that we need, we could have a function that return the FsImpl, and make
 * some kind of wrapper in the FileSys class (or even just let the user call the functions manually), but it's better to
 * implement said wrappers on the File class itself. */

Status File::Read(UInt64 Offset, UInt64 Length, Void *Buffer, UInt64 &Count) const {
    if (Buffer == Null || !Length) {
        return Status::InvalidArg;
    }

    return Count = 0, ((Flags & OPEN_DIR) || !(Flags & OPEN_READ) || Fs.Read == Null)
                      ? Status::Unsupported : Fs.Read(Priv, INode, Offset, Length, Buffer, &Count);
}

Status File::Write(UInt64 Offset, UInt64 Length, const Void *Buffer, UInt64 &Count) const {
    if (Buffer == Null || !Length) {
        return Status::InvalidArg;
    }

    return Count = 0, ((Flags & OPEN_DIR) || !(Flags & OPEN_WRITE) || Fs.Write == Null)
                      ? Status::Unsupported : Fs.Write(Priv, INode, Offset, Length, Buffer, &Count);
}

Status File::ReadDirectory(UIntPtr Index, String &Name) const {
    Char *out = Null;
    Status status = (!(Flags & OPEN_DIR) || !(Flags & OPEN_READ) || Fs.ReadDirectory == Null)
                    ? Status::Unsupported : Fs.ReadDirectory(Priv, INode, Index, &out);

    if (status != Status::Success) {
        return status;
    }

    return Name = String(out, True), delete out, !Name.GetLength() ? Status::OutOfMemory : Status::Success;
}

Status File::Search(const String &Name, UInt8 Flags, File &Out) const {
    /* ->Search returns the Priv and INode values for the file, we should mount the wrapper around the returned values,
     * remembering that the Fs remains the same. */

    Void *priv;
    UInt64 inode, len;

    Status status = (!(this->Flags & OPEN_DIR) || !(this->Flags & OPEN_READ) || Fs.Search == Null)
                    ? Status::Unsupported : Fs.Search(Priv, INode, Name.GetValue(), &priv, &inode, &len);

    if (status != Status::Success) {
        return status;
    } else if (Fs.Open != Null && (status = Fs.Open(priv, inode, Flags)) != Status::Success) {
        if (Fs.Close != Null) {
            Fs.Close(priv, inode);
        }

        return status;
    }

    return Out = File(Name, Flags, Fs, len, priv, inode), Status::Success;
}

Status File::Create(const String &Name, UInt8 Flags) const {
    return (!(this->Flags & OPEN_DIR) || !(this->Flags & OPEN_WRITE) || Fs.Create == Null)
           ? Status::Unsupported : Fs.Create(Priv, INode, Name.GetValue(), Flags);
}

Status File::Control(UIntPtr Function, const Void *InBuffer, Void *OutBuffer) const {
    return ((Flags & OPEN_DIR) || !(Flags & OPEN_READ) || Fs.Control == Null)
           ? Status::Unsupported : Fs.Control(Priv, INode, Function, InBuffer, OutBuffer);
}

Status File::Unmount() const {
    /* Only the FileSys::Unmount function should call us, we don't have any way to be sure that we aren't being called
     * by something else, but let's at least try to check the name, and if this is a directory. */

    return (!Name.Compare("/") || !(Flags & OPEN_DIR) || Fs.Unmount == Null) ? Status::Unsupported
                                                                             : Fs.Unmount(Priv, INode);
}

MountPoint &MountPoint::operator =(MountPoint &&Source) {
    if (this != &Source) {
        Root = Move(Source.Root);
        Path = Move(Source.Path);
    }

    return *this;
}

MountPoint &MountPoint::operator =(const MountPoint &Source) {
    if (this != &Source) {
        Root = Source.Root;
        Path = Source.Path;
    }

    return *this;
}

List<String> FileSys::TokenizePath(const String &Path) {
    /* The String class already have a function that tokenizes the string, and return a List<> of tokens, but we have
     * some tokens that we need to remove... */

    List<String> ret = Path.Tokenize("/");

    for (UIntPtr i = 0; i < ret.GetLength();) {
        if (ret[i].Compare(".")) {
            /* '.' means the current directory, we can just remove the token without doing anything else. */
            ret.Remove(i);
        } else if (ret[i].Compare("..")) {
            /* '..' means the parent directory, we need to remove ourselves and the token before us (or,
             * in case there is no entry before us, only remove ourselves. */

            if (i > 0) {
                ret.Remove(--i);
            }

            ret.Remove(i);
        } else {
            i++;
        }
    }

    return ret;
}

String FileSys::CanonicalizePath(const String &Path, const String &Increment) {
    /* This time, we need to tokenize TWO strings, append the contents of the second list to the first, and do the same
     * process we did in the TokenizePath function. */

    List<String> lpath = Path.Tokenize("/"), lincr = Increment.Tokenize("/");

    if (lincr.GetLength() && lpath.Add(lincr) != Status::Success) {
        return {};
    }

    /* Now, we can do the same process to remove the '.' and the '..'s. */

    for (UIntPtr i = 0; i < lpath.GetLength();) {
        if (lpath[i].Compare(".")) {
            /* '.' means the current directory, we can just remove the token without doing anything else. */
            lpath.Remove(i);
        } else if (lpath[i].Compare("..")) {
            /* '..' means the parent directory, we need to remove ourselves and the token before us (or,
             * in case there is no entry before us, only remove ourselves. */

            if (i > 0) {
                lpath.Remove(--i);
            }

            lpath.Remove(i);
        } else {
            i++;
        }
    }

    /* Finally, let's try to alloc the return string, let's just hope that the allocations are not going to fail here,
     * at the very end. */

    if (!lpath.GetLength()) {
        return "/";
    }

    String ret;

    for (const String &part : lpath) {
        ret.Append("/{}", part);
    }

    return ret;
}

Status FileSys::Register(const FsImpl &Info) {
    /* Almost all of the Impl fields can be Null, except for the Name field, as we use it to make sure there
     * is no duplicate filesystem registered. */

    if (Info.Name == Null) {
        return Status::InvalidArg;
    } else if (&GetFileSys(Info.Name) != &EmptyFs) {
        return Status::AlreadyExists;
    }

    return FileSystems.Add(Info);
}

static String FixView(const String &Path) {
    String path(Path.GetValue());
    for (; path[Path.GetViewEnd() - Path.GetViewStart()] == '/';
           path.SetView(Path.GetViewStart(), Path.GetViewEnd() - 1)) ;
    return path;
}

Status FileSys::CheckMountPoint(const String &Path) {
    /* We could probably only return a Boolean, BUT, we need to check if the Path starts with a slash, and in
     * the case have trailing slashes, we need to allocate memory, both return different status codes, so just
     * a Boolean isn't enough. */

    if (Path[0] != '/') {
        return Status::InvalidArg;
    } else if (!MountPoints.GetLength()) {
        return Status::NotMounted;
    }

    String path = FixView(Path);

    for (const MountPoint &mp : MountPoints) {
        if (!(mp.GetPath().Compare(path))) {
            continue;
        }

        return Status::AlreadyMounted;
    }

    return Status::NotMounted;
}

Status FileSys::CreateMountPoint(const String &Path, const File &Root) {
    /* We need to export this to be visible so the user can mount the boot directory, the root directory, the
     * /Devices folder etc. */

    if (Path[0] != '/' || (Root.GetFlags() & (OPEN_READ | OPEN_DIR)) != (OPEN_READ | OPEN_DIR)) {
        return Status::InvalidArg;
    }

    String path = FixView(Path);

    if (MountPoints.GetLength()) {
        if (CheckMountPoint(path) != Status::NotMounted) {
            return Status::AlreadyMounted;
        }
    }

    return MountPoints.Add(MountPoint(path, Root));
}

static Status CheckFlags(UInt8 SourceFlags, UInt8 Flags) {
    /* We can't just implement one big comparison that returns a boolean, as each case have one specific error
     * code. */

    if ((Flags & OPEN_DIR) && !(SourceFlags & OPEN_DIR)) {
        return Status::NotDirectory;
    } else if (!(Flags & OPEN_DIR) && (SourceFlags & OPEN_DIR)) {
        return Status::NotFile;
    } else if ((Flags & OPEN_READ) && !(SourceFlags & OPEN_READ)) {
        return Status::NotRead;
    } else if (((Flags & OPEN_WRITE) && !(SourceFlags & OPEN_WRITE))) {
        return Status::NotWrite;
    }

    return ((Flags & OPEN_EXEC) && !(SourceFlags & OPEN_EXEC)) ? Status::NotExec : Status::Success;
}

Status FileSys::Open(const String &Path, UInt8 Flags, File &Out) {
    if (Path[0] != '/') {
        return Status::InvalidArg;
    } else if (MountPoints.GetLength() == 0) {
        return Status::DoesntExist;
    } else if ((Flags & OPEN_RECUR_CREATE) && !(Flags & OPEN_CREATE)) {
        Flags |= OPEN_CREATE;
    }

    /* The 'Flags' variable aren't on the format that the File class expects. It contains info about if we should create
     * the file in case it doesn't exists, if we can create all the folders in a recur way etc. We can extract the valid
     * File flags by using a mask (which zeroes out all the invalid flags). We need to get the mount point of the path,
     * the GetMountPoint function returns both the mount point and the remainder of the path (that is, the original
     * path, but with the mount point path removed), we can tokenize the remainder, and try to open/create
     * everything. */

    Status status;
    String remain;
    UInt8 ffile = Flags & FILE_FLAGS_MASK, fdir = ffile | OPEN_DIR;
    const MountPoint &mp = GetMountPoint(Path, remain);

    if (Flags & OPEN_CREATE) {
        fdir |= OPEN_WRITE;
    }

    if (&mp == &EmptyMp) {
        return Status::NotMounted;
    } else if ((status = CheckFlags(mp.GetRoot().GetFlags(), !remain.GetLength() ? ffile : fdir)) != Status::Success) {
        return status;
    } else if (!remain.GetLength()) {
        return Out = mp.GetRoot(), Status::Success;
    }

    List<String> parts = TokenizePath(remain);

    if (!parts.GetLength()) {
        return Status::OutOfMemory;
    }

    File dir = mp.GetRoot();

    while (parts.GetLength() != 1) {
        File cur;
        String name = Move(parts[0]);

        parts.Remove(0);

        if ((status = dir.Search(name, fdir, cur)) != Status::Success) {
            if (status != Status::DoesntExist || !(Flags & OPEN_RECUR_CREATE) ||
                (status = dir.Create(name, fdir)) != Status::Success) {
                return status;
            }
        }

        dir = Move(cur);
    }

    /* Now only the creation of the file/directory itself is left, we can remove the last list entry, delete the
     * list, and try to search for the file. If we do find it, we need to make sure that the user didn't said that we
     * should only try to create the file, and if we don't find it and the create flag is set, we need to try creating
     * it. */

    String name = Move(parts[0]);

    parts.Remove(0);

    if ((status = dir.Search(name, ffile, Out)) != Status::Success) {
        if (status != Status::DoesntExist || !(Flags & OPEN_CREATE) ||
            (status = dir.Create(name, ffile)) != Status::Success) {
            return status;
        }

        if ((status = dir.Search(name, ffile, Out)) != Status::Success) {
            return status;
        }
    } else if (Flags & OPEN_ONLY_CREATE) {
        return Status::AlreadyExists;
    }

    return Status::Success;
}

Status FileSys::Mount(const String &Dest, const String &Source, UInt8 Flags) {
    /* We removed the option to manually specify the file system type, this makes the code a bit smaller, and we have
     * less things to do, we may add it again later, but for now just do the same checks that we did in the old code
     * (dest doesn't exist, source exists and we can do whatever the Flags say, etc). */

    Flags = (Flags & FILE_FLAGS_MASK) & ~OPEN_DIR;

    File dst, src;
    Status status;
    UInt64 inode;
    Void *priv;

    if (Open(Dest, OPEN_READ, dst) == Status::Success || Open(Dest, OPEN_DIR | OPEN_READ, dst) == Status::Success) {
        return Status::AlreadyMounted;
    } else if ((status = Open(Source, Flags, src)) != Status::Success) {
        return status;
    }

    for (const FsImpl &fs : FileSystems) {
        if ((status = fs.Mount((Void*)&src, &priv, &inode)) == Status::Success) {
            if ((status = CreateMountPoint(Dest, File("/", OPEN_DIR | Flags, fs, 0, priv, inode))) != Status::Success) {
                fs.Unmount(priv, inode);
            }

            return status;
        } else if (status == Status::OutOfMemory) {
            /* I think that it is a good idea to fail in the case the FS driver returns that the kernel is out of
             * memory. */

            return status;
        }
    }

    return Status::InvalidFs;
}

Status FileSys::Unmount(const String &Path) {
    /* Unmounting is just a matter of finding the mount point struct that points to Path (Path has to be the EXACT
     * mount path, not some sub-folder or file inside the mount point). We need to do the same handling of trailing
     * slashes on the Path as we do on the CreateMountPoint function. */

    if (Path[0] != '/') {
        return Status::InvalidArg;
    } else if (!MountPoints.GetLength()) {
        return Status::NotMounted;
    }

    UIntPtr idx = 0;
    String path = FixView(Path);

    for (const MountPoint &mp : MountPoints) {
        if (!(mp.GetPath().Compare(path))) {
            idx++;
            continue;
        }

        mp.GetRoot().Unmount();
        MountPoints.Remove(idx);

        return Status::Success;
    }

    return Status::NotMounted;
}

const FsImpl &FileSys::GetFileSys(const String &Path) {
    /* As the String class contains a constructor around C-strings, they will be auto converted into CHicago strings,
     * which makes our job a lot easier. */

    if (FileSystems.GetLength()) {
        for (const FsImpl &fs : FileSystems) {
            if (Path.Compare(fs.Name)) {
                return fs;
            }
        }
    }

    return EmptyFs;
}

const MountPoint &FileSys::GetMountPoint(const String &Path, String &Remain) {
    /* We have two options to iterate through the list and try to get the right mount point: checking each mount point
     * that the path is equal to the start of our Path, and return the one with the bigger length, or, copying the
     * string, and doing something like what we do at CreateMountPoint, but remembering to save the length, and only do
     * the 'stop at next slash' after each iteration. We're going with the second option. */

    if (!MountPoints.GetLength()) {
        return EmptyMp;
    }

    String path(Path.GetValue());
    UIntPtr end = Path.GetViewEnd(), start = 0;
    for (; end > Path.GetViewStart() && path[end - Path.GetViewStart()] == '/'; end--) ;
    UIntPtr len = end - Path.GetViewStart();

    for (path.SetView(Path.GetViewStart(), Path.GetViewStart() + 1);
         path.GetViewEnd() <= end; path.SetView(Path.GetViewStart(), Path.GetViewEnd() + 1), start++, len--) {
        for (const MountPoint &mp : MountPoints) {
            if (mp.GetPath().Compare(path)) {
                for (UIntPtr i = 1; i < len; i++) {
                    if (Remain.Append(Path[start + i]) != Status::Success) {
                        Remain.Clear();
                        return EmptyMp;
                    }
                }

                return mp;
            }
        }
    }

    return EmptyMp;
}
