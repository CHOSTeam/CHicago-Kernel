/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 16 of 2020, at 09:24 BRT
 * Last edited on November 30 of 2020, at 15:01 BRT */

#include <siafs.hxx>
#include <textout.hxx>

const FsImpl MemFs::Impl = {
	Null,
	&MemFs::Close, &MemFs::Read, &MemFs::Write,
	Null, Null, Null, Null, Null, Null,
	"MemFs"
};

const FsImpl SiaFs::Impl = {
	&SiaFs::Open, &SiaFs::Close,
	&SiaFs::Read, &SiaFs::Write,
	&SiaFs::ReadDirectory, &SiaFs::Search,
	&SiaFs::Create,
	Null,
	&SiaFs::Mount, &SiaFs::Unmount,
	"SiaFs"
};

MemFs::MemFs(Void *Location, UInt64 Length) : Location(Location), Length(Length) { }

Status MemFs::MakeFile(Void *Location, UInt64 Length, UInt8 Flags, File *&Out) {
	/* MemFs files are always going to be files, never directories, as said in the .hxx file, MemFs is mostly
	 * used for initrd and things like that. You should ALWAYS call this function to mount MemFs files. */
	
	if (Flags & OPEN_DIR) {
		return Status::NotDirectory;
	}
	
	MemFs *priv = new MemFs(Location, Length);
	
	if (priv == Null) {
		return Status::OutOfMemory;
	}
	
	Out = new File("<memory>", Flags & FILE_FLAGS_MASK, &MemFs::Impl, Length, priv, 0);
	
	if (Out == Null) {
		delete priv;
		return Status::OutOfMemory;
	}
	
	return Status::Success;
}

Void MemFs::Close(const Void *Priv, UInt64) {
	/* MakeFile allocates a new instance of the MemFs class, if we don't implement the ::Close function we can't
	 * free it. 'Priv' contains the MemFs pointer casted into a void pointer, we still need to check it (just to
	 * make sure that the kernel/user didn't went crazy).
	 * I only discovered today (July 17 2020) that I can omit the variable name here to stop the unused variable
	 * warnings. */
	
	if (Priv != Null) {
		delete (const MemFs*)Priv;
	}
}

Status MemFs::Read(const Void *Priv, UInt64, UInt64 Offset, UInt64 Length, Void *Buffer, UInt64 *Count) {
	/* Only the Close, Read and Write functions are implemented, to make sure that there is no way to Mount or
	* Open MemFs, as the, only way to do it SHOULD BE using the static MakeFile function. */
	
	const MemFs *fs = (MemFs*)Priv;
	
	if (Priv == Null || Buffer == Null) {
		return Status::InvalidArg;
	} else if (Offset >= fs->Length || Offset + Length > fs->Length) {
		return Status::EOF;
	}
	
	StrCopyMemory(Buffer, (Void*)((UIntPtr)fs->Location + Offset), Length);
	
	if (Count != Null) {
		*Count = Length;
	}
	
	return Status::Success;
}

Status MemFs::Write(const Void *Priv, UInt64, UInt64 Offset, UInt64 Length, const Void *Buffer, UInt64 *Count) {
	MemFs *fs = (MemFs*)Priv;
	
	if (Priv == Null || Buffer == Null) {
		return Status::InvalidArg;
	} else if (Offset >= fs->Length || Offset + Length > fs->Length) {
		return Status::EOF;
	}
	
	StrCopyMemory((Void*)((UIntPtr)fs->Location + Offset), Buffer, Length);
	
	if (Count != Null) {
		*Count = Length;
	}
	
	return Status::Success;
}

SiaFs::SiaFs(const File *Source, const Header &Hdr, Void *ToFree, UIntPtr ExpandLoc) :
			 Source(Source), Hdr(Hdr), ToFree(ToFree), ExpandLoc(ExpandLoc) { }

SiaFs::~SiaFs(Void) {
	/* If we had to allocate the memory, we also need to free it, as the memory is not going to magically be freed
	 * by some mage. */
	
	if (ToFree) {
		delete (UInt8*)ToFree;
	}
	
	delete Source;
}

Status SiaFs::Register(Void) {
	return FileSys::Register(&SiaFs::Impl);
}

