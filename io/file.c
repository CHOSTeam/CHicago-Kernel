// File author is Ítalo Lima Marconato Matias
//
// Created on July 16 of 2018, at 18:28 BRT
// Last edited on January 26 of 2020, at 12:59 BRT

#include <chicago/alloc.h>
#include <chicago/debug.h>
#include <chicago/file.h>
#include <chicago/panic.h>
#include <chicago/rand.h>
#include <chicago/string.h>

List FsMountPointList = { Null, Null, 0, False };
List FsTypeList = { Null, Null, 0, False };

static UIntPtr FsCountSeparations(PWChar path) {
	UIntPtr count = 0;
	
	for (UIntPtr i = 0; i < StrGetLength(path); i++) {
		if (path[i] == '/') {
			count++;
		}
	}
	
	return count;
}

PList FsTokenizePath(PWChar path) {
	if (path == Null) {																													// Path is Null?
		return Null;																													// Yes...
	} else if ((StrGetLength(path) == 0) || ((StrGetLength(path) == 1) && (path[0] == '/'))) {											// Root directory?
		return ListNew(True);																											// Yes, so just return an empty list
	}
	
	PWChar clone = StrDuplicate(path);
	PList list = ListNew(True);																											// Create the tok list
	
	if (list == Null || clone == Null) {																								// Failed to alloc it?
		return Null;																													// Yes, so we can't do anything :(
	} else if (FsCountSeparations(path) == 0) {																							// If the path doesn't have any '/', we don't need to do anything
		ListAdd(list, clone);
		return list;
	}
	
	PWChar tok = StrTokenize(clone, L"/");																								// Let's tokenize it!
	
	while (tok != Null) {
		if ((StrGetLength(tok) == 2) && (StrCompare(tok, L".."))) {																		// Parent directory (..)?
			if (list->length > 0) {																										// We're in the root directory?
				MemFree((UIntPtr)(ListRemove(list, list->length - 1)));																	// No, so remove the last entry (current one)
			}
		} else if (!(((StrGetLength(tok) == 1) && (StrCompare(tok, L"."))) || StrGetLength(tok) == 0)) {								// Current directory (.)?
			ListAdd(list, StrDuplicate(tok));																							// No, so add it to the list
		}
		
		tok = StrTokenize(Null, L"/");
	}
	
	MemFree((UIntPtr)clone);
	
	return list;
}

PWChar FsCanonicalizePath(PWChar path) {
	if (path == Null) {																													// Path is Null?
		return Null;																													// YES
	} else if (path[0] != '/') {																										// Absolute path?
		return Null;																													// No, but we don't support relative paths in this function :(
	}
	
	PList list = ListNew(True);																											// Create the tok list
	
	if (list == Null) {																													// Failed to alloc space for it?
		return Null;																													// :(
	}
	
	PWChar final = Null;
	PWChar foff = Null;
	UIntPtr fsize = 0;
	
	if (FsCountSeparations(path) == 1) {																								// Do we need to tokenize it?
		ListAdd(list, StrDuplicate(path + 1));																							// Nope :)
	} else {
		PWChar tok = StrTokenize(path, L"/");																							// So, we need to tokenize it (if you want, take a look in the FsTokenizePath function)
		
		while (tok != Null) {
			if ((StrGetLength(tok) == 2) && (StrCompare(tok, L".."))) {
				if (list->length > 0) {
					MemFree((UIntPtr)(ListRemove(list, list->length - 1)));
				}
			} else if (!((StrGetLength(tok) == 1) && (StrCompare(tok, L".")))) {
				ListAdd(list, StrDuplicate(tok));
			}

			tok = StrTokenize(Null, L"/");
		}
	}
	
	ListForeach(list, i) {																												// No, so let's get the final size
		fsize += (StrGetLength((PWChar)(i->data)) + 2) * sizeof(WChar);
	}
	
	final = foff = (PWChar)MemAllocate(fsize);																							// Alloc space
	
	if (final == Null) {																												// Failed?
		ListFree(list);																													// Yes, so free everything and return
		return Null;
	}
	
	ListForeach(list, i) {																												// Now let's copy everything to the ret string!
		StrCopy(foff++, L"/");
		StrCopy(foff, (PWChar)(i->data));
		foff += StrGetLength((PWChar)(i->data));
	}
	
	ListFree(list);																														// Free our list
	
	return final;																														// And return!
}

