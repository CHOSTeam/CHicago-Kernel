// File author is Ítalo Lima Marconato Matias
//
// Created on July 16 of 2018, at 18:29 BRT
// Last edited on February 02 of 2020, at 10:55 BRT

#include <chicago/alloc.h>
#include <chicago/debug.h>
#include <chicago/device.h>
#include <chicago/file.h>
#include <chicago/panic.h>
#include <chicago/string.h>

static Status DevFsControlFile(PFsNode file, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf);
static Status DevFsMapFile(PFsNode file, UIntPtr off, UIntPtr addr, UInt32 prot);
static Status DevFsSyncFile(PFsNode file, UIntPtr off, UIntPtr start, UIntPtr size);

static Status DevFsReadFile(PFsNode file, UIntPtr off, UIntPtr len, PUInt8 buf, PUIntPtr read) {
	if (file == Null || len == 0 || buf == Null || read == Null) {								// Any null pointer?
		return STATUS_INVALID_ARG;																// Yes, so we can't continue
	} else if ((file->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {									// We're trying to read raw bytes in an... Directory?
		return STATUS_NOT_FILE;																	// Yes (Why?)
	}
	
	PDevice dev = FsGetDevice(file->name);														// Get the device using the name
	
	if (dev == Null) {																			// Failed for some unknown reason?
		return STATUS_READ_FAILED;																// Yes
	}
	
	return FsReadDevice(dev, off, len, buf, read);												// Redirect to the FsReadDevice function
}

static Status DevFsWriteFile(PFsNode file, UIntPtr off, UIntPtr len, PUInt8 buf, PUIntPtr write) {
	if (file == Null || len == 0 || buf == Null || write == Null) {								// Any null pointer?
		return STATUS_INVALID_ARG;																// Yes, so we can't continue
	} else if ((file->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {									// We're trying to write raw bytes to an... Directory?
		return STATUS_NOT_FILE;																	// Yes (Why?)
	}
	
	PDevice dev = FsGetDevice(file->name);														// Get the device using the name
	
	if (dev == Null) {																			// Failed for some unknown reason?
		return STATUS_WRITE_FAILED;																// Yes
	}
	
	return FsWriteDevice(dev, off, len, buf, write);											// Redirect to the FsWriteDevice function
}

static Status DevFsOpenFile(PFsNode node) {
	if (node == Null) {																			// Null pointer?
		return STATUS_INVALID_ARG;																// Yes
	} else if (StrCompare(L"/", node->name)) {													// Root?
		return STATUS_SUCCESS;																	// Yes
	}
	
	PDevice dev = FsGetDevice(node->name);														// Get the device using the name
	
	if (dev == Null) {																			// Failed for some unknown reason?
		return STATUS_INVALID_ARG;																// Yes
	} else if (dev->read == Null && (node->flags & FS_FLAG_READ) == FS_FLAG_READ) {				// We can't read from the device, but the user wants to read it?
		return STATUS_CANT_READ;
	} else if (dev->write == Null && (node->flags & FS_FLAG_WRITE) == FS_FLAG_WRITE) {			// We can't write from the device, but the user wants to write it?
		return STATUS_CANT_WRITE;
	}
	
	return STATUS_SUCCESS;
}

static Void DevFsCloseFile(PFsNode node) {
	if (node == Null) {																			// Null pointer?
		return;																					// Yes
	} else if (StrCompare(node->name, L"/")) {													// Root directory?
		return;																					// Yes, DON'T FREE IT, NEVER!
	}
	
	MemFree((UIntPtr)node->name);																// Free everything!
	MemFree((UIntPtr)node);
}

static Status DevFsReadDirectoryEntry(PFsNode dir, UIntPtr entry, PWChar *ret) {
	if (dir == Null || ret == Null) {															// Any null pointer?
		return STATUS_INVALID_ARG;																// Yes, so we can't continue
	} else if ((dir->flags & FS_FLAG_DIR) != FS_FLAG_DIR) {										// We're trying to do ReadDirectoryEntry in an... File?
		return STATUS_NOT_DIR;																	// Yes (Why?)
	} else if (entry == 0) {																	// Current directory?
		*ret = StrDuplicate(L".");
		return *ret == Null ? STATUS_OUT_OF_MEMORY : STATUS_SUCCESS;
	} else if (entry == 1) {																	// Parent directory?
		*ret = StrDuplicate(L"..");
		return *ret == Null ? STATUS_OUT_OF_MEMORY : STATUS_SUCCESS;
	} else {
		PDevice dev = FsGetDeviceByID(entry - 2);												// Get the device by ID (using the entry)
		
		if (dev == Null) {																		// Found?
			return STATUS_FILE_DOESNT_EXISTS;													// Nope
		}
		
		*ret = StrDuplicate(dev->name);															// Return the device name
		
		return *ret == Null ? STATUS_OUT_OF_MEMORY : STATUS_SUCCESS;
	}
}

static Status DevFsFindInDirectory(PFsNode dir, PWChar name, PFsNode *ret) {
	if ((dir == Null) || (name == Null) || (ret == Null)) {										// Any null pointer?
		return STATUS_INVALID_ARG;																// Yes, so we can't continue
	} else if ((dir->flags & FS_FLAG_DIR) != FS_FLAG_DIR) {										// We're trying to do FindInDirectory in an... File?
		return STATUS_NOT_DIR;																	// Yes (Why?)
	}
	
	UIntPtr inode = FsGetDeviceID(name);														// Try to get the device by the name
	
	if (inode == (UIntPtr)-1) {																	// Failed?
		return STATUS_FILE_DOESNT_EXISTS;														// Yes
	}
	
	PDevice dev = FsGetDeviceByID(inode);														// Get the dev struct
	
	if (dev == Null) {
		return STATUS_READ_FAILED;																// Failed :(
	}
	
	PFsNode node = (PFsNode)MemAllocate(sizeof(FsNode));										// Alloc the fs node struct
	
	if (node == Null) {																			// Failed?
		return STATUS_OUT_OF_MEMORY;															// Yes
	}
	
	node->name = StrDuplicate(dev->name);														// Duplicate the name
	
	if (node->name == Null) {																	// Failed?
		MemFree((UIntPtr)node);																	// Yes, so free everything
		return STATUS_OUT_OF_MEMORY;															// And return
	}
	
	node->priv = Null;
	node->flags = FS_FLAG_FILE;
	node->inode = inode;
	node->length = 0;
	node->offset = 0;
	node->read = DevFsReadFile;
	node->write = DevFsWriteFile;
	node->open = DevFsOpenFile;
	node->close = DevFsCloseFile;
	node->readdir = Null;
	node->finddir = Null;
	node->create = Null;
	node->control = DevFsControlFile;
	node->map = dev->map == Null ? Null : DevFsMapFile;
	node->sync = dev->sync == Null ? Null : DevFsSyncFile;
	
	*ret = node;
	
	return STATUS_SUCCESS;
}

static Status DevFsControlFile(PFsNode file, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf) {
	if (file == Null) {																			// Any null pointer?
		return STATUS_INVALID_ARG;																// Yes, so we can't continue
	} else if ((file->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {									// ... We're trying to use the control in an directory?
		return STATUS_NOT_FILE;																	// Yes (Why?)
	}
	
	PDevice dev = FsGetDevice(file->name);														// Get the device using the name
	
	if (dev == Null) {																			// Failed for some unknown reason?
		return STATUS_CONTROL_FAILED;															// Yes
	}
	
	return FsControlDevice(dev, cmd, ibuf, obuf);												// Redirect to the FsControlDevice function
}

static Status DevFsMapFile(PFsNode file, UIntPtr off, UIntPtr addr, UInt32 prot) {
	if (file == Null) {																			// Any null pointer?
		return STATUS_INVALID_ARG;																// Yes, so we can't continue
	} else if ((file->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {									// ... We're trying to map an directory?
		return STATUS_NOT_FILE;																	// Yes (Why?)
	}
	
	PDevice dev = FsGetDevice(file->name);														// Get the device using the name
	
	if (dev == Null) {																			// Failed for some unknown reason?
		return STATUS_MAPFILE_FAILED;															// Yes
	} else if (dev->map == Null) {																// Do we have the map function?
		return STATUS_MAPFILE_FAILED;															// No, we don't...
	}
	
	return dev->map(dev, off, addr, prot);														// Redirect
}

static Status DevFsSyncFile(PFsNode file, UIntPtr off, UIntPtr start, UIntPtr size) {
	if (file == Null) {																			// Any null pointer?
		return STATUS_INVALID_ARG;																// Yes, so we can't continue
	} else if ((file->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {									// ... We're trying to map an directory?
		return STATUS_NOT_FILE;																	// Yes (Why?)
	}
	
	PDevice dev = FsGetDevice(file->name);														// Get the device using the name
	
	if (dev == Null) {																			// Failed for some unknown reason?
		return STATUS_INVALID_ARG;																// Yes
	} else if (dev->sync == Null) {																// Do we have the map function?
		return STATUS_INVALID_ARG;																// No, we don't...
	}
	
	return dev->sync(dev, off, start, size);													// Redirect
}

Void DevFsInit(Void) {
	PWChar path = StrDuplicate(L"/Devices");													// Try to duplicate the path string
	
	if (path == Null) {																			// Failed?
		DbgWriteFormated("PANIC! Couldn't mount /Devices\r\n");									// Yes (halt, as we need DevFs for everything)
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	PWChar type = StrDuplicate(L"DevFs");														// Try to duplicate the type string
	
	if (type == Null) {																			// Failed?
		DbgWriteFormated("PANIC! Couldn't mount /Devices\r\n");									// Yes (same as above)
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	PFsNode root = (PFsNode)MemAllocate(sizeof(FsNode));										// Try to alloc some space for the root directory
	
	if (root == Null) {																			// Failed?
		DbgWriteFormated("PANIC! Couldn't mount /Devices\r\n");									// Yes (same as above)
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	root->name = StrDuplicate(L"/");															// Try to duplicate the root directory string
	
	if (root->name == Null) {																	// Failed?
		DbgWriteFormated("PANIC! Couldn't mount /Devices\r\n");									// Yes (same as above)
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	root->priv = Null;
	root->flags = FS_FLAG_DIR;
	root->inode = (UIntPtr)-1;
	root->length = 0;
	root->offset = 0;
	root->read = Null;
	root->write = Null;
	root->open = DevFsOpenFile;
	root->close = DevFsCloseFile;
	root->readdir = DevFsReadDirectoryEntry;
	root->finddir = DevFsFindInDirectory;
	root->control = Null;
	root->map = Null;
	root->sync = Null;
	
	if (!FsAddMountPoint(path, type, root)) {													// Try to add this device
		DbgWriteFormated("PANIC! Couldn't mount /Devices\r\n");									// Failed (same as above)
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
}