Status SiaFs::MountRamFs(const String &Path, Void *Location, UInt64 Length) {
	/* This function serves two purposes: creating an actual ramfs/tmpfs, when the Location is Null, and mounting
	 * the initrd, when the Location isn't Null. If the Location is Null, we need to manually alloc the specified
	 * Length, and remember to free it after we're finished. Before trying to create the File*, we need to check
	 * if the Path isn't already mounted. FileSys::Open would return NotMounted, but that only happens if the root
	 * directory isn't mounted, so we need to use the CheckMountPoint function. */
	
	UIntPtr expand = 0;
	Boolean free = False;
	
	if (Length < sizeof(SiaFs::Header) + sizeof(SiaFs::FileHeader)) {
		return Status::InvalidFs;
	} else if (Location == Null) {
		if ((Location = new UInt8[Length]) == Null) {
			return Status::OutOfMemory;
		}
		
		/* TODO: We still don't have any way to generate random number to fill in the UUID, so we're just going
		 * to fill it with some default value. We just need to create the SIA header (and clear it), and create the
		 * root directory header (and clear it), as the root directory should ALWAYS exist. */
		
		SiaFs::Header hdr { SIA_MAGIC, 0, { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
											0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF }, 0, 0, 0, 0, 0, sizeof(hdr) };
		SiaFs::FileHeader root { 0, SIA_FLAGS_DIRECTORY | SIA_FLAGS_READ | SIA_FLAGS_WRITE | SIA_FLAGS_EXEC, 0, 0,
								 "/" };
		
		StrCopyMemory(Location, &hdr, sizeof(hdr));
		StrCopyMemory((Void*)((UIntPtr)Location + sizeof(hdr)), &root, sizeof(root));
		
		free = True;
		expand = sizeof(hdr) + sizeof(root);
	}
	
#ifndef NDEBUG
	Debug->Write("[SiaFs] Mounting the ramfs located at 0x%x (length 0x%x) to the path '%S'\n", Location,
				 (UIntPtr)Length, &Path);
#endif
	
	Status status = FileSys::CheckMountPoint(Path);
	
	if (status != Status::NotMounted) {
#ifndef NDEBUG
		Debug->Write("        A mount point already exists at the path, we're out of memory, or the path is invalid\n");
#endif
		
		if (free) {
			delete (UInt8*)Location;
		}
		
		return status;
	}
	
	/* I think that it is a good idea to check if the location contains a valid SIA header before allocating the File*
	 * as well. */
	
	SiaFs::Header hdr;
	StrCopyMemory(&hdr, Location, sizeof(hdr));
	
	if ((status = CheckHeader(hdr, Length)) != Status::Success) {
		if (free) {
			delete (UInt8*)Location;
		}
		
		return status;
	}
	
	/* ::MakeFile will return a pointer, we have to dereference it when calling our constructor, and we can delete it
	 * after that (as it is going to be copied, and the File class can handle making sure that the file will only be
	 * completly destructed after we are unmounted. */
	
	UInt8 flags = OPEN_RX | ((hdr.Info & SIA_INFO_FIXED) ? 0 : OPEN_WRITE);
	File *src;
	
	if ((status = MemFs::MakeFile(Location, Length, flags, src)) != Status::Success) {
#ifndef NDEBUG
		Debug->Write("        The system is probably out of memory (couldn't create the MemFs file node)\n");
#endif
		
		if (free) {
			delete (UInt8*)Location;
		}
		
		return status;
	}
	
	SiaFs *fs = new SiaFs(src, hdr, free ? Location : Null, expand);
	
	if (fs == Null) {
#ifndef NDEBUG
		Debug->Write("        The system is out of memory (couldn't alloc the FS data pointer)\n");
#endif
		
		if (free) {
			delete (UInt8*)Location;
		}
		
		delete src;
		return Status::OutOfMemory;
	}
	
	SiaFs::PrivData *priv = new SiaFs::PrivData { fs, hdr.RootOffset, { 0, 0, 0, 0, { 0 } } };
	
	if (priv == Null) {
#ifndef NDEBUG
		Debug->Write("        The system is out of memory (couldn't alloc the priv data pointer)\n");
#endif	
		delete fs;
		delete src;
		return Status::OutOfMemory;
	}
	
	StrCopyMemory(&priv->Hdr, (Void*)((UIntPtr)Location + hdr.RootOffset), sizeof(priv->Hdr));
	
	if ((status = FileSys::CreateMountPoint(Path, new File("/", flags | OPEN_DIR | OPEN_EXEC, &SiaFs::Impl, 0,
														   priv, 0))) != Status::Success) {
#ifndef NDEBUG
		Debug->Write("        The system is probably out of memory (couldn't create the mount point)\n");
#endif
		delete fs;
		delete priv;
		delete src;
		return status;
	}
	
	return Status::Success;
}