PWChar FsJoinPath(PWChar src, PWChar incr) {
	if ((src == Null) && (incr == Null)) {																								// We need at least one of them!
		return Null;
	} else if ((src != Null) && (incr == Null)) {																						// We only have src?
		return FsCanonicalizePath(src);																									// Yes, so we can only canonicalize it
	} else if ((src == Null) && (incr != Null)) {																						// Only incr?
		return FsCanonicalizePath(incr);																								// Yes, so we can only canonicalize it
	} else if (src[0] != '/') {																											// Absolute path?
		return Null;																													// No
	}
	
	UIntPtr psize = (StrGetLength(src) + StrGetLength(incr) + 2) * sizeof(WChar);														// Let's calculate the final string size
	PWChar path = Null;
	PWChar poff = Null;
	
	if (incr[0] != '/' && StrGetLength(src) >= 2 && src[StrGetLength(src) - 2] != '/') {
		psize += sizeof(WChar);
	}
	
	path = poff = (PWChar)MemAllocate(psize);																							// Allocate some space
	
	if (path == Null) {																													// Failed?
		return Null;																													// Yes
	}
	
	StrCopy(poff, src);																													// Copy the src
	poff += StrGetLength(src);
	
	if (incr[0] != '/' && StrGetLength(src) >= 2 && src[StrGetLength(src) - 2] != '/') {
		*poff++ = '/';
	}
	
	StrCopy(poff, incr);																												// Copy the incr
	
	PWChar final = FsCanonicalizePath(path);																							// Canonicalize
	
	MemFree((UIntPtr)path);																												// Free our temp path
	
	return final;																														// And return
}

PWChar FsGetRandomPath(PWChar prefix) {
	PWChar name = (PWChar)MemAllocate(9);																								// Random names are 8-characters long
	
	while (name != Null) {																												// Let's do it!
		for (UIntPtr i = 0; i < 8; i++) {																								// Generate 8 "random" hex numbers
			name[i] = L"0123456789ABCDEF"[RandGenerate() % 16];
			RandSetSeed(RandGenerate());
		}
		
		name[8] = 0;																													// End with zero
		
		PWChar path = FsJoinPath(prefix, name);																							// Join the prefix and the random name
		
		if (path != Null) {																												// Failed?
			PFsNode file = Null;
			Status status = FsOpenFile(path, 0, &file);																					// No, let's check if it already exists
			
			MemFree((UIntPtr)path);																										// And let's free the path
			
			if (status == STATUS_FILE_DOESNT_EXISTS) {
				return name;																											// Yeah, it doesn't exists, so we can return it!
			}
			
			FsCloseFile(file);																											// Close the file, and let's keep on trying!
		}
	}
	
	return Null;
}

