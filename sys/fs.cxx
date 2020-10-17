/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 06 of 2020, at 11:15 BRT
 * Last edited on October 09 of 2020, at 22:03 BRT */

#include <chicago/fs.hxx>
#include <chicago/textout.hxx>

List<const FsImpl*> FileSys::FileSystems;
List<const MountPoint*> FileSys::MountPoints;

/* We need to handle the reference count on all the constructors except for the Void one. We're not
 * going to implement a const File* constructor, instead, call the construtor passing *ptr. */

File::File(Void) : Name("<unknown>"), Flags(0), Fs(Null), Length(0), Priv(Null), INode(0),
				   References(Null) { }

File::File(const String &Name, UInt8 Flags, const FsImpl *Fs, UInt64 Length, const Void *Priv,
		   UInt64 INode) : Name((String&)Name), Flags(Flags), Fs(Fs), Length(Length), Priv(Priv),
		   INode(INode), References(new UIntPtr) {
	if (References != Null) {
		(*References)++;
	}
}

File::File(const File &Source) : Name(Source.Name), Flags(Source.Flags), Fs(Source.Fs),
								 Length(Source.Length), Priv(Source.Priv), INode(Source.INode),
								 References(Source.References) {
	if (References != Null) {
		(*References)++;
	}
}

File::~File(Void) {
	if (References == Null || --(*References) == 0) {
		if (References != Null) {
			delete References;
		}
		
		if (Fs != Null && Fs->Close != Null) {
			Fs->Close(Priv, INode);
		}
	}
}