Status SiaFs::CheckHeader(const SiaFs::Header &Hdr, UInt64 Length) {
	/* We have four things to check: if the header magic is valid, if the free file headers are valid (check the start
	 * of the headers, and if there is at least space for the amount of specified headers), if the free data headers
	 * are valid (same as the free file headers), and if the root directory offset it valid.
	 * We don't need to check if the KERNEL flag is set (different from the x86 bootloader, for example), as this may
	 * not be the kernel image/initrd. */
	
	if (Hdr.Magic != SIA_MAGIC) {
#ifndef NDEBUG
		Debug->Write("        The header magic is invalid (expected 0x%x, got 0x%x).\n", SIA_MAGIC, Hdr.Magic);
#endif
		return Status::InvalidFs;
	} else if (Hdr.FreeFileCount != 0 &&
			   (Hdr.FreeFileOffset < sizeof(SiaFs::Header) ||
				Hdr.FreeFileOffset + Hdr.FreeFileCount * sizeof(SiaFs::FileHeader) >= Length)) {
#ifndef NDEBUG
		Debug->Write("        The free file count/offset is too small/too big (starts at 0x%x, while the min is 0x%x, ",
					 (UIntPtr)Hdr.FreeFileOffset, sizeof(SiaFs::Header));
		Debug->Write("ends at 0x%x, while the max is 0x%x)\n",
					 (UIntPtr)(Hdr.FreeFileOffset + Hdr.FreeFileCount * sizeof(SiaFs::FileHeader)), (UIntPtr)Length);
#endif
		return Status::InvalidFs;
	} else if (Hdr.FreeDataCount != 0 &&
			   (Hdr.FreeDataOffset < sizeof(SiaFs::Header) ||
				Hdr.FreeDataOffset + Hdr.FreeDataCount * sizeof(SiaFs::DataHeader) >= Length)) {
#ifndef NDEBUG
		Debug->Write("        The free data count/offset is too small/too big (starts at 0x%x, while the min is 0x%x, ",
					 (UIntPtr)Hdr.FreeDataOffset, sizeof(SiaFs::Header));
		Debug->Write("min end is 0x%x, while the max length is 0x%x)\n",
					 (UIntPtr)(Hdr.FreeDataOffset + Hdr.FreeDataCount * sizeof(SiaFs::DataHeader)), (UIntPtr)Length);
#endif
		return Status::InvalidFs;
	} else if (Hdr.RootOffset < sizeof(SiaFs::Header) || Hdr.RootOffset + sizeof(SiaFs::FileHeader) >= Length) {
#ifndef NDEBUG
		Debug->Write("        The root directory location is invalid (starts at 0x%x, while the min is 0x%x, ",
					 (UIntPtr)Hdr.RootOffset, sizeof(SiaFs::Header));
		Debug->Write("min end is 0x%x, while the max length is 0x%x)\n",
					 (UIntPtr)(Hdr.RootOffset + sizeof(SiaFs::FileHeader)), (UIntPtr)Length);
#endif
		return Status::InvalidFs;
	}
	
	return Status::Success;
}

Status SiaFs::CheckPrivData(const Void *Priv, const SiaFs::PrivData *&OutPriv) {
	/* This counts for all the functions below (and ourselves): Priv contains the PrivData pointer, you can read
	 * the pointer to get both the FS class instance (which contains the source file) and the file header. */
	
	if (Priv == Null) {
		return Status::InvalidArg;
	}
	
	OutPriv = (const SiaFs::PrivData*)Priv;
	
	return Status::Success;
}

Status SiaFs::CheckPrivData(const Void *Priv, SiaFs::PrivData *&OutPriv) {
	/* Non const version of the CheckPrivData, Read, Write and Create will call this version, as we need to change
	 * some info in the priv struct. */
	
	if (Priv == Null) {
		return Status::InvalidArg;
	}
	
	OutPriv = (SiaFs::PrivData*)Priv;
	
	return Status::Success;
}

static Status CheckFileFlags(UInt16 FileFlags, UInt8 OpenFlags) {
	/* We can't just implement one big comparison that returns a boolean, as each case have one specific error
	 * code. */
	
	if ((OpenFlags & OPEN_DIR) && !(FileFlags & SIA_FLAGS_DIRECTORY)) {
		return Status::NotDirectory;
	} else if (!(OpenFlags & OPEN_DIR) && (FileFlags & SIA_FLAGS_DIRECTORY)) {
		return Status::NotFile;
	} else if ((OpenFlags & OPEN_READ) && !(FileFlags & SIA_FLAGS_READ)) {
		return Status::CantRead;
	} else if (((OpenFlags & OPEN_WRITE) && !(FileFlags & SIA_FLAGS_WRITE))) {
		return Status::CantWrite;
	}
	
	return ((OpenFlags & OPEN_EXEC) && !(FileFlags & SIA_FLAGS_EXEC)) ? Status::CantExec : Status::Success;
}

static Status CheckOpenFlags(UInt8 OpenFlagsSource, UInt8 OpenFlagsFile) {
	if ((OpenFlagsFile & OPEN_READ) && !(OpenFlagsSource & OPEN_READ)) {
		return Status::CantRead;
	}
	
	return ((OpenFlagsFile & OPEN_WRITE) && !(OpenFlagsSource & OPEN_WRITE)) ? Status::CantWrite :
																			   Status::Success;
}

