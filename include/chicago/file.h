// File author is Ítalo Lima Marconato Matias
//
// Created on July 16 of 2018, at 18:18 BRT
// Last edited on January 20 of 2020, at 22:01 BRT

#ifndef __CHICAGO_FILE_H__
#define __CHICAGO_FILE_H__

#include <chicago/device.h>
#include <chicago/list.h>

#define FS_FLAG_DIR 0x01
#define FS_FLAG_FILE 0x02
#define FS_FLAG_READ 0x04
#define FS_FLAG_WRITE 0x08

#define OPEN_FLAGS_DIR 0x01
#define OPEN_FLAGS_READ 0x02
#define OPEN_FLAGS_WRITE 0x04
#define OPEN_FLAGS_CREATE 0x08
#define OPEN_FLAGS_ONLY_CREATE 0x10
#define OPEN_FLAGS_DONT_CREATE_DIRS 0x20
#define OPEN_FLAGS_APPEND 0x40

typedef struct FsNodeStruct {
	PWChar name;
	PVoid priv;
	UIntPtr flags;
	UIntPtr inode;
	UIntPtr length;
	UIntPtr offset;
	Status (*read)(struct FsNodeStruct *, UIntPtr, UIntPtr, PUInt8, PUIntPtr);
	Status (*write)(struct FsNodeStruct *, UIntPtr, UIntPtr, PUInt8, PUIntPtr);
	Boolean (*open)(struct FsNodeStruct *);
	Void (*close)(struct FsNodeStruct *);
	Status (*readdir)(struct FsNodeStruct *, UIntPtr, PWChar *);
	Status (*finddir)(struct FsNodeStruct *, PWChar, struct FsNodeStruct **);
	Status (*create)(struct FsNodeStruct *, PWChar, UIntPtr);
	Status (*control)(struct FsNodeStruct *, UIntPtr, PUInt8, PUInt8);
} FsNode, *PFsNode;

typedef struct {
	PWChar path;
	PWChar type;
	PFsNode root;
} FsMountPoint, *PFsMountPoint;

typedef struct {
	PWChar name;
	Boolean (*probe)(PFsNode);
	Status (*mount)(PFsNode, PWChar, PFsMountPoint *);
	Boolean (*umount)(PFsMountPoint);
} FsType, *PFsType;

Void DevFsInit(Void);
Void Iso9660Init(Void);

PList FsTokenizePath(PWChar path);
PWChar FsCanonicalizePath(PWChar path);
PWChar FsJoinPath(PWChar src, PWChar incr);
PWChar FsGetRandomPath(PWChar prefix);
Status FsReadFile(PFsNode file, UIntPtr off, UIntPtr len, PUInt8 buf, PUIntPtr read);
Status FsWriteFile(PFsNode file, UIntPtr off, UIntPtr len, PUInt8 buf, PUIntPtr write);
Status FsOpenFile(PWChar path, UIntPtr flags, PFsNode *ret);
Status FsCloseFile(PFsNode node);
Status FsMountFile(PWChar path, PWChar file, PWChar type);
Status FsUmountFile(PWChar path);
Status FsReadDirectoryEntry(PFsNode dir, UIntPtr entry, PWChar *ret);
Status FsFindInDirectory(PFsNode dir, PWChar name, PFsNode *ret);
Status FsControlFile(PFsNode file, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf);
PFsMountPoint FsGetMountPoint(PWChar path, PWChar *outp);
PFsType FsGetType(PWChar type);
Boolean FsAddMountPoint(PWChar path, PWChar type, PFsNode root);
Boolean FsRemoveMountPoint(PWChar path);
Boolean FsAddType(PWChar name, Boolean (*probe)(PFsNode), Status (*mount)(PFsNode, PWChar, PFsMountPoint*), Boolean (*umount)(PFsMountPoint));
Boolean FsRemoveType(PWChar name);
Void FsInit(Void);

#endif		// __CHICAGO_FILE_H__