Status FsReadFile(PFsNode file, UIntPtr off, UIntPtr len, PUInt8 buf, PUIntPtr read) {
	if (file == Null || len == 0 || buf == Null) {																						// Sanity check
		return STATUS_INVALID_ARG;
	} else if ((file->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {																			// We're trying to read an file?
		return STATUS_NOT_FILE;																											// ... Why are you trying to read raw bytes from an directory?
	} else if (file->read == Null || (file->flags & FS_FLAG_READ) != FS_FLAG_READ) {													// Can we read it?
		return STATUS_CANT_READ;																										// Nope...
	}
	
	return file->read(file, off, len, buf, read);																						// Yeah, we can!
}

Status FsWriteFile(PFsNode file, UIntPtr off, UIntPtr len, PUInt8 buf, PUIntPtr write) {
	if (file == Null || len == 0 || buf == Null) {																						// Sanity check
		return STATUS_INVALID_ARG;
	} else if ((file->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {																			// We're trying to read an file?
		return STATUS_NOT_FILE;																											// ... Why are you trying to read raw bytes from an directory?
	} else if (file->write == Null || (file->flags & FS_FLAG_WRITE) != FS_FLAG_WRITE) {													// Can we write to it?
		return STATUS_CANT_WRITE;																										// Nope...
	}
	
	return file->write(file, off, len, buf, write);																						// Yeah, we can!
}

static Status FsOpenFileInt(PFsNode node, UIntPtr flags) {
	if (node == Null) {																													// Node is Null pointer?
		return STATUS_INVALID_ARG;																										// Yes... (Really, those comments are starting to get annoying)
	} else if (node->open != Null) {																									// Any implementation?
		Status status = node->open(node);																								// Yes, so call it
		
		if (status != STATUS_SUCCESS) {
			return status;																												// The open failed...
		} else if (((flags & FS_FLAG_READ) == FS_FLAG_READ) && node->read == Null) {													// Can we read?
			return STATUS_CANT_READ;																									// Nope...
		} else if (((flags & FS_FLAG_WRITE) == FS_FLAG_WRITE) && node->write == Null) {													// Can we write?
			return STATUS_CANT_WRITE;																									// Nope...
		}
		
		return STATUS_SUCCESS;
	} else {
		return STATUS_OPEN_FAILED;
	}
}

static Status FsCreateFileInt(PFsNode dir, PWChar name, UIntPtr type) {
	if ((dir == Null) || (name == Null)) {
		return STATUS_INVALID_ARG;
	} else if ((dir->flags & FS_FLAG_DIR) != FS_FLAG_DIR) {																				// Directory-only function!
		return STATUS_NOT_DIR;
	}
	
	PFsNode file;
	Status status = FsFindInDirectory(dir, name, &file);																				// Let's try to not create entries with the same name
	
	if (status == STATUS_SUCCESS) {
		FsCloseFile(file);																												// ...
		return STATUS_FILE_ALREADY_EXISTS;
	}
	
	if (dir->create != Null) {
		return dir->create(dir, name, type);
	} else {
		return STATUS_CREATE_FAILED;
	}
}

Status FsOpenFile(PWChar path, UIntPtr flags, PFsNode *ret) {
	if ((path == Null) || (ret == Null)) {																								// Some null pointer checks
		return STATUS_INVALID_ARG;
	} else if (FsMountPointList.length == 0) {																							// Don't even lose time trying to open some file if our mount point list is empty
		return STATUS_FILE_DOESNT_EXISTS;
	} else if (path[0] != '/') {																										// Finally, we only support absolute paths in this function
		return STATUS_OPEN_FAILED;
	}
	
	UIntPtr flgs = 0;
	
	if ((flags & OPEN_FLAGS_READ) == OPEN_FLAGS_READ) {																					// Parse the flags
		flgs |= FS_FLAG_READ;
	}
	
	if ((flags & OPEN_FLAGS_WRITE) == OPEN_FLAGS_WRITE) {
		flgs |= FS_FLAG_WRITE;
	}
	
	PWChar rpath = Null;
	PFsMountPoint mp = FsGetMountPoint(path, &rpath);																					// Find the mount point
	
	if ((mp == Null) || (rpath == Null)) {																								// Failed?
		return STATUS_FILE_DOESNT_EXISTS;																								// Yes
	}
	
	PList parts = FsTokenizePath(rpath);																								// Let's try to tokenize our path (relative to the mount point)!
	PFsNode cur = mp->root;
	
	if (parts == Null) {																												// Failed?
		return STATUS_OUT_OF_MEMORY;																									// Yes
	}
	
	Status status = FsOpenFileInt(cur, 0);																								// Let's try to "open" the mount point root directory
	
	if (status != STATUS_SUCCESS) {
		ListFree(parts);																												// Failed
		return status;
	}
	
	UIntPtr ftype = (flags & OPEN_FLAGS_DIR) ? FS_FLAG_DIR : FS_FLAG_FILE;
	
	ListForeach(parts, i) {
		PFsNode new;
		status = FsFindInDirectory(cur, (PWChar)(i->data), &new);																		// Let's try to find the file/folder in the folder (the file is i->data and the folder is cur)
		
		FsCloseFile(cur);																												// Close the current file (we don't want to leak memory)
		cur = new;
		
		if (status == STATUS_FILE_DOESNT_EXISTS && (flags & OPEN_FLAGS_CREATE) == OPEN_FLAGS_CREATE) {									// Should we just try to create the folder/file?
			if ((flags & OPEN_FLAGS_DONT_CREATE_DIRS) == OPEN_FLAGS_DONT_CREATE_DIRS && i->next == Null) {								// I think so... is this a dir? If yes, should we create it?
				goto c;																													// ... So just fail...
			}
			
			status = FsCreateFileInt(cur, (PWChar)(i->data), i->next == Null ? ftype : FS_FLAG_DIR);									// Create the file
			
			if (status != STATUS_SUCCESS) {
				goto c;																													// Failed
			}
			
			status = FsFindInDirectory(cur, (PWChar)(i->data), &cur);																	// Now, try to find it again...
		} else if (status == STATUS_SUCCESS && (flags & OPEN_FLAGS_ONLY_CREATE) == OPEN_FLAGS_ONLY_CREATE && i->next == Null) {			// Wait, in case this is actually the file (that is, the next entry is Null), should we really just open it, or should we fail?
			status = STATUS_FILE_ALREADY_EXISTS;																						// We should fail...
		}
		
c:		if (status != STATUS_SUCCESS) {
			ListFree(parts);																											// Failed
			return status;
		}
		
		if ((status = FsOpenFileInt(cur, i->next == Null ? flgs : 0)) != STATUS_SUCCESS) {												// Let's try to "open" the file/folder
			FsCloseFile(cur);																											// Failed
			ListFree(parts);
			return status;
		} else if (i->next == Null && (cur->flags & ftype) != ftype) {																	// In case this is actually the file/dir, is this the right type? That is, in case we asked for a directory, is this a directory?
			FsCloseFile(cur);																											/// ...
			ListFree(parts);
			return STATUS_OPEN_FAILED;
		}
	}
	
	if ((flags & OPEN_FLAGS_APPEND) == OPEN_FLAGS_APPEND) {																				// Move to the end of the file?
		cur->offset = cur->length;																										// Yeah
	}
	
	ListFree(parts);																													// Finally, free the list
	
	cur->flags |= flgs;																													// Set the flags
	*ret = cur;																															// And return!
	
	return STATUS_SUCCESS;
}

Status FsCloseFile(PFsNode node) {
	if (node == Null) {																													// Look, except for FsOpenFile, FsMountFile and FsUmountFile, all the other function follows the same model, so if you want, just take a look at any of them!
		return STATUS_INVALID_ARG;
	} else if (node->close != Null) {
		node->close(node);
	}
	
	return STATUS_SUCCESS;
}

Status FsMountFile(PWChar path, PWChar file, PWChar type) {
	if ((path == Null) || (file == Null)) {																								// Null pointer check...
		return STATUS_INVALID_ARG;
	}
	
	PFsNode src;																														// Try to open the source file
	Status status = FsOpenFile(file, OPEN_FLAGS_READ, &src);																			// We need at least the read permission
	
	if (status != STATUS_SUCCESS) {																										// Failed?
		return status;																													// Yes
	} else if ((src->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {																			// The file isn't an file?
		FsCloseFile(src);																												// Yeah, but we can't mount directories...
		return STATUS_NOT_FILE;
	}
	
	PFsNode dest;
	status = FsOpenFile(path, 0, &dest);																								// Try to open the destination file
	
	if ((status != STATUS_SUCCESS) && (!StrCompare(path, L"/"))) {																		// Failed (and we aren't trying to mount the root directory)?
		FsCloseFile(src);																												// Yes, close the src file
		return STATUS_MOUNT_FAILED;																										// And return
	} else if (status != STATUS_SUCCESS) {
		dest = Null;
	} else if (status == STATUS_SUCCESS) {
		if (!StrCompare(path, L"/")) {																									// Trying to mount the root directory?
			if ((dest->flags & FS_FLAG_DIR) != FS_FLAG_DIR) {																			// No, so we need to check if the dest is an directory!
				FsCloseFile(dest);																										// Isn't, so close it, close src and return
				FsCloseFile(src);
				return STATUS_NOT_DIR;
			}
		}
	}
	
	PFsType typ = FsGetType(type);																										// Let's try to get the type
	
	if ((type != Null) && (typ == Null)) {																								// If it returned Null and was an user requested type, IT FAILED, RETURN NULL
		FsCloseFile(dest);
		FsCloseFile(src);
		return STATUS_MOUNT_FAILED;
	} else if (typ != Null) {																											// Else (if was user requested type and was found), let's do some probe
		if (typ->probe != Null) {
			if (!typ->probe(src)) {
				FsCloseFile(dest);																										// Failed... we can't mount it
				FsCloseFile(src);
				return STATUS_MOUNT_FAILED;
			}
		}
	} else if (type == Null) {
		ListForeach(&FsTypeList, i) {																									// Let's probe all the entries of the fs type list!
			PFsType tp = (PFsType)(i->data);
			
			if (tp->probe != Null) {
				if (tp->probe(src)) {
					typ = tp;																											// Found it!
					break;
				}
			}
		}
		
		if (typ == Null) {
			FsCloseFile(dest);																											// We haven't found it...
			FsCloseFile(src);
			return STATUS_MOUNT_FAILED;
		}
	}
	
	if (typ->mount == Null) {
		FsCloseFile(dest);																												// WTF? This FS doesn't have an mount function lol
		FsCloseFile(src);
		return STATUS_MOUNT_FAILED;
	}
	
	PFsMountPoint mp;
	status = typ->mount(src, path, &mp);																								// Try to mount it
	
	if (status != STATUS_SUCCESS) {																										// The mount failed?
		FsCloseFile(dest);																												// Yes :(
		FsCloseFile(src);
		return status;
	}
	
	FsCloseFile(dest);																													// Let's close our files, we don't need them anymore
	
	if (!FsAddMountPoint(mp->path, mp->type, mp->root)) {																				// And let's try to add this mount point
		MemFree((UIntPtr)mp);																											// Failed, so return False
		FsCloseFile(src);
		return STATUS_OUT_OF_MEMORY;
	} else {
		MemFree((UIntPtr)mp);																											// Otherwise, return True
		return STATUS_SUCCESS;
	}
}

Status FsUmountFile(PWChar path) {
	if (path == Null) {																													// Again, null pointer checks
		return STATUS_INVALID_ARG;
	}
	
	PWChar rpath = Null;
	PFsMountPoint mp = FsGetMountPoint(path, &rpath);																					// Let's get the mount point info/struct
	
	if (mp == Null) {																													// Failed...
		return STATUS_NOT_MOUNTED;
	} else if (!StrCompare(rpath, L"")) {
		return STATUS_UMOUNT_FAILED;																									// You need to pass the absolute path to the mount point to this function!
	}
	
	PFsType type = FsGetType(mp->type);																									// Try to get the fs type info
	
	if (type != Null) {																													// Failed?
		if (type->umount != Null) {																										// No, we can call the umount function?
			if (!type->umount(mp)) {																									// Yes!
				return STATUS_UMOUNT_FAILED;																							// Failed...
			}
		}
	}
	
	return FsRemoveMountPoint(path) ? STATUS_SUCCESS : STATUS_UMOUNT_FAILED;															// The FsRemoveMountPoint will do all the job now!
}

Status FsReadDirectoryEntry(PFsNode dir, UIntPtr entry, PWChar *ret) {
	if (dir == Null || ret == Null) {
		return STATUS_INVALID_ARG;
	} else if ((dir->flags & FS_FLAG_DIR) != FS_FLAG_DIR) {																				// Directory-only function!
		return STATUS_NOT_DIR;
	} else if (dir->readdir != Null) {
		return dir->readdir(dir, entry, ret);
	} else {
		return STATUS_CANT_READ;
	}
}

Status FsFindInDirectory(PFsNode dir, PWChar name, PFsNode *ret) {
	if ((dir == Null) || (name == Null)) {
		return STATUS_INVALID_ARG;
	} else if ((dir->flags & FS_FLAG_DIR) != FS_FLAG_DIR) {																				// Directory-only function!
		return STATUS_NOT_DIR;
	} else if (dir->finddir != Null) {
		return dir->finddir(dir, name, ret);
	} else {
		return STATUS_CANT_READ;
	}
}

Status FsControlFile(PFsNode file, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf) {
	if (file == Null) {																													// File is Null pointer?
		return STATUS_INVALID_ARG;																										// Yes, so we can't do anything...
	} else if ((file->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {																			// We're trying to control an file?
		return STATUS_NOT_FILE;																											// ... Why... WHY?
	} else if (file->control != Null) {																									// Implementation?
		return file->control(file, cmd, ibuf, obuf);																					// Yes, so call it!
	} else {
		return STATUS_CANT_CONTROL;
	}
}

PFsMountPoint FsGetMountPoint(PWChar path, PWChar *outp) {
	if (path == Null || FsMountPointList.length == 0) {																					// We have anything to do?
		if (outp != Null) {																												// Nope, let's set the outp (out pointer) to Null and return Null
			*outp = Null;
		}
		
		return Null;
	}
	
	PWChar dup = StrDuplicate(path);																									// Let's duplicate the path, as we're going to change the string
	UIntPtr ncurr = StrGetLength(dup) - 1;
	
	if (dup == Null) {
		return Null;
	} else if (!StrCompare(dup, L"/")) {																								// We're trying to find the root mount point?
		while (dup[ncurr] == '/') {																										// No
			dup[ncurr--] = '\0';
		}
	}
	
	while (dup[0] != '\0') {
		Boolean root = StrCompare(dup, L"/");
		
		ListForeach(&FsMountPointList, i) {
			PFsMountPoint mp = (PFsMountPoint)(i->data);
			
			if (StrGetLength(mp->path) == (ncurr + 1)) {																				// The length of this mount point is the same of the current length of our duplicate pointer?
				if (StrCompare(mp->path, dup)) {																						// Yes, so let's compare!
					MemFree((UIntPtr)dup);																								// WE FOUND IT! So free our duplicate, we don't need it anymore :)
					
					if (outp != Null) {																									// If the user requested, let's save the relative path
						if ((mp->path[StrGetLength(mp->path) - 1] == '/') || (StrCompare(path, mp->path))) {							// The mount point path finishes with an slash (or we're trying to "get" the root directory of the mount point)?
							*outp = path + StrGetLength(mp->path);																		// Yes, so we can use mp->path length
						} else {
							*outp = path + StrGetLength(mp->path) + 1;																	// No, so we need to use mp->path length + 1
						}
					}
					
					return mp;																											// And return the mount point info
				}
			}
		}
		
		while (dup[ncurr] != '/') {
			dup[ncurr--] = '\0';
		}
		
		if (!(StrCompare(dup, L"/") && !root)) {
			dup[ncurr--] = '\0';
		}
	}
	
	MemFree((UIntPtr)dup);																												// Alright, we failed (this mount point doesn't exists...)
	
	if (outp != Null) {																													// So let's set the outp (out pointer) to Null
		*outp = Null;
	}
	
	return Null;																														// And return Null
}

PFsType FsGetType(PWChar type) {
	if (type == Null) {																													// Null pointer checks
		return Null;
	} else if (FsTypeList.length == 0) {																								// Type list have any entry?
		return Null;																													// No? So don't even lose time searching in it
	}
	
	ListForeach(&FsTypeList, i) {
		PFsType typ = (PFsType)(i->data);
		
		if (StrGetLength(typ->name) != StrGetLength(type)) {																			// Same length as the requested type?
			continue;																													// No, so we don't even need to compare
		} else if (StrCompare(typ->name, type)) {																						// We found it?
			return typ;																													// We found it
		}
	}
	
	return Null;																														// We haven't found it
}

Boolean FsAddMountPoint(PWChar path, PWChar type, PFsNode root) {
	if ((path == Null) || (type == Null) || (root == Null)) {																			// Null pointer checks
		return False;
	}
	
	PWChar rpath = Null;
	FsGetMountPoint(path, &rpath);
	UIntPtr len = StrGetLength(path);
	 
	if (path[len - 1] == '/' && rpath != Null && !StrCompare(rpath, L"")) {																// This mount point doesn't exist right?
		return False;																													// ...
	} else if (path[len - 1] != '/' && rpath != Null && StrCompare(path, L"")) {														// Same check
		return False;
	}
	
	PFsMountPoint mp = (PFsMountPoint)MemAllocate(sizeof(FsMountPoint));																// Let's try to allocate space for the struct
	
	if (mp == Null) {
		return False;																													// Failed
	}
	
	mp->path = path;
	mp->type = type;
	mp->root = root;
	
	if (!ListAdd(&FsMountPointList, mp)) {																								// Try to add it to the list
		MemFree((UIntPtr)mp);																											// Failed...
		return False;
	}
	
	return True;
}

Boolean FsRemoveMountPoint(PWChar path) {
	if (path == Null) {																													// Null pointer?
		return False;																													// Yes
	}
	
	PWChar rpath = Null;
	PFsMountPoint mp = FsGetMountPoint(path, &rpath);																					// Let's try to find the mount point
	UIntPtr len = StrGetLength(path);
	
	if ((mp == Null) || (rpath == Null)) {																								// Found it?
		return False;																													// No....
	} else if (path[len - 1] == '/' && !StrCompare(rpath, L"")) {																		// The user tried to remove the mount point using the name of an file/folder that was inside of it?
		return False;																													// Yes...
	} else if (path[len - 1] != '/' && StrCompare(rpath, L"")) {																		// Same check
		return False;
	}
	
	UIntPtr idx;
	
	if (!ListSearch(&FsMountPointList, mp, &idx)) {																						// Try to find the mp on the list...
		return False;
	} else if (ListRemove(&FsMountPointList, idx) == Null) {																			// Try to remove it!
		return False;																													// IT FAILED WHEN IT WAS REMOVING... REMOVING!
	}
	
	MemFree((UIntPtr)mp->root->name);																									// Free everything
	MemFree((UIntPtr)mp->root);
	MemFree((UIntPtr)mp->type);
	MemFree((UIntPtr)mp->path);
	MemFree((UIntPtr)mp);
	
	return True;																														// And return True!
}

Boolean FsAddType(PWChar name, Boolean (*probe)(PFsNode), Status (*mount)(PFsNode, PWChar, PFsMountPoint*), Boolean (*umount)(PFsMountPoint)) {
	if ((name == Null) || (probe == Null) || (mount == Null) || (umount == Null)) {														// Null pointer checks
		return False;
	} else if (FsGetType(name) != Null) {																								// This fs type doesn't exists... right?
		return False;																													// ...
	}
	
	PFsType type = (PFsType)MemAllocate(sizeof(FsType));																				// Let's try to allocate space for the struct
	
	if (type == Null) {
		return False;																													// Failed
	}
	
	type->name = name;
	type->probe = probe;
	type->mount = mount;
	type->umount = umount;
	
	if (!ListAdd(&FsTypeList, type)) {																									// Try to add it to the list
		MemFree((UIntPtr)type);																											// Failed...
		return False;
	}
	
	return True;
}

Boolean FsRemoveType(PWChar name) {
	if (name == Null) {																													// Null pointer?
		return False;																													// Yes
	}
	
	PFsType type = FsGetType(name);																										// Let's try to find the fs type
	
	if (type == Null) {																													// Found it?
		return False;																													// No...
	}
	
	UIntPtr idx;																														// Search for the index of the type on the type list
	
	if (!ListSearch(&FsTypeList, type, &idx)) {
		return False;
	} else if (ListRemove(&FsTypeList, idx) == Null) {																					// Try to remove it!
		return False;																													// IT FAILED WHEN IT WAS REMOVING... REMOVING!
	}
	
	MemFree((UIntPtr)type->name);																										// Free everything
	MemFree((UIntPtr)type);
	
	return True;																														// And return True!
}

Void FsInitTypes(Void) {
	DevFsInit();																														// Mount the DevFs
	Iso9660Init();																														// Add the Iso9660 to the fs type list
}

Void FsInit(Void) {
	FsInitTypes();																														// Init all the supported fs types
	
	PWChar bdpath = FsJoinPath(L"/Devices", FsGetBootDevice());																			// Let's mount the boot device
	
	if (bdpath == Null) {
		DbgWriteFormated("PANIC! Couldn't mount the boot device\r\n");
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	if (FsMountFile(L"/", bdpath, Null) != STATUS_SUCCESS) {
		DbgWriteFormated("PANIC! Couldn't mount the boot device\r\n");
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	MemFree((UIntPtr)bdpath);
}