Status SiaFs::Open(const Void *Priv, UInt64, UInt8 Flags) {
	/* All of the functions have to call CheckPrivData as this one is doing. We should only check if the flags
	 * are valid, as you can't write to an read-only file. Checking the source file flags is also required, as
	 * while the file were rying to access may be R/W, the source file may be R-only. */
	
	Status status;
	const SiaFs::PrivData *priv = Null;
	
	if ((status = CheckPrivData(Priv, priv)) != Status::Success) {
		return status;
	} else if ((status = CheckFileFlags(priv->Hdr.Flags, Flags)) != Status::Success) {
		return status;
	}
	
	return CheckOpenFlags(priv->Fs->Source->GetFlags(), Flags);
}

Void SiaFs::Close(const Void *Priv, UInt64) {
	/* The PrivData struct exists so we don't need to ->Read the file header all the time, we only need to free
	 * it, as the FS struct is going to be freed later (on the unmount function). */
	
	const SiaFs::PrivData *priv = Null;
	
	if (CheckPrivData(Priv, priv) == Status::Success && !String("/").Compare(priv->Hdr.Name)) {
		delete priv;
	}
}

Status SiaFs::Read(const Void *Priv, UInt64, UInt64 Offset, UInt64 Length, Void *Buffer, UInt64 *Count) {
	if (Length == 0 || Buffer == Null || Count == Null) {
		return Status::InvalidArg;
	}
	
	Status status;
	SiaFs::PrivData *priv = Null;
	
	if ((status = CheckPrivData(Priv, priv)) != Status::Success) {
		return status;
	} else if ((status = CheckFileFlags(priv->Hdr.Flags, OPEN_READ)) != Status::Success ||
			   (status = CheckOpenFlags(priv->Fs->Source->GetFlags(), OPEN_READ)) != Status::Success) {
		/* The File::Read function is supposed to this for us, do we even need to check it here? Idk. */
		return status;
	} else if (Offset >= priv->Hdr.Size || Offset + Length > priv->Hdr.Size) {
		return Status::EOF;
	}
	
	SiaFs::DataHeader hdr;
	UInt64 cur = 0, skip = Offset % sizeof(hdr.Contents), read;
	
	*Count = 0;
	
	/* The GoToOffset function isn't static, but it's still private, only we (members of the SiaFs class) can call
	 * it. */
	
	if ((status = priv->Fs->GoToOffset(priv->Hdr, priv->Offset, Offset, cur)) != Status::Success) {
		return status;
	}
	
	while (Length) {
		if ((status = priv->Fs->Source->Read(cur, sizeof(hdr), &hdr, read)) != Status::Success) {
			return status;
		} else if (read != sizeof(hdr)) {
			return Status::ReadFailed;
		}
		
		/* The .Contents field contains a static amount of data, as of today (July 17 2020), that amount is 504
		 * bytes, which when we add the size of the .Next field gives us 512 bytes, the most common sector size
		 * for HDs. If the size of the .Contents field is smaller than the remaining size, we can just copy everything
		 * from it, else, we need to make sure that we will only copy the remaining amount. */
		
		UInt64 size = sizeof(hdr.Contents) - skip;
		
		if (size > Length) {
			size = Length;
		}
		
		StrCopyMemory((Void*)((UIntPtr)Buffer + *Count), &hdr.Contents[skip], size);
		
		skip = 0;
		*Count += size;
		Length -= size;
		cur = hdr.Next;
	}
	
	return Status::Success;
}

