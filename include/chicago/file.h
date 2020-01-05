// File author is Ítalo Lima Marconato Matias
//
// Created on July 16 of 2018, at 18:18 BRT
// Last edited on January 04 of 2020, at 17:58 BRT

#ifndef __CHICAGO_FILE_H__
#define __CHICAGO_FILE_H__

#include <chicago/device.h>
#include <chicago/list.h>

#define FS_FLAG_DIR 0x01
#define FS_FLAG_FILE 0x02

typedef struct FsNodeStruct {
	PWChar name;
	PVoid priv;
	UIntPtr flags;
	UIntPtr inode;
	UIntPtr length;
	UIntPtr offset;
	UIntPtr (*read)(struct FsNodeStruct *, UIntPtr, UIntPtr, PUInt8);
	UIntPtr (*write)(struct FsNodeStruct *, UIntPtr, UIntPtr, PUInt8);
	Boolean (*open)(struct FsNodeStruct *);
	Void (*close)(struct FsNodeStruct *);
	PWChar (*readdir)(struct FsNodeStruct *, UIntPtr);
	struct FsNodeStruct *(*finddir)(struct FsNodeStruct *, PWChar);
	Boolean (*create)(struct FsNodeStruct *, PWChar, UIntPtr);
	Boolean (*control)(struct FsNodeStruct *, UIntPtr, PUInt8, PUInt8);
} FsNode, *PFsNode;

typedef struct {
	PWChar path;
	PWChar type;
	PFsNode root;
} FsMountPoint, *PFsMountPoint;

typedef struct {
	PWChar name;
	Boolean (*probe)(PFsNode);
	PFsMountPoint (*mount)(PFsNode, PWChar);
	Boolean (*umount)(PFsMountPoint);
} FsType, *PFsType;

Void DevFsInit(Void);
Void Iso9660Init(Void);

PList FsTokenizePath(PWChar path);
PWChar FsCanonicalizePath(PWChar path);
PWChar FsJoinPath(PWChar src, PWChar incr);
PWChar FsGetRandomPath(PWChar prefix);
UIntPtr FsReadFile(PFsNode file, UIntPtr off, UIntPtr len, PUInt8 buf);
UIntPtr FsWriteFile(PFsNode file, UIntPtr off, UIntPtr len, PUInt8 buf);
PFsNode FsOpenFile(PWChar path);
Void FsCloseFile(PFsNode node);
Boolean FsMountFile(PWChar path, PWChar file, PWChar type);
Boolean FsUmountFile(PWChar path);
PWChar FsReadDirectoryEntry(PFsNode dir, UIntPtr entry);
PFsNode FsFindInDirectory(PFsNode dir, PWChar name);
Boolean FsCreateFile(PFsNode dir, PWChar name, UIntPtr type);
Boolean FsControlFile(PFsNode file, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf);
PFsMountPoint FsGetMountPoint(PWChar path, PWChar *outp);
PFsType FsGetType(PWChar type);
Boolean FsAddMountPoint(PWChar path, PWChar type, PFsNode root);
Boolean FsRemoveMountPoint(PWChar path);
Boolean FsAddType(PWChar name, Boolean (*probe)(PFsNode), PFsMountPoint (*mount)(PFsNode, PWChar), Boolean (*umount)(PFsMountPoint));
Boolean FsRemoveType(PWChar name);
Void FsInit(Void);

#endif		// __CHICAGO_FILE_H__