File &File::operator =(const File &Source) {
	if (Source.Priv != Priv || Source.INode != INode || Source.References != References) {
		if (References == Null || --(*References) == 0) {
			if (References != Null) {
				delete References;
			}
			
			if (Fs != Null && Fs->Close == Null) {
				Fs->Close(Priv, INode);
			}
		}
		
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

/* The File class stores all the private data that we need, we could have a function that return the
 * FsImpl, and make some kind of wrapper in the FileSys class (or even just let the user call the
 * functions manually), but it's better to implement said wrappers on the File class itself. */

Status File::Read(UInt64 Offset, UInt64 Length, Void *Buffer, UInt64 &Count) const {
	if (Length == 0 || Buffer == Null) {
		return Status::InvalidArg;
	}
	
	Count = 0;
	
	return ((Flags & OPEN_DIR) || !(Flags & OPEN_READ) || Fs == Null || Fs->Read == Null)
		   ? Status::Unsupported : Fs->Read(Priv, INode, Offset, Length, Buffer, &Count);
}

Status File::Write(UInt64 Offset, UInt64 Length, const Void *Buffer, UInt64 &Count) const {
	if (Length == 0 || Buffer == Null) {
		return Status::InvalidArg;
	}
	
	Count = 0;
	
	return ((Flags & OPEN_DIR) || !(Flags & OPEN_WRITE) || Fs == Null || Fs->Write == Null)
		   ? Status::Unsupported : Fs->Write(Priv, INode, Offset, Length, Buffer, &Count);
}

Status File::ReadDirectory(UIntPtr Index, String &Name) const {
	Char *out = Null;
	Status status = (!(Flags & OPEN_DIR) || !(Flags & OPEN_READ) || Fs == Null ||
					 Fs->ReadDirectory == Null) ? Status::Unsupported :
												  Fs->ReadDirectory(Priv, INode, Index, &out);
	
	if (status != Status::Success) {
		return status;
	}
	
	Name = String(out, True);
	
	delete out;
	return Name.GetSize() == 0 ? Status::OutOfMemory : Status::Success;
}

Status File::Search(const String &Name, UInt8 Flags, File &Out) const {
	/* ->Search returns the Priv and INode values for the file, we should mount the wrapper around
	 * the returned values, remembering that the Fs remains the same. */
	
	Void *priv;
	UInt64 inode, len;
	
	Status status = (!(this->Flags & OPEN_DIR) || !(this->Flags & OPEN_READ) || Fs == Null ||
					 Fs->Search == Null) ? Status::Unsupported :
										   Fs->Search(Priv, INode, Name.ToCString(), &priv, &inode,
													  &len);
	
	if (status != Status::Success) {
		return status;
	} else if (Fs->Open != Null && (status = Fs->Open(priv, inode, Flags)) != Status::Success) {
		if (Fs->Close != Null) {
			Fs->Close(priv, inode);
		}
		
		return status;
	}
	
	Out = File(Name, Flags, Fs, len, priv, inode);
	
	return Status::Success;
}

Status File::Create(const String &Name, UInt8 Flags) const {
	return (!(this->Flags & OPEN_DIR) || !(this->Flags & OPEN_WRITE) || Fs == Null || Fs->Create == Null)
		   ? Status::Unsupported : Fs->Create(Priv, INode, Name.ToCString(), Flags);
}

Status File::Control(UIntPtr Function, const Void *InBuffer, Void *OutBuffer) const {
	return ((Flags & OPEN_DIR) || !(Flags & OPEN_READ) || Fs == Null || Fs->Control == Null)
		   ? Status::Unsupported : Fs->Control(Priv, INode, Function, InBuffer, OutBuffer);
}

Status File::Unmount(Void) const {
	/* Only the FileSys::Unmount function should call us, we don't have any way to be sure that we aren't
	 * being called by something else, but let's at least try to check the name, and if this is a
	 * directory. */
	
	return (!Name.Compare("/") || !(Flags & OPEN_DIR) || Fs == Null || Fs->Unmount == Null)
		   ? Status::Unsupported : Fs->Unmount(Priv, INode);
}

/* The Mount/Unmount function should never be called by anyone except the FileSys ::Mount/::Unmount
 * functions, so we don't need to implement the wrappers around them (they are not even specific to
 * one file). */

List<String> FileSys::TokenizePath(const String &Path) {
	/* The String class already have a function that tokenizes the string, and return a List<> of tokens,
	 * but we have some tokens that we need to remove... */
	
	List<String> ret = Path.Tokenize("/");
	
	for (UIntPtr i = 0; i < ret.GetLength();) {
		const String &val = ret[i];
		
		if (val.Compare(".")) {
			/* '.' means the current directory, we can just remove the token without doing anything else. */
			ret.Remove(i);
		} else if (val.Compare("..")) {
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
	/* This time, we need to tokenize TWO strings, append the contents of the second list to the first, and
	 * do the same process we did in the TokenizePath function. */
	
	List<String> lpath = Path.Tokenize("/");
	List<String> lincr = Increment.Tokenize("/");
	
	if (lincr.GetLength() != 0 && lpath.Add(lincr) != Status::Success) {
		return String();
	}
	
	/* Now, we can do the same process to remove the '.' and the '..'s. */
	
	for (UIntPtr i = 0; i < lpath.GetLength();) {
		const String &val = lpath[i];
		
		if (val.Compare(".")) {
			/* '.' means the current directory, we can just remove the token without doing anything else. */
			lpath.Remove(i);
		} else if (val.Compare("..")) {
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
	
	/* Finally, let's try to alloc the return string, let's just hope that the allocations are not going to
	 * fail here, at the very end. */
	
	if (lpath.GetLength() == 0) {
		return String("/");
	}
	
	String ret;
	
	for (const String &part : lpath) {
		if (ret.Append("/%S", &part) != Status::Success) {
			return String();
		}
	}
	
	return ret;
}

Status FileSys::Register(const FsImpl *Info) {
	/* Almost all of the Impl fields can be Null, except for the Name field, as we use it to make sure there
	 * is no duplicate filesystem registered. */
	
	if (Info == Null || Info->Name == Null) {
		return Status::InvalidArg;
	} else if (GetFileSys(Info->Name) != Null) {
		return Status::AlreadyExists;
	}
	
#ifdef DBG
	Status status = FileSystems.Add(Info);
	
	if (status == Status::Success) {
		Debug->Write("[FileSys] Registered '%s' as a valid ::Mount filesystem\n", Info->Name);
	}
	
	return status;
#else
	return FileSystems.Add(Info);
#endif
}

Status FileSys::CheckMountPoint(const String &Path) {
	/* We could probably only return a Boolean, BUT, we need to check if the Path starts with a slash, and in
	 * the case have trailing slashes, we need to allocate memory, both return different status codes, so just
	 * a Boolean isn't enough. */
	
	if (Path[0] != '/') {
		return Status::InvalidArg;
	} else if (MountPoints.GetLength() == 0) {
		return Status::NotMounted;
	}
	
	Char *path = (Path[Path.GetLength() - 1] == '/' && Path.GetLength() != 1)
				  ? Path.ToMutCString() : (Char*)Path.ToCString();
	
	if (path == Null) {
		return Status::OutOfMemory;
	} else if (Path.GetLength() != 1 && Path[Path.GetLength() - 1] == '/') {
		for (Char *cur = path + (Path.GetLength() - 1); *cur == '/' && cur != path; cur--) {
			*cur = 0;
		}
	}
	
	for (const MountPoint *mp : MountPoints) {
		if (!(mp->GetPath().Compare(path))) {
			continue;
		}
		
		return Status::AlreadyMounted;
	}
	
	return Status::NotMounted;
}

Status FileSys::CreateMountPoint(const String &Path, const File &Root) {
	/* We need to export this to be visisble so the user can mount the boot directory, the root directory, the
	 * /Devices folder etc. */
	
	if (Path[0] != '/' || !(Root.GetFlags() & OPEN_DIR)) {
		return Status::InvalidArg;
	}
	
	Char *path = (Path[Path.GetLength() - 1] == '/' && Path.GetLength() != 1)
				  ? Path.ToMutCString() : (Char*)Path.ToCString();
	
	if (path == Null) {
		return Status::OutOfMemory;
	} else if (Path.GetLength() != 1 && Path[Path.GetLength() - 1] == '/') {
		/* The mount point should never be created with trailing slashes, as the GetMountPoint always removes
		 * them (which we're going to do as well). */
		
		for (Char *cur = path + (Path.GetLength() - 1); *cur == '/' && cur != path; cur--) {
			*cur = 0;
		}
	}
	
	if (MountPoints.GetLength() != 0) {
		UIntPtr idx = 0;
		
		for (const MountPoint *mp : MountPoints) {
			if (mp->GetPath().Compare(Path)) {
				MountPoints.Remove(idx);
				delete mp;
				goto c;
			} else {
				idx++;
			}
		}
	}
	
c:	MountPoint *mp = new MountPoint(Path, Root);
	Status status;
	
	if (mp == Null) {
		return Status::OutOfMemory;
	} else if ((status = MountPoints.Add(mp)) != Status::Success) {
		delete mp;
		return status;
	}
	
#ifdef DBG
	Debug->Write("[FileSys] Created a mount point at the path '%S'\n", &Path);
#endif
	
	return Status::Success;
}

static Status CheckFlags(UInt8 SourceFlags, UInt8 Flags) {
	/* We can't just implement one big comparison that returns a boolean, as each case have one specific error
	 * code. */
	
	if ((Flags & OPEN_DIR) && !(SourceFlags & OPEN_DIR)) {
		return Status::NotDirectory;
	} else if (!(Flags & OPEN_DIR) && (SourceFlags & OPEN_DIR)) {
		return Status::NotFile;
	} else if ((Flags & OPEN_READ) && !(SourceFlags & OPEN_READ)) {
		return Status::CantRead;
	} else if (((Flags & OPEN_WRITE) && !(SourceFlags & OPEN_WRITE))) {
		return Status::CantWrite;
	}
	
	return ((Flags & OPEN_EXEC) && !(SourceFlags & OPEN_EXEC)) ? Status::CantExec : Status::Success;
}

Status FileSys::Open(const String &Path, UInt8 Flags, File &Out) {
	if (Path[0] != '/') {
		return Status::InvalidArg;
	} else if (MountPoints.GetLength() == 0) {
		return Status::DoesntExist;
	} else if ((Flags & OPEN_RECUR_CREATE) && !(Flags & OPEN_CREATE)) {
		Flags |= OPEN_CREATE;
	}
	
	/* The 'Flags' variable aren't on the format that the File class expects. It contains info about if we should
	 * create the file in case it doesn't exists, if we can create all the folders in a recur way etc. We can
	 * extract the valid File flags by using a mask (which zeroes out all the invalid flags).
	 * We need to get the mount point of the path, the GetMountPoint function returns both the mount point and the
	 * remainder of the path (that is, the original path, but with the mount point path removed), we can tokenize
	 * the remainder, and try to open/create everything. */
	
	Status status;
	String remain;
	UInt8 ffile = Flags & FILE_FLAGS_MASK;
	UInt8 fdir = ffile | OPEN_DIR;
	const MountPoint *mp = GetMountPoint(Path, remain);
	
	if (Flags & OPEN_CREATE) {
		fdir |= OPEN_WRITE;
	}
	
	if (mp == Null) {
		return Status::NotMounted;
	} else if ((status = CheckFlags(mp->GetRoot().GetFlags(),
									remain.GetLength() == 0 ? ffile : fdir)) != Status::Success) {
		return status;
	} else if (remain.GetLength() == 0) {
		Out = mp->GetRoot();
		return Status::Success;
	}
	
	List<String> parts = TokenizePath(remain);
	
	if (parts.GetLength() == 0) {
		return Status::OutOfMemory;
	}
	
	File dir = mp->GetRoot();
	
	while (parts.GetLength() != 1) {
		File cur;
		String name = parts[0];
		
		parts.Remove(0);
		
		if ((status = dir.Search(name, fdir, cur)) != Status::Success) {
			if (status != Status::DoesntExist || !(Flags & OPEN_RECUR_CREATE) ||
				(status = dir.Create(name, fdir)) != Status::Success) {
				return status;
			}
		}
		
		dir = cur;
	}
	
	/* Now only the creation of the file/directory itself is left, we can remove the last list entry, delete the
	 * list, and try to search for the file. If we do find it, we need to make sure that the user didn't said that
	 * we should only try to create the file, and if we don't find it and the create flag is set, we need to try
	 * creating it. */
	
	String name = parts[0];
	
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
	 * (dest doesn't exist, source exists and we can do whatever the Flags say, etc).
	 * The Mount function should clone the file that we pass (by casting it to File* and calling the copy constructor,
	 * when the filesys is implemented on the kernel itself, or by calling the CloneFile function, when the filesys is
	 * a driver. */
	
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
	
	File *srcp = new File(src);
	
	if (srcp == Null) {
		return Status::OutOfMemory;
	}
	
	for (const FsImpl *fs : FileSystems) {
		if ((status = fs->Mount((Void*)srcp, &priv, &inode)) == Status::Success) {
			if ((status = CreateMountPoint(Dest, File("/", OPEN_DIR | Flags, fs, 0, priv, inode))) != Status::Success) {
				fs->Unmount(priv, inode);
			}
			
			delete srcp;
			return status;
		} else if (status == Status::OutOfMemory) {
			/* I think that it is a good idea to fail in the case the FS driver returns that the kernel is out of
			 * memory. */
			delete srcp;
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
	} else if (MountPoints.GetLength() == 0) {
		return Status::NotMounted;
	}
	
	Char *path = Path[Path.GetLength() - 1] == '/' ? Path.ToMutCString() : (Char*)Path.ToCString();
	UIntPtr idx = 0;
	
	if (path == Null) {
		return Status::OutOfMemory;
	} else if (Path[Path.GetLength() - 1] == '/') {
		for (Char *cur = path + (Path.GetLength() - 1); *cur == '/' && cur != path; cur--) {
			*cur = 0;
		}
	}
	
	for (const MountPoint *mp : MountPoints) {
		if (!(mp->GetPath().Compare(path))) {
			idx++;
			continue;
		}
		
		MountPoints.Remove(idx);
		mp->GetRoot().Unmount();
		
		delete mp;
		return Status::Success;
	}
	
	return Status::NotMounted;
}

const FsImpl *FileSys::GetFileSys(const String &Path) {
	/* As the String class contains a constructor around C-strings, they will be auto converted into CHicago
	 * strings, which makes our job a lot easier. */
	
	if (FileSystems.GetLength() == 0) {
		return Null;
	}
	
	for (const FsImpl *fs : FileSystems) {
		if (Path.Compare(fs->Name)) {
			return fs;
		}
	}
	
	return Null;
}

const MountPoint *FileSys::GetMountPoint(const String &Path, String &Remain) {
	/* We have two options to iterate through the list and try to get the right mount point: checking each
	 * mount point that the path is equal to the start of our Path, and return the one with the bigger length,
	 * or, copying the string, and doing something like what we do at CreateMountPoint, but remembering to
	 * save the length, and only do the 'stop at next slash' after each iteration. We're going with the second
	 * option. */
	
	if (MountPoints.GetLength() == 0) {
		return Null;
	}
	
	Char *path = Path.ToMutCString();
	UIntPtr start = Path.GetLength() - 1;
	UIntPtr len = 0;
	
	if (path == Null) {
		return Null;
	}
	
	while (start != 0 && path[start] == '/') {
		path[start--] = 0;
	}
	
	for (Char *cur = path + start; cur >= path; (*cur--) = 0, start--, len++) {
		for (const MountPoint *mp : MountPoints) {
			if (mp->GetPath().Compare(path)) {
				for (UIntPtr i = 1; i <= len; i++) {
					if (Remain.Append(Path[start + i]) != Status::Success) {
						Remain.Clear();
						return Null;
					}
				}
				
				return mp;
			}
		}
	}
	
	return Null;
}