Status SiaFs::Write(const Void *Priv, UInt64, UInt64 Offset, UInt64 Length, const Void *Buffer, UInt64 *Count) {
	if (Length == 0 || Buffer == Null || Count == Null) {
		return Status::InvalidArg;
	}
	
	Status status;
	SiaFs::PrivData *priv = Null;
	
	if ((status = CheckPrivData(Priv, priv)) != Status::Success) {
		return status;
	} else if ((status = CheckFileFlags(priv->Hdr.Flags, OPEN_WRITE)) != Status::Success ||
			   (status = CheckOpenFlags(priv->Fs->Source->GetFlags(), OPEN_WRITE)) != Status::Success) {
		/* The File::Write function is supposed to this for us, do we even need to check it here? Idk. */
		return status;
	} else if (priv->Fs->Hdr.Info & SIA_INFO_FIXED) {
		/* Tho we do need to check if the fixed flag is set, and I don't think that the File::Write function can
		 * even do this for us lol. */
		return Status::CantWrite;
	}
	
	SiaFs::DataHeader hdr;
	UInt64 cur = 0, skip = Offset % sizeof(hdr.Contents), rw, ncur;
	
	*Count = 0;
	
	/* The GoToOffset function isn't static, but it's still private, only we (members of the SiaFs class) can call
	 * it. */
	
	if ((status = priv->Fs->GoToOffset(priv->Hdr, priv->Offset, Offset, cur, True)) != Status::Success) {
		return status;
	}
	
	while (Length) {
		if ((status = priv->Fs->Source->Read(cur, sizeof(hdr), &hdr, rw)) != Status::Success) {
			return status;
		} else if (rw != sizeof(hdr)) {
			return Status::ReadFailed;
		}
		
		/* If the size of the .Contents field is smaller than the remaining size, we can just copy everything from
		 * it, else, we need to make sure that we will only copy the remaining amount. */
		
		UInt64 size = sizeof(hdr.Contents) - skip;
		
		if (size > Length) {
			size = Length;
		}
		
		StrCopyMemory(&hdr.Contents[skip], (Void*)((UIntPtr)Buffer + *Count), size);
		
		if ((status = priv->Fs->Source->Write(cur, sizeof(hdr), &hdr, rw)) != Status::Success) {
			return status;
		} else if (rw != sizeof(hdr)) {
			return Status::WriteFailed;
		}
		
		skip = 0;
		*Count += size;
		Length -= size;
		
		if (hdr.Next == 0) {
			/* For allocating more data entries here, we can do the exact same that we do on the ::GoToOffset
			 * function, so yeah, this is only a CTRL+C CTRL+V of the same snippet of code from ::GoToOffset. */
			
			if ((status = priv->Fs->AllocDataEntry2(ncur)) != Status::Success) {
				return status;
			}
			
			hdr.Next = ncur;
			
			if ((status = priv->Fs->Source->Write(cur, sizeof(hdr), &hdr, rw)) != Status::Success) {
				return status;
			} else if (rw != sizeof(hdr)) {
				return Status::WriteFailed;
			}
		}
		
		cur = hdr.Next;
	}
	
	return Status::Success;
}

Status SiaFs::ReadDirectory(const Void *Priv, UInt64, UIntPtr Index, Char **Name) {
	if (Name == Null) {
		return Status::InvalidArg;
	}
	
	Status status;
	const SiaFs::PrivData *priv = Null;
	
	if ((status = CheckPrivData(Priv, priv)) != Status::Success) {
		return status;
	} else if ((status = CheckFileFlags(priv->Hdr.Flags, OPEN_DIR | OPEN_READ)) != Status::Success ||
			   (status = CheckOpenFlags(priv->Fs->Source->GetFlags(), OPEN_DIR | OPEN_READ)) != Status::Success) {
		return status;
	} else if (priv->Hdr.Offset == 0) {
		return Status::DoesntExist;
	}
	
	/* Just like the data entries are a linked list, the file entries are also a linked list, we just need to follow
	 * all the links. */
	
	SiaFs::FileHeader hdr;
	UInt64 cur = priv->Hdr.Offset, read, len = 0;
	
	if ((status = priv->Fs->Source->Read(cur, sizeof(hdr), &hdr, read)) != Status::Success) {
		return status;
	}
	
	while (Index) {
		Index--;
		
		if ((cur = hdr.Next) == 0) {
			return Status::DoesntExist;
		} else if ((status = priv->Fs->Source->Read(cur, sizeof(hdr), &hdr, read)) != Status::Success) {
			return status;
		}
	}
	
	for (; hdr.Name[len]; len++) ;
	
	*Name = new Char[len + 1];
	
	if (*Name != Null) {
		for (UIntPtr i = 0; i < len; i++) {
			(*Name)[i] = hdr.Name[i];
		}
	}
	
	return *Name == Null ? Status::OutOfMemory : Status::Success;
}

