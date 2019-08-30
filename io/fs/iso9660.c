// File author is Ítalo Lima Marconato Matias
//
// Created on July 17 of 2018, at 16:10 BRT
// Last edited on August 30 of 2019, at 14:02 BRT

#include <chicago/alloc.h>
#include <chicago/file.h>
#include <chicago/iso9660.h>
#include <chicago/string.h>

Boolean Iso9660ReadFile(PFsNode file, UIntPtr off, UIntPtr len, PUInt8 buf) {
	if (file == Null) {																								// Let's do some null pointer checks first!
		return False;
	} else if ((file->priv == Null) || (file->inode == 0)) {
		return False;
	} else if (file->read == Null) {
		return False;
	} else if ((file->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {														// We're trying to read raw bytes from an directory?
		return False;																								// Why?
	} else if (off >= file->length) {																				// For byte per byte read
		return False;
	}
	
	PFsNode dev = file->priv;																						// Let's get our base device (it's inside of the priv)
	
	if (dev->read == Null) {																						// We have the read function... right?
		return False;																								// Nope (how you initialized this device WITHOUT THE READ FUNCTION????????????)
	} if ((dev->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {															// It's a file?
		return False;																								// Nope (again, how?)
	}
	
	PIso9660DirEntry entry = (PIso9660DirEntry)file->inode;															// Let's get our dir entry (it's inside of the inode)
	UIntPtr end = 0;
	
	if ((off + len) > entry->extent_length_lsb) {																	// Let's calc the size that we're going to read
		end = entry->extent_length_lsb;
	} else {
		end = off + len;
	}
	
	return dev->read(dev, (entry->extent_lba_lsb * 2048) + off, end - off, buf);									// And let's read!
}

Boolean Iso9660OpenFile(PFsNode node) {
	if (node == Null) {																								// Trying to open an null pointer?
		return False;																								// Yes (probably someone called this function directly... or the kernel have some fatal bug)
	} else {
		return True;
	}
}

Void Iso9660CloseFile(PFsNode node) {
	if (node == Null) {																								// Null pointer?
		return;																										// Yes
	} else if (StrCompare(node->name, L"\\")) {																		// Root directory?
		return;																										// Yes, don't free it, it's important for us (only the umount function can free it)
	}
	
	MemFree((UIntPtr)node->name);																					// Let's free everything that we allocated!
	MemFree(node->inode);
	MemFree((UIntPtr)node);
}

PWChar Iso9660ReadDirectoryEntry(PFsNode dir, UIntPtr entry) {
	if (dir == Null) {																								// I already did this comment (null pointer check) so many times... if you want, take a look in the other functions
		return Null;
	} else if ((dir->priv == Null) || (dir->inode == 0)) {
		return Null;
	} else if ((dir->flags & FS_FLAG_DIR) != FS_FLAG_DIR) {															// Trying to read an directory entry using an file... why?
		return Null;
	} else if (entry == 0) {																						// Current directory?
		return StrDuplicate(L".");
	} else if (entry == 1) {																						// Parent directory?
		return StrDuplicate(L"..");
	}
	
	PFsNode dev = dir->priv;																						// Get our device and make sure that it's valid
	
	if (dev->read == Null) {
		return False;
	} if ((dev->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {
		return False;
	}
	
	PIso9660DirEntry rent = (PIso9660DirEntry)dir->inode;															// Get our directory entry
	PUInt8 data = (PUInt8)MemAllocate(rent->extent_length_lsb);														// Allocate memory for reading it
	PUInt8 off = data;
	UIntPtr soff = 0;
	
	if (data == Null) {
		return Null;
	}
	
	for (UIntPtr i = rent->extent_length_lsb; i > 0; ) {															// Let's do it!
		if (!dev->read(dev, (rent->extent_lba_lsb + soff) * 2048, 2048, off)) {
			MemFree((UIntPtr)data);
			return Null;
		}
		
		if (i >= 2048) {
			off += 2048;
			i -= 2048;
			soff++;
		} else {
			break;
		}
	}
	
	UIntPtr idx = 0;
	
	for (off = data; (UIntPtr)(off - data) <= rent->extent_length_lsb; ) {											// Now, let's search for the file/directory of index 'i' inside of it
		PIso9660DirEntry dent = (PIso9660DirEntry)off;
		
		if (dent->directory_record_size == 0) {																		// 0-sized directory record?
			off++;																									// Yes, let's go to the next one (next byte, of course)
			continue;
		}
		
		if ((dent->flags & 0x01) != 0x01) {																			// Hidden file/directory?
			if (idx == entry) {																						// It's an normal, visible, file/directory! Is the one that we want?
				PWChar ret = Null;																					// YES!
				
				if (dent->name[dent->name_length - 2] == ';' && dent->name[dent->name_length - 1] == '1') {			// Let's alloc space for the name (and for fixing it)
					ret = (PWChar)MemAllocate((dent->name_length - 2) * sizeof(WChar));
				} else {
					ret = (PWChar)MemAllocate((dent->name_length + 1) * sizeof(WChar));
				}
				
				if (ret == Null) {																					// Failed? (When MemAllocate fails, we get a very big Null)
					MemFree((UIntPtr)data);
					return Null;
				}
				
				if (dent->name[dent->name_length - 2] == ';' && dent->name[dent->name_length - 1] == '1') {			// Fix the name?
					StrUnicodeFromC(ret, (PChar)dent->name, dent->name_length - 2);									// Yes
					ret[dent->name_length - 2] = '\0';
				} else {
					StrUnicodeFromC(ret, (PChar)dent->name, dent->name_length);										// No, so we only need to put the 0 (NUL) at the end of the string!
					ret[dent->name_length] = '\0';
				}
				
				MemFree((UIntPtr)data);
				
				return ret;
			}
			
			idx++;
		}
		
		off += dent->directory_record_size;
	}
	
	MemFree((UIntPtr)data);																							// We haven't found it...
	
	return Null;
}

PFsNode Iso9660FindInDirectory(PFsNode dir, PWChar name) {
	if ((dir == Null) || (name == Null)) {																			// The first part of this function IS EXACTLY EQUAL TO THE first part of ReadDirectoryEntry, if you want, take a look at it
		return Null;
	} else if ((dir->priv == Null) || (dir->inode == 0)) {
		return Null;
	} else if ((dir->flags & FS_FLAG_DIR) != FS_FLAG_DIR) {
		return Null;
	}
	
	PFsNode dev = dir->priv;
	
	if (dev->read == Null) {
		return False;
	} if ((dev->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {
		return False;
	}
	
	PIso9660DirEntry rent = (PIso9660DirEntry)dir->inode;
	PUInt8 data = (PUInt8)MemAllocate(rent->extent_length_lsb);
	PUInt8 off = data;
	UIntPtr soff = 0;
	
	if (data == Null) {
		return Null;
	}
	
	for (UIntPtr i = rent->extent_length_lsb; i > 0; ) {
		if (!dev->read(dev, (rent->extent_lba_lsb + soff) * 2048, 2048, off)) {
			MemFree((UIntPtr)data);
			return Null;
		}
		
		if (i >= 2048) {
			off += 2048;
			i -= 2048;
			soff++;
		} else {
			break;
		}
	}
	
	for (off = data; (UIntPtr)(off - data) <= rent->extent_length_lsb; ) {											// Now, let's search for the entry with the requested name
		PIso9660DirEntry dent = (PIso9660DirEntry)off;
		
		if (dent->directory_record_size == 0) {																		// 0-sized record?
			off++;																									// Yes, let's go to the next one (next byte, of course)
			continue;
		}
		
		if ((dent->flags & 0x01) != 0x01) {																			// Hidden file/directory?
			PWChar dename = Null;																					// It's an normal, visible, file/directory! Let's discover if it's the one that we want
			
			if (dent->name[dent->name_length - 2] == ';' && dent->name[dent->name_length - 1] == '1') {				// Let's do the same fix that we did in the ReadDirectoryEntry function
				dename = (PWChar)MemAllocate((dent->name_length - 2) * sizeof(WChar));
			} else {
				dename = (PWChar)MemAllocate((dent->name_length + 1) * sizeof(WChar));
			}
			
			if (dename == Null) {
				MemFree((UIntPtr)data);
				return Null;
			}
			
			if (dent->name[dent->name_length - 2] == ';' && dent->name[dent->name_length - 1] == '1') {
					StrUnicodeFromC(dename, (PChar)dent->name, dent->name_length - 2);
					dename[dent->name_length - 2] = '\0';
				} else {
					StrUnicodeFromC(dename, (PChar)dent->name, dent->name_length);
					dename[dent->name_length] = '\0';
				}
			
			if (StrGetLength(dename) == StrGetLength(name)) {														// Same length?
				if (StrCompare(dename, name)) {																		// YES! It's the entry that we want?
					PIso9660DirEntry de = (PIso9660DirEntry)MemAllocate(dent->directory_record_size);				// yes, so let's copy it
					
					if (de == Null) {
						MemFree((UIntPtr)dename);																	// The allocation failed...
						MemFree((UIntPtr)data);
						return Null;
					} else {
						StrCopyMemory(de, dent, dent->directory_record_size);										// Copy it!
					}
					
					PFsNode node = (PFsNode)MemAllocate(sizeof(FsNode));											// Alloc space for the fs node
					
					if (node == Null) {
						MemFree((UIntPtr)de);																		// Allocation failed, free everything and return Null
						MemFree((UIntPtr)dename);
						MemFree((UIntPtr)data);
						return Null;
					}
					
					node->name = dename;																			// Let's fill everything
					node->priv = dev;																				// Priv is the "device"
					
					if ((dent->flags & 0x02) == 0x02) {																// Directory?
						node->flags = FS_FLAG_DIR;																	// Yes!
						node->read = Null;
						node->readdir = Iso9660ReadDirectoryEntry;
						node->finddir = Iso9660FindInDirectory;
					} else {
						node->flags = FS_FLAG_FILE;																	// Nope
						node->read = Iso9660ReadFile;
						node->readdir = Null;
						node->finddir = Null;
					}
					
					node->inode = (UIntPtr)de;																		// Inode is the directory entry (because of this i copied it before)
					node->length = dent->extent_length_lsb;
					node->offset = 0;
					node->write = Null;
					node->open = Iso9660OpenFile;
					node->close = Iso9660CloseFile;
					node->create = Null;
					node->control = Null;
					
					MemFree((UIntPtr)data);																			// Free the allocated/readed directory data
					
					return node;																					// And return!
				}
			}
			
			MemFree((UIntPtr)dename);
		}
		
		off += dent->directory_record_size;
	}
	
	MemFree((UIntPtr)data);																							// We haven't found it...
	
	return Null;
}

Boolean Iso9660Probe(PFsNode file) {
	if (file == Null) {																								// Some checks (null pointer? file? HAVE THE READ FUNCTION???????????)
		return False;
	} else if ((file->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {
		return False;
	} else if (file->read == Null) {
		return False;
	}
	
	PIso9660PVD pvd = (PIso9660PVD)MemAllocate(sizeof(Iso9660PVD));													// Let's try to find the primary volume descriptor
	
	if (pvd == Null) {
		return False;																								// Allocation failed
	}
	
	for (UInt32 i = 0x10; i < 0x15 ; i++) {
		if (file->read(file, i * 2048, 2048, (PUInt8)pvd)) {														// Let's try to read this sector
			if ((pvd->type == 0x01) && (StrCompareMemory(pvd->cd001, "CD001", 5)) && (pvd->version == 0x01)) {		// It's the PVD?
				MemFree((UIntPtr)pvd);																				// Yes! So it's an valid Iso9660 fs!
				return True;
			} else if (pvd->type == 0xFF) {																			// It's the terminator...
				break;
			}
		} else {
			break;																									// Failed...
		}
	}
	
	MemFree((UIntPtr)pvd);
	
	return False;
}

PFsMountPoint Iso9660Mount(PFsNode file, PWChar path) {
	if ((file == Null) || (path == Null)) {
		return False;
	} else if ((file->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {
		return False;
	} else if (file->read == Null) {
		return False;
	}
	
	PIso9660PVD pvd = (PIso9660PVD)MemAllocate(sizeof(Iso9660PVD));
	Boolean found = False;
	
	if (pvd == Null) {
		return Null;
	}
	
	for (UInt32 i = 0x10; !found && i < 0x15; i++) {
		if (file->read(file, i * 2048, 2048, (PUInt8)pvd)) {														// Let's try to read this sector
			if ((pvd->type == 0x01) && (StrCompareMemory(pvd->cd001, "CD001", 5)) && (pvd->version == 0x01)) {		// It's the PVD?
				found = True;																						// Yes!
			} else if (pvd->type == 0xFF) {																			// It's the terminator...
				break;
			}
		} else {
			break;																									// Failed...
		}
	}
	
	if (!found) {																									// We have found it?
		MemFree((UIntPtr)pvd);																						// Nope
		return Null;
	}
	
	PIso9660DirEntry rootde = (PIso9660DirEntry)MemAllocate(34);													// Let's copy the root directory
	
	if (rootde == Null) {
		MemFree((UIntPtr)pvd);																						// Failed to allocate the required space :(
		return Null;
	} else {
		StrCopyMemory(rootde, (PVoid)(&pvd->root_directory), 34);													// Let's copy it
		MemFree((UIntPtr)pvd);																						// And free the PVD, we don't need it anymore
	}
	
	PFsMountPoint mp = (PFsMountPoint)MemAllocate(sizeof(FsMountPoint));											// Let's alloc space for the mount point struct
	
	if (mp == Null) {
		MemFree((UIntPtr)rootde);																					// Fai- really, i will stop putting those "Failed" and "Null pointer check" comments...
		return Null;
	}
	
	mp->path = StrDuplicate(path);																					// Let's try to duplicate the path string
	
	if (mp->path == Null) {
		MemFree((UIntPtr)mp);
		MemFree((UIntPtr)rootde);
		return Null;
	}
	
	mp->type = StrDuplicate(L"Iso9660");																			// And the type string
	
	if (mp->type == Null) {
		MemFree((UIntPtr)mp->path);
		MemFree((UIntPtr)mp);
		MemFree((UIntPtr)rootde);
		
		return Null;
	}
	
	mp->root = (PFsNode)MemAllocate(sizeof(FsNode));																// Create the root directory node
	
	if (mp->root == Null) {
		MemFree((UIntPtr)mp->type);
		MemFree((UIntPtr)mp->path);
		MemFree((UIntPtr)mp);
		MemFree((UIntPtr)rootde);
		
		return Null;
	}
	
	mp->root->name = StrDuplicate(L"\\");																			// Duplicate the name
	
	if (mp->root->name == Null) {
		MemFree((UIntPtr)mp->root);
		MemFree((UIntPtr)mp->type);
		MemFree((UIntPtr)mp->path);
		MemFree((UIntPtr)mp);
		MemFree((UIntPtr)rootde);
		
		return Null;
	}
	
	mp->root->priv = file;																							// Finally, fill everything!
	mp->root->flags = FS_FLAG_DIR;
	mp->root->inode = (UIntPtr)rootde;
	mp->root->length = 0;
	mp->root->offset = 0;
	mp->root->read = Null;
	mp->root->write = Null;
	mp->root->open = Iso9660OpenFile;
	mp->root->close = Iso9660CloseFile;
	mp->root->readdir = Iso9660ReadDirectoryEntry;
	mp->root->finddir = Iso9660FindInDirectory;
	mp->root->create = Null;
	mp->root->control = Null;
	
	return mp;
}

Boolean Iso9660Umount(PFsMountPoint mp) {
	if (mp == Null) {																								// Sanity checks
		return False;
	} else if (mp->root == Null) {
		return False;
	} else if (mp->root->priv == Null) {
		return False;
	} else if (mp->root->inode == 0) {
		return False;
	} else if (!StrCompare(mp->type, L"Iso9660")) {
		return False;
	}
	
	FsCloseFile((PFsNode)mp->root->priv);																			// Close the dev file
	MemFree(mp->root->inode);																						// And free the root inode
	
	return True;
}

Void Iso9660Init(Void) {
	PWChar name = StrDuplicate(L"Iso9660");																			// Let's get our type string
	
	if (name == Null) {
		return;
	}
	
	FsAddType(name, Iso9660Probe, Iso9660Mount, Iso9660Umount);														// And add ourself into the type list
}
