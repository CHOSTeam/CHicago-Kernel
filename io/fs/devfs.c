// File author is Ítalo Lima Marconato Matias
//
// Created on July 16 of 2018, at 18:29 BRT
// Last edited on October 29 of 2019, at 16:56 BRT

#include <chicago/alloc.h>
#include <chicago/debug.h>
#include <chicago/device.h>
#include <chicago/file.h>
#include <chicago/panic.h>
#include <chicago/string.h>

Boolean DevFsControlFile(PFsNode file, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf);

Boolean DevFsReadFile(PFsNode file, UIntPtr off, UIntPtr len, PUInt8 buf) {
	if (file == Null) {																			// Any null pointer?
		return False;																			// Yes, so we can't continue
	} else if ((file->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {									// We're trying to read raw bytes in an... Directory?
		return False;																			// Yes (Why?)
	}
	
	PDevice dev = FsGetDevice(file->name);														// Get the device using the name
	
	if (dev == Null) {																			// Failed for some unknown reason?
		return False;																			// Yes
	}
	
	return FsReadDevice(dev, off, len, buf);													// Redirect to the FsReadDevice function
}

Boolean DevFsWriteFile(PFsNode file, UIntPtr off, UIntPtr len, PUInt8 buf) {
	if (file == Null) {																			// Any null pointer?
		return False;																			// Yes, so we can't continue
	} else if ((file->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {									// We're trying to write raw bytes in an... Directory?
		return False;																			// Yes (Why?)
	}
	
	PDevice dev = FsGetDevice(file->name);														// Get the device using the name
	
	if (dev == Null) {																			// Failed for some unknown reason?
		return False;																			// Yes
	}
	
	return FsWriteDevice(dev, off, len, buf);													// Redirect to the FsWriteDevice function
}

Boolean DevFsOpenFile(PFsNode node) {
	if (node == Null) {																			// Null pointer?
		return False;																			// Yes
	} else {
		return True;
	}
}

Void DevFsCloseFile(PFsNode node) {
	if (node == Null) {																			// Null pointer?
		return;																					// Yes
	} else if (StrCompare(node->name, L"/")) {													// Root directory?
		return;																					// Yes, DON'T FREE IT, NEVER!
	}
	
	MemFree((UIntPtr)node->name);																// Free everything!
	MemFree((UIntPtr)node);
}

PWChar DevFsReadDirectoryEntry(PFsNode dir, UIntPtr entry) {
	if (dir == Null) {																			// Any null pointer?
		return Null;																			// Yes, so we can't continue
	} else if ((dir->flags & FS_FLAG_DIR) != FS_FLAG_DIR) {										// We're trying to do ReadDirectoryEntry in an... File?
		return Null;																			// Yes (Why?)
	} else if (entry == 0) {																	// Current directory?
		return StrDuplicate(L".");
	} else if (entry == 1) {																	// Parent directory?
		return StrDuplicate(L"..");
	} else {
		PDevice dev = FsGetDeviceByID(entry - 2);												// Get the device by ID (using the entry)
		
		if (dev == Null) {																		// Found?
			return Null;																		// Nope
		}
		
		return StrDuplicate(dev->name);															// Return the device's name
	}
}

PFsNode DevFsFindInDirectory(PFsNode dir, PWChar name) {
	if ((dir == Null) || (name == Null)) {														// Any null pointer?
		return Null;																			// Yes, so we can't continue
	} else if ((dir->flags & FS_FLAG_DIR) != FS_FLAG_DIR) {										// We're trying to do FindInDirectory in an... File?
		return Null;																			// Yes (Why?)
	}
	
	PDevice dev = FsGetDevice(name);															// Try to get the device by the name
	
	if (dev == Null) {																			// Failed?
		return Null;																			// Yes
	}
	
	PFsNode node = (PFsNode)MemAllocate(sizeof(FsNode));										// Alloc the fs node struct
	
	if (node == Null) {																			// Failed?
		return Null;																			// Yes
	}
	
	node->name = StrDuplicate(dev->name);														// Duplicate the name
	
	if (node->name == Null) {																	// Failed?
		MemFree((UIntPtr)node);																	// Yes, so free everything
		return Null;																			// And return
	}
	
	node->priv = Null;
	node->flags = FS_FLAG_FILE;
	node->inode = FsGetDeviceID(name);															// Try to get the device idx in the list
	
	if (node->inode == (UIntPtr)-1) {															// Failed?
		MemFree((UIntPtr)node->name);															// Yes, so free everything
		MemFree((UIntPtr)node);
		return Null;																			// And return
	}
	
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
	
	return node;
}

Boolean DevFsControlFile(PFsNode file, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf) {
	if (file == Null) {																			// Any null pointer?
		return False;																			// Yes, so we can't continue
	} else if ((file->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {									// ... We're trying to use the control in an directory?
		return False;																			// Yes (Why?)
	}
	
	PDevice dev = FsGetDevice(file->name);														// Get the device using the name
	
	if (dev == Null) {																			// Failed for some unknown reason?
		return False;																			// Yes
	}
	
	return FsControlDevice(dev, cmd, ibuf, obuf);												// Redirect to the FsControlDevice function
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
	
	if (!FsAddMountPoint(path, type, root)) {													// Try to add this device
		DbgWriteFormated("PANIC! Couldn't mount /Devices\r\n");									// Failed (same as above)
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
}