Status SiaFs::Search(const Void *Priv, UInt64, const Char *Name, Void **OutPriv, UInt64*, UInt64 *OutLen) {
	if (Name == Null || OutPriv == Null || OutLen == Null) {
		return Status::InvalidArg;
	}
	
	Status status;
	const SiaFs::PrivData *priv = Null;
	
	if ((status = CheckPrivData(Priv, priv)) != Status::Success) {
		return status;
	} else if ((status = CheckFileFlags(priv->Hdr.Flags, OPEN_DIR | OPEN_READ)) != Status::Success ||
			   (status = CheckOpenFlags(priv->Fs->Source->GetFlags(), OPEN_READ)) != Status::Success) {
		return status;
	} else if (priv->Hdr.Offset == 0) {
		return Status::DoesntExist;
	}
	
	/* We need to do the same thing that we do on the ::ReadDirectory function, but instead of limiting ourselves
	 * for the index, and returning the name, we should check the name and return a PrivData pointer and the length
	 * of the file (or 0 on directories). We can construct a CHicago string around the name, so we can just call
	 * ::Compare. */
	
	String name(Name);
	SiaFs::FileHeader hdr;
	UInt64 cur = priv->Hdr.Offset, read;
	
	if ((status = priv->Fs->Source->Read(cur, sizeof(hdr), &hdr, read)) != Status::Success) {
		return status;
	}
	
	while (True) {
		if (name.Compare(hdr.Name)) {
			break;
		} else if ((cur = hdr.Next) == 0) {
			return Status::DoesntExist;
		} else if ((status = priv->Fs->Source->Read(cur, sizeof(hdr), &hdr, read)) != Status::Success) {
			return status;
		}
	}
	
	/* As said above, we need to allocate the PrivData pointer, this time we don't need to allocate the SiaFs
	 * pointer as it already exists (priv->Fs). */
	
	*OutPriv = new SiaFs::PrivData { priv->Fs, cur, { 0, 0, 0, 0, { 0 } } };
	
	if (*OutPriv != Null) {
		StrCopyMemory(&((SiaFs::PrivData*)*OutPriv)->Hdr, &hdr, sizeof(hdr));
		*OutLen = (hdr.Flags & SIA_FLAGS_DIRECTORY) ? 0 : hdr.Size;
	}
	
	return *OutPriv == Null ? Status::OutOfMemory : Status::Success;
}

Status SiaFs::Create(const Void *Priv, UInt64, const Char *Name, UInt8 Flags) {
	if (Name == Null) {
		return Status::InvalidArg;
	}
	
	UInt64 entry;
	Status status;
	SiaFs::PrivData *priv = Null;
	
	/* This time we need both read and write permissions. The create function is pretty simple (thanks to the
	 * helper functions lol), we first allocate the file entry, and we try linking it into the last entry from
	 * the directory. The FileSys::Open function should be the one calling us, so we don't need to mount any
	 * pointer, or call Search. */
	
	if ((status = CheckPrivData(Priv, priv)) != Status::Success) {
		return status;
	} else if ((status = CheckFileFlags(priv->Hdr.Flags, OPEN_DIR | OPEN_RW)) != Status::Success ||
			   (status = CheckOpenFlags(priv->Fs->Source->GetFlags(), OPEN_DIR | OPEN_RW)) != Status::Success) {
		return status;
	} else if ((status = priv->Fs->AllocFileEntry2(Name, Flags, entry)) != Status::Success) {
		return status;
	}
	
	return priv->Fs->LinkFileEntry(priv->Hdr, priv->Offset, entry);
}

Status SiaFs::Mount(const Void *Source, Void **OutPriv, UInt64*) {
	/* We're directly integrated with the kernel, so we can cast the Source to its true form... const File*.
	 * What we're going to do it's pretty simillar to what we do on the MountRamFs function, but now we don't
	 * have one memory location that we can just access, we need to actually R/W from a file.
	 * Also, this time we don't have the destination path, we just need to check if the source file is valid,
	 * and try getting everything ready. */
	
	const File *src = (const File*)Source;
	
	if (Source == Null || OutPriv == Null) {
		return Status::InvalidArg;
	} else if (src->GetLength() < sizeof(SiaFs::Header) + sizeof(SiaFs::FileHeader)) {
		return Status::InvalidFs;
	}
	
#ifndef NDEBUG
	Debug->Write("[SiaFs] Mounting the SIA file at the path '%S'\n", &src->GetName());
#endif
	
	UInt64 read;
	SiaFs::Header hdr;
	SiaFs::FileHeader root;
	Status status = src->Read(0, sizeof(hdr), &hdr, read);
	
	if (status != Status::Success) {
		return status;
	} else if (read != sizeof(hdr)) {
		return Status::ReadFailed;
	} else if ((status = CheckHeader(hdr, src->GetLength())) != Status::Success) {
		return status;
	} else if ((status = src->Read(hdr.RootOffset, sizeof(root), &root, read)) != Status::Success) {
		return status;
	} else if (read != sizeof(root)) {
		return status;
	}
	
	SiaFs *fs = new SiaFs(src, hdr, Null, 0);
	
	if (fs == Null) {
#ifndef NDEBUG
		Debug->Write("        The system is out of memory (couldn't alloc the FS data pointer)\n");
#endif	
		return Status::OutOfMemory;
	}
	
	*OutPriv = new SiaFs::PrivData { fs, hdr.RootOffset, { 0, 0, 0, 0, { 0 } } };
	
	if (*OutPriv == Null) {
#ifndef NDEBUG
		Debug->Write("        The system is out of memory (couldn't alloc the priv data pointer)\n");
#endif	
		delete fs;
		return Status::OutOfMemory;
	}
	
	StrCopyMemory(&(((SiaFs::PrivData*)*OutPriv)->Hdr), &root, sizeof(root));
	
	return Status::Success;
}

Status SiaFs::Unmount(const Void *Priv, UInt64) {
	Status status;
	SiaFs::PrivData *priv = Null;
	
	/* Unmounting is just a matter of deleting the FS pointer and deleting the priv pointer (if the priv data
	 * had a custom destructor with some type of ref counting, we probably could only delete it). */
	
	if ((status = CheckPrivData(Priv, priv)) != Status::Success) {
		return status;
	}
	
	delete priv->Fs;
	delete priv;
	
	return Status::Success;
}

Status SiaFs::GoToOffset(SiaFs::FileHeader &FHdr, UInt64 FOff, UInt64 Offset, UInt64 &Out, Boolean Alloc) {
	/* This function is used by both ::Read and ::Write, it tries traversing the data linked list until we find
	 * the start of the data that we want, and, for the ::Write function, it auto-allocs what doesn't exist. */
	
	UInt64 cur = FHdr.Offset, ncur, rw;
	SiaFs::DataHeader hdr;
	Status status;
	
	if (!Alloc && cur == 0) {
		return Status::EOF;
	} else if (Alloc && cur == 0) {
		/* I'm guessing you just called ::Create, and this is the first time calling ::Write... Ok, let's alloc
		 * the first data sector of this file... */
		
		if ((status = AllocDataEntry2(cur)) != Status::Success) {
			return status;
		}
		
		FHdr.Offset = cur;
		
		if ((status = Source->Write(FOff, sizeof(FHdr), &FHdr, rw)) != Status::Success) {
			return status;
		} else if (rw != sizeof(FHdr)) {
			return Status::WriteFailed;
		}
	}
	
	while (Offset >= sizeof(hdr.Contents)) {
		if ((status = Source->Read(cur, sizeof(hdr), &hdr, rw)) != Status::Success) {
			return status;
		} else if (rw != sizeof(hdr)) {
			return Status::ReadFailed;
		}
		
		Offset -= sizeof(hdr.Contents);
		
		if (!Alloc && hdr.Next == 0) {
			return Status::EOF;
		} else if (Alloc && hdr.Next == 0) {
			/* This is the same process that we did above, but instead of writing the value to FHdr.Offset, we're
			 * going to write it to hdr.Next (and write hdr back instead of FHdr). */
			
			if ((status = AllocDataEntry2(ncur)) != Status::Success) {
				return status;
			}
			
			hdr.Next = ncur;
			
			if ((status = Source->Write(cur, sizeof(hdr), &hdr, rw)) != Status::Success) {
				return status;
			} else if (rw != sizeof(hdr)) {
				return Status::WriteFailed;
			}
		}
		
		cur = hdr.Next;
	}
	
	Out = cur;
	
	return Status::Success;
}

Status SiaFs::AllocDataEntry1(UInt64 Offset) {
	/* First version of the alloc data entry function, it doesn't actually allocate the offset, it just creates the
	 * data header, zeroes it, and writes it into the source file (at the specified offset). */
	
	if (Offset < sizeof(Hdr)) {
		return Status::InvalidArg;
	}
	
	SiaFs::DataHeader hdr { 0, { 0 } };
	Status status;
	UInt64 wrote;
	
	if ((status = Source->Write(Offset, sizeof(hdr), &hdr, wrote)) != Status::Success) {
		return status;
	}
	
	return wrote != sizeof(hdr) ? Status::WriteFailed : Status::Success;
}

Status SiaFs::AllocDataEntry2(UInt64 &Offset) {
	/* Second version of the alloc data entry function, this one actually tries to allocate the offset, first, we need
	 * to check if there is a free data entry, the SIA header contains the amount of free data entries, and the offset
	 * to the first free entry. */
	
	if (Hdr.FreeDataCount != 0) {
		SiaFs::DataHeader hdr;
		Status status;
		UInt64 rw;
		
		Offset = Hdr.FreeDataOffset;
		
		if ((status = Source->Read(Offset, sizeof(hdr), &hdr, rw)) != Status::Success) {
			return status;
		}
		
		/* The free data entries are always linked to the next entry, they are a linked list, so making sure that we're
		 * not going to alloc the same entry two times is just a matter of making sure to set the start of the list as
		 * the next  entry. */
		
		Hdr.FreeDataCount--;
		Hdr.FreeDataOffset = hdr.Next;
		
		if ((status = AllocDataEntry1(Offset)) != Status::Success) {
			return status;
		} else if ((status = Source->Write(0, sizeof(Hdr), &Hdr, rw)) != Status::Success) {
			return status;
		}
		
		return rw != sizeof(Hdr) ? Status::WriteFailed : Status::Success;
	}
	
	/* If the source file is R/W, just writing after the current length is going to auto-increase the file size, so
	 * let's try doing that to alloc more one entry... Except that if this is a ramfs we need to know the last location
	 * that we used, as Source->Location is going to be the size of the memory area that we have... */
	
	Status status = AllocDataEntry1(Offset = ExpandLoc ? ExpandLoc : Source->GetLength());
	
	if (status == Status::Success && ExpandLoc) {
		ExpandLoc += sizeof(SiaFs::FileHeader);
	}
	
	return status;
}

/* The two AllocFileEntry function are equivalent to the two AllocDataEntry functions, but they alloc file entries
 * instead of data entries. The file entries are also all linked to each other. */

static UInt16 ToSiaFlags(UInt8 Flags) {
	UInt16 ret = 0;
	
	/* As of July 17 2020 both the flags used on our FileSystem representation (OPEN_*) and the SIA flags (SIA_FLAGS_*)
	 * are the same, but to make sure that we're always going to send out valid flags into the files, let's manually parse
	 * the OPEN_* flags. */
	
	if (Flags & OPEN_DIR) {
		ret |= SIA_FLAGS_DIRECTORY;
	}
	
	if (Flags & OPEN_READ) {
		ret |= SIA_FLAGS_READ;
	}
	
	if (Flags & OPEN_WRITE) {
		ret |= SIA_FLAGS_WRITE;
	}
	
	if (Flags & OPEN_EXEC) {
		ret |= SIA_FLAGS_EXEC;
	}
	
	return ret;
}

Status SiaFs::AllocFileEntry1(const String &Name, UInt8 Flags, UInt64 Offset) {
	if (Offset < sizeof(Hdr)) {
		return Status::InvalidArg;
	}
	
	SiaFs::FileHeader hdr { 0, 0, 0, 0, { 0 } };
	Status status;
	UInt64 wrote;
	
	/* The main difference is that we also need to set two fields: The flags and the name. */
	
	for (UIntPtr i = 0; i < Name.GetLength(); i++) {
		hdr.Name[i] = (Char)Name[i];
	}
	
	hdr.Flags = ToSiaFlags(Flags);
	
	if ((status = Source->Write(Offset, sizeof(hdr), &hdr, wrote)) != Status::Success) {
		return status;
	}
	
	return wrote != sizeof(hdr) ? Status::WriteFailed : Status::Success;
}

Status SiaFs::AllocFileEntry2(const String &Name, UInt8 Flags, UInt64 &Offset) {
	if (Hdr.FreeFileCount != 0) {
		SiaFs::FileHeader hdr;
		Status status;
		UInt64 rw;
		
		Offset = Hdr.FreeFileOffset;
		
		if ((status = Source->Read(Offset, sizeof(hdr), &hdr, rw)) != Status::Success) {
			return status;
		}
		
		Hdr.FreeFileCount--;
		Hdr.FreeFileOffset = hdr.Next;
		
		if ((status = AllocFileEntry1(Name, Flags, Offset)) != Status::Success) {
			return status;
		} else if ((status = Source->Write(0, sizeof(Hdr), &Hdr, rw)) != Status::Success) {
			return status;
		}
		
		return rw != sizeof(Hdr) ? Status::WriteFailed : Status::Success;
	}
	
	Status status = AllocFileEntry1(Name, Flags, (Offset = ExpandLoc ? ExpandLoc : Source->GetLength()));
	
	if (status == Status::Success && ExpandLoc) {
		ExpandLoc += sizeof(SiaFs::FileHeader);
	}
	
	return status;
}

Status SiaFs::LinkFileEntry(SiaFs::FileHeader &FHdr, UInt64 FOff, UInt64 Offset) {
	/* First entry (FHdr.Offset == 0): Write the offset into the file header, write the file header back, and we're
	 * done. Other entries: Find the last entry, and write the offset into the .Next field of it (and write the file
	 * header of said entry back lol). */
	
	SiaFs::FileHeader hdr = FHdr;
	Status status;
	UInt64 rw;
	
	if (FHdr.Offset == 0) {
		FHdr.Offset = hdr.Offset = Offset;
		goto e;
	} else {
		FOff = FHdr.Offset;
		
		if ((status = Source->Read(FOff, sizeof(hdr), &hdr, rw)) != Status::Success) {
			return status;
		} else if (rw != sizeof(hdr)) {
			return Status::ReadFailed;
		}
	}
	
	while (hdr.Next != 0) {
		FOff = hdr.Next;
		
		if ((status = Source->Read(FOff, sizeof(hdr), &hdr, rw)) != Status::Success) {
			return status;
		} else if (rw != sizeof(hdr)) {
			return Status::ReadFailed;
		}
	}
	
e:	if ((status = Source->Write(FOff, sizeof(hdr), &hdr, rw)) != Status::Success) {
		return status;
	}
	
	return rw != sizeof(hdr) ? Status::WriteFailed : Status::Success;
}
