// File author is Ítalo Lima Marconato Matias
//
// Created on December 08 of 2018, at 10:28 BRT
// Last edited on August 26 of 2019, at 19:46 BRT

#include <chicago/alloc.h>
#include <chicago/arch.h>
#include <chicago/console.h>
#include <chicago/debug.h>
#include <chicago/device.h>
#include <chicago/display.h>
#include <chicago/heap.h>
#include <chicago/net.h>
#include <chicago/nls.h>
#include <chicago/panic.h>
#include <chicago/process.h>
#include <chicago/rand.h>
#include <chicago/string.h>
#include <chicago/version.h>

extern PList FsDeviceList;

#define TEST(name) static Int Test ## name(Void)																										// Macros for making and executing the tests
#define CALL_TEST(name) if (Test ## name()) { pass++; } else { fail++; } tests++;

TEST(Allocator0) {
	Int *arr = (Int*)MemAllocate(sizeof(Int));																											// Alloc some memory
	
	if (arr == Null) {
		return False;																																	// Failed :(
	}
	
	*arr = 10;																																			// Assign some value
	
	Int ret = *arr == 10;																																// Check if the value is ok
	
	MemFree((UIntPtr)arr);																																// Free the allocated memory
	
	return ret;
}

TEST(Allocator1) {
	UIntPtr old = 0;																																	// Let's alloc a lot of small blocks!
	
	for (Int i = 0; i < 10000; i++) {
		UIntPtr addr = MemAllocate(sizeof(Int));																										// Alloc some memory
		
		if (addr == 0) {
			return False;																																// Failed :(
		}
		
		if (old != 0 && (addr > old)) {																													// Check if this one is in the same address of the old one
			MemFree(addr);																																// Isn't :(
			return False;
		} else if (old == 0) {																															// Set the 'old' variable if it isn't set
			old = addr;
		}
		
		MemFree(addr);																																	// Free the allocated address
	}
	
	return True;
}

TEST(Allocator2) {
	Int arr[10000];																																		// Let's test allocating one big block, a lot of small blocks, assigning some values, checking them and after all, free everything
	Int **arr2 = (Int**)MemAllocate(sizeof(Int*) * 10000);																								// Alloc some memory
	Int tmp = 0;
	
	if (arr2 == Null) {
		return False;																																	// Failed
	}
	
	for (Int i = 0; i < 10000; i++) {																													// Let's allocate and set the values!
		tmp = (Int)RandGenerate();																														// Generate some aleatory value
		arr[i] = tmp;																																	// Assign to our local array
		arr2[i] = (Int*)MemAllocate(sizeof(Int));																										// Alloc some memory
		
		if (arr2[i] == Null) {																															// Failed?
			for (Int j = 0; j < i; j++) {																												// Yes, free all the small block that we allocated
				MemFree((UIntPtr)arr2[j]);
			}
			
			MemFree((UIntPtr)arr2);																														// And the big block
			
			return False;
		}
		
		*(arr2[i]) = tmp;																																// Now, just assign the value!
	}
	
	for (Int i = 0; i < 10000; i++) {																													// Now let's check everything!
		if (*(arr2[i]) != arr[i]) {																														// Same value?
			for (Int j = i; j < 10000; j++) {																											// Nope, let's free everything :(
				MemFree((UIntPtr)arr2[j]);
			}
			
			MemFree((UIntPtr)arr2);
			
			return False;
		}
		
		MemFree((UIntPtr)arr2[i]);																														// Ok, so let's free this block!
	}
	
	MemFree((UIntPtr)arr2);																																// Free the big block
	
	return True;
}

TEST(Allocator3) {
	Int **arr = (Int**)MemAllocate(sizeof(Int*) * 3000000);																								// Alloc one very big block
	
	if (arr == Null) {
		return False;																																	// Failed ;(
	}
	
	for (Int i = 0; i < 10000; i++) {																													// Now a lot of small block (just like the Allocator2 test)
		arr[i] = (Int*)MemAllocate(sizeof(Int));
		
		if (arr[i] == Null) {
			for (Int j = 0; j < i; j++) {																												// Failed, free everything :(
				MemFree((UIntPtr)arr[j]);
			}
			
			MemFree((UIntPtr)arr);
			
			return False;
		}
	}
	
	for (Int i = 0; i < 10000; i++) {																													// Now, let's free everything
		MemFree((UIntPtr)arr[i]);
	}
	
	MemFree((UIntPtr)arr);																																// Including our 'arr'
	
	UIntPtr new = MemAllocate(50000);																													// Allocate another big block
	MemFree(new);																																		// And free it
	
	return new <= (UIntPtr)arr;																															// Return if the address of the new block was in the address of the old one
}

static UIntPtr BVal(UIntPtr bytes) {
	if (bytes < 1024) {																																	// Less than 1KB?
		return bytes;																																	// Yes, just return in bytes
	} else if (bytes < 1024 * 1024) {																													// Less than 1MB?
		return bytes / 1024;																															// Yes, return in kilobytes
	} else if (bytes < 1024 * 1024 * 1024) {																											// Less than 1GB?
		return bytes / (1024 * 1024);																													// Yes, return in megabytes
	} else {
		return bytes / (1024 * 1024 * 1024);																											// Return in gigabytes
	}
}

static PWChar BStr(UIntPtr bytes) {
	if (bytes < 1024) {																																	// Less than 1KB?
		return L"B";
	} else if (bytes < 1024 * 1024) {																													// Less than 1MB?
		return L"KB";
	} else if (bytes < 1024 * 1024 * 1024) {																											// Less than 1GB?
		return L"MB";
	} else {
		return L"GB";																																	// 1GB+
	}
}

static Boolean ToIPv4(PWChar in, PUInt8 out) {
	UInt32 accum = 0;
	UInt32 segs = 0;
	UInt32 cnt = 0;
	
	while (*in != '\0') {																																// Let's try to parse the IP(v4) address!
		if (*in == '.') {																																// Separator?
			if (cnt == 0) {																																// Yes, we have at least one character?
				return False;																															// Nope, so it's a invalid address...
			} else if (segs == 3) {																														// More than 4 segments?
				return False;																															// Yes, invalid address
			}
			
			out[segs++] = (UInt8)accum;																													// Add this segment
			accum = 0;																																	// And go to the next one
			cnt = 0;
			in++;
			
			continue;
		}
		
		if ((*in < '0') || (*in > '9')) {																												// Valid number?
			return False;																																// Nope
		}
		
		if ((accum = (accum * 10) + (*in - '0')) > 255) {																								// Valid 8-bit number?
			return False;																																// Nope
		}
		
		cnt++;
		in++;
	}
	
	if (segs != 3) {																																	// More than 4 segments?
		return False;																																	// Yes, invalid address
	} else if (cnt == 0) {																																// We have at least one characer?
		return False;																																	// Nope
	}
	
	out[segs++] = (UInt8)accum;																															// Add this last segment!
	
	return True;
}

static Void ShellMain(Void) {
	RandSetSeed(RandGenerateSeed());																													// Init our "random" number generator
	
	PWChar cmd = (PWChar)MemAllocate(1024);																												// Alloc memory for reading the keyboard
	PWChar *argv = (PWChar*)MemAllocate(sizeof(PWChar) * 256);																							// Alloc memory for transforming the cmd into argc/argv
	PWChar cwd = StrDuplicate(L"\\");																													// And alloc memory for our cwd (current working directory)
	
	if ((cmd == Null) || (argv == Null) || (cwd == Null)) {
		DbgWriteFormated("PANIC! Couldn't init the shell\r\n");																							// Failed... PANIC!
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	while (True) {
		ConWriteFormated(L"%fchicago%f@%f%s%r %f%s%r $ ", 0xFFAA5500, 0xFF55FF55, 0xFF5555FF, CHICAGO_CODENAME, 0xFFFF5555, cwd);						// Print the prompt
		ConsoleDeviceReadKeyboard(1023, cmd);																											// Read from the keyboard
		
		PWChar tok = StrTokenize(cmd, L" ");																											// Let's tokenize!
		UIntPtr argc = 0;
		
		while (tok != Null) {
			argv[argc++] = tok;																															// Add this arg
			tok = StrTokenize(Null, L" ");																												// Go to the next one!
		}
		
		if (StrGetLength(argv[0]) == 3 && StrCompare(argv[0], L"cat")) {																				// Print file content
			if (argc == 1) {																															// Show usage?
				ConWriteFormated(NlsGetMessage(NLS_SHELL_USAGE1), argv[0]);																				// Yes
				continue;
			}
			
			PWChar path = argv[1][0] == '\\' ? FsJoinPath(argv[1], Null) : FsJoinPath(cwd, argv[1]);													// Try to "create" the full file path string
			
			if (path == Null) {
				ConWriteFormated(NlsGetMessage(NLS_SHELL_CAT_ERR1));																					// Failed...
				continue;
			}
			
			PFsNode file = FsOpenFile(path);																											// Let's open the file!
			
			MemFree((UIntPtr)path);
			
			if ((file == Null) || ((file->flags & FS_FLAG_FILE) != FS_FLAG_FILE)) {
				ConWriteFormated(NlsGetMessage(NLS_SHELL_CAT_ERR2), argv[1]);																			// ...
				continue;
			}
			
			PUInt8 buf = (PUInt8)MemAllocate(file->length);																								// Let's read the file!
			
			if (buf == Null) {
				ConWriteFormated(NlsGetMessage(NLS_SHELL_CAT_ERR2), argv[1]);																			// ...
				FsCloseFile(file);
				continue;
			} else if (!FsReadFile(file, 0, file->length, buf)) {																						// Read!
				ConWriteFormated(NlsGetMessage(NLS_SHELL_CAT_ERR2), argv[1]);																			// ...
				FsCloseFile(file);
				continue;
			}
			
			ConSetRefresh(False);																														// Disable screen refresh
			
			for (UIntPtr i = 0; i < file->length; i++) {
				if ((buf[i] & 128) && ((i + 1) < file->length)) {																						// UTF-8?
					ConWriteCharacter((WChar)(((buf[i] & 0x1F) << 6) + (buf[i + 1] & 0x3F)));															// Yes :)
					i++;
				} else if (((i + 1) < file->length) && (buf[i] == '\r') && (buf[i + 1] == '\n')) {														// Carriage return + Line feed?
					ConWriteFormated(L"\r\n");																											// Yes :)
					i++;
				} else if (buf[i] == '\n') {																											// Line feed?
					ConWriteFormated(L"\r\n");																											// Yes :)
				} else {
					ConWriteCharacter(buf[i]);																											// Probably it's some normal ASCII character
				}
			}
			
			ConWriteFormated(L"\r\n");
			DispRefresh();																																// Refresh the screen
			ConSetRefresh(True);																														// Enable screen refresh
			MemFree((UIntPtr)buf);																														// Free the buffer
			FsCloseFile(file);																															// And close the file
		} else if (StrGetLength(argv[0]) == 2 && StrCompare(argv[0], L"cd")) {																			// Change directory
			if (argc == 1) {																															// Show usage?
				ConWriteFormated(NlsGetMessage(NLS_SHELL_USAGE1), argv[0]);																				// Yes
				continue;
			}
			
			PWChar new = argv[1][0] == '\\' ? FsJoinPath(argv[1], Null) : FsJoinPath(cwd, argv[1]);														// Try to "create" the new cwd string
			
			if (new == Null) {
				ConWriteFormated(NlsGetMessage(NLS_SHELL_CD_ERR1));																						// Failed...
				continue;
			}
			
			PFsNode file = FsOpenFile(new);																												// Let's see if it exists!
			
			if ((file == Null) || ((file->flags & FS_FLAG_DIR) != FS_FLAG_DIR)) {
				ConWriteFormated(NlsGetMessage(NLS_SHELL_CD_ERR2), argv[1]);																			// ...
				MemFree((UIntPtr)new);
				continue;
			}
			
			ConWriteFormated(L"\r\n");
			FsCloseFile(file);																															// Ok, close the file
			MemFree((UIntPtr)cwd);																														// Free the old cwd string
			cwd = new;																																	// And set the new one!
		} else if (StrGetLength(argv[0]) == 3 && StrCompare(argv[0], L"cls")) {																			// Clear the screen
			ConClearScreen();
		} else if (StrGetLength(argv[0]) == 4 && StrCompare(argv[0], L"echo")) {																		// Print the arguments to the screen
			ConSetRefresh(False);																														// Disable screen refresh
			
			for (UIntPtr i = 1; i < argc; i++) {																										// Print
				ConWriteFormated(L"%s%s%s", i != 1 ? L" " : L"", argv[i], (i + 1) >= argc ? L"\r\n\r\n" : L"");
			}
			
			DispRefresh();																																// Refresh the screen
			ConSetRefresh(True);																														// Enable screen refresh
		} else if (StrGetLength(argv[0]) == 4 && StrCompare(argv[0], L"help")) {																		// Print the avaliable commands
			ConWriteFormated(NlsGetMessage(NLS_SHELL_HELP));
		} else if (StrGetLength(argv[0]) == 4 && StrCompare(argv[0], L"lang")) {																		// Change the system language
			if (argc == 1) {
				ConWriteFormated(NlsGetMessage(NLS_SHELL_LANG_LIST), NlsGetLanguages());																// Just list the languages
			} else if (NlsGetLanguage(argv[1]) == (UIntPtr)-1) {																						// Valid language?
				ConWriteFormated(NlsGetMessage(NLS_SHELL_LANG_INVALID), argv[1]);																		// Nope...
			} else {
				NlsSetLanguage(NlsGetLanguage(argv[1]));																								// Ok, set!
				ConWriteFormated(L"\r\n");
			}
		} else if (StrGetLength(argv[0]) == 2 && StrCompare(argv[0], L"ls")) {																			// List directory
			PWChar path = argc == 1 ? StrDuplicate(cwd) : (argv[1][0] == '\\' ? FsJoinPath(argv[1], Null) : FsJoinPath(cwd, argv[1]));					// Try to get the path
			
			if (path == Null) {
				ConWriteFormated(NlsGetMessage(NLS_SHELL_LS_ERR));																						// ...
				continue;
			}
			
			PFsNode dir = FsOpenFile(path);																												// Try to open the dir
			PWChar cur = Null;
			UIntPtr i = 0;
			
			MemFree((UIntPtr)path);																														// We don't need the path anymore!
			
			if ((dir == Null) || ((dir->flags & FS_FLAG_DIR) != FS_FLAG_DIR)) {																			// Ok?
				ConWriteFormated(NlsGetMessage(NLS_SHELL_LS_ERR));
				continue;
			}
			
			ConSetRefresh(False);																														// Disable screen refresh
			
			while ((cur = FsReadDirectoryEntry(dir, i + 2)) != Null) {																					// Let's GO!
				if (i != 0) {
					ConWriteFormated(L" ");
				}
				
				PFsNode node = FsFindInDirectory(dir, cur);																								// Let's try to get the type of this node!
				
				if ((node != Null) && ((node->flags & FS_FLAG_DIR) == FS_FLAG_DIR)) {																	// Directory?
					ConWriteFormated(L"%f%s%r", 0xFFFF5555, cur);																						// Yes, show it in red!
				} else {
					ConWriteFormated(L"%s", cur);																										// It's a file (or we failed to open it)
				}
				
				if (node != Null) {
					FsCloseFile(node);																													// Close it
				}
				
				MemFree((UIntPtr)cur);																													// Free the current entry
				i++;																																	// And go to the next one!
			}
			
			if (i == 0) {
				ConWriteFormated(L"\r\n");
			} else {
				ConWriteFormated(L"\r\n\r\n");
			}
			
			DispRefresh();																																// Refresh the screen
			ConSetRefresh(True);																														// Enable screen refresh
			FsCloseFile(dir);																															// Close the cwd node
		} else if (StrGetLength(argv[0]) == 5 && StrCompare(argv[0], L"lsdev")) {																		// List the devices
			UIntPtr j = 0;
			
			ListForeach(FsDeviceList, i) {
				ConWriteFormated(L"%d - 0x%x - 0x%x - ", j++, i, i->data);
				ConWriteFormated(L"%s\r\n", ((PDevice)i->data)->name);
			}
			
			ConWriteFormated(L"\r\n");
		} else if (StrGetLength(argv[0]) == 5 && StrCompare(argv[0], L"panic")) {																		// Crash the system
			PsCurrentProcess->id = PsCurrentThread->id = 0;																								// *HACK*
			DbgWriteFormated("PANIC! User requested panic :)\r\n");
			Panic(PANIC_KERNEL_UNEXPECTED_ERROR);
		} else if (StrGetLength(argv[0]) == 4 && StrCompare(argv[0], L"ping")) {
			if (argc < 2) {																																// Show usage?
				ConWriteFormated(NlsGetMessage(NLS_SHELL_SETIP_USAGE), argv[0]);																		// Yes
				continue;
			} else if (argc == 2) {																														// Set the ip of the default device?
				PNetworkDevice dev = NetGetDefaultDevice();																								// Yes!
				
				if (dev == Null) {
					ConWriteFormated(NlsGetMessage(NLS_SHELL_DNDNOTSET));
					continue;
				}
				
				UInt8 ipv4[4] = { 0 };																													// Let's convert the user input!
				
				if (!ToIPv4(argv[1], ipv4)) {
					ConWriteFormated(NlsGetMessage(NLS_SHELL_SETIP_ERR3));																				// Failed...
					continue;
				}
				
				PICMPv4Socket sock = NetAddICMPv4Socket(dev, ipv4, False);																				// Create the socket
				
				if (sock == Null) {
					ConWriteFormated(NlsGetMessage(NLS_SHELL_SETIP_ERR1));																				// Failed...
					continue;
				}

				NetSendICMPv4Socket(sock);																												// Send the REQUEST
				PICMPHeader res = NetReceiveICMPv4Socket(sock);																							// Wait for the REPLY
				
				NetRemoveICMPv4Socket(sock);																											// Remove the socket
				
				if (res == Null) {																														// Good reply?
					ConWriteFormated(NlsGetMessage(NLS_SHELL_PING_REPLY2), argv[1]);																	// Nope :(
				} else {
					ConWriteFormated(NlsGetMessage(NLS_SHELL_PING_REPLY1), argv[1]);																	// Yes :)
					MemFree((UIntPtr)res);
				}
				
				continue;
			}
			
			PWChar path = argv[1][0] == '\\' ? FsJoinPath(argv[1], Null) : FsJoinPath(L"\\Devices\\", argv[1]);											// Try to "create" the full file path string
			
			if (path == Null) {
				ConWriteFormated(NlsGetMessage(NLS_SHELL_SETIP_ERR1));																					// Failed...
				continue;
			}
			
			PFsNode file = FsOpenFile(path);																											// Let's open the file!
			
			MemFree((UIntPtr)path);
			
			if ((file == Null) || ((file->flags & FS_FLAG_FILE) != FS_FLAG_FILE)) {
				ConWriteFormated(NlsGetMessage(NLS_SHELL_SETIP_ERR2), argv[1]);																			// ...
				continue;
			}
			
			PNetworkDevice dev = NetGetDevice(file);																									// Try to get the network device
			
			FsCloseFile(file);
			
			if (dev == Null) {
				ConWriteFormated(NlsGetMessage(NLS_SHELL_SETIP_ERR2), argv[1]);																			// ...
				continue;
			}
			
			UInt8 ipv4[4] = { 0 };																														// Let's convert the user input!
			
			if (!ToIPv4(argv[2], ipv4)) {
				ConWriteFormated(NlsGetMessage(NLS_SHELL_SETIP_ERR3));																					// Failed...
				continue;
			}
			
			PICMPv4Socket sock = NetAddICMPv4Socket(dev, ipv4, False);																					// Create the socket
			
			if (sock == Null) {
				ConWriteFormated(NlsGetMessage(NLS_SHELL_SETIP_ERR1));																					// Failed...
				continue;
			}
			
			NetSendICMPv4Socket(sock);																													// Send the REQUEST
			PICMPHeader res = NetReceiveICMPv4Socket(sock);																								// Wait for the REPLY
			
			NetRemoveICMPv4Socket(sock);																												// Remove the socket
			
			if (res == Null) {																															// Good reply?
				ConWriteFormated(NlsGetMessage(NLS_SHELL_PING_REPLY2), argv[2]);																		// Nope :(
			} else {
				ConWriteFormated(NlsGetMessage(NLS_SHELL_PING_REPLY1), argv[2]);																		// Yes :)
				MemFree((UIntPtr)res);
			}
		} else if (StrGetLength(argv[0]) == 2 && StrCompare(argv[0], L"ps")) {																			// List all the processes
			ConSetRefresh(False);																														// Disable screen refresh
			
			ListForeach(PsProcessList, i) {																												// Print all the processes
				PProcess proc = (PProcess)i->data;
				PWChar name = proc->name == Null ? NlsGetMessage(NLS_SHELL_PS_UNAMED) : proc->name;
				UIntPtr usage = proc->mem_usage + (proc->id == 0 ? HeapGetCurrent() - HeapGetStart() : 0);
				UIntPtr ths = proc->threads->length;
				
				ConWriteFormated(NlsGetMessage(NLS_SHELL_PS), name, proc->id, ths, BVal(usage), BStr(usage), i->next == Null ? L"\r\n" : L"");
			}
			
			DispRefresh();																																// Refresh the screen
			ConSetRefresh(True);																														// Enable screen refresh
		} else if (StrGetLength(argv[0]) == 5 && StrCompare(argv[0], L"setip")) {																		// Set the IP(v4) address
			if (argc < 2) {																																// Show usage?
				ConWriteFormated(NlsGetMessage(NLS_SHELL_SETIP_USAGE), argv[0]);																		// Yes
				continue;
			} else if (argc == 2) {																														// Set the ip of the default device?
				PNetworkDevice dev = NetGetDefaultDevice();																								// Yes!
				
				if (dev == Null) {
					ConWriteFormated(NlsGetMessage(NLS_SHELL_DNDNOTSET));
					continue;
				}
				
				UInt8 newip[4] = { 0 };																													// Let's convert the user input!
				
				if (!ToIPv4(argv[1], newip)) {
					ConWriteFormated(NlsGetMessage(NLS_SHELL_SETIP_ERR3));																				// Failed...
					continue;
				}
				
				StrCopyMemory(dev->ipv4_address, newip, 4);																								// Change the IPv4 address!
				ConWriteFormated(L"\r\n");
				
				continue;
			}
			
			PWChar path = argv[1][0] == '\\' ? FsJoinPath(argv[1], Null) : FsJoinPath(L"\\Devices\\", argv[1]);											// Try to "create" the full file path string
			
			if (path == Null) {
				ConWriteFormated(NlsGetMessage(NLS_SHELL_SETIP_ERR1));																					// Failed...
				continue;
			}
			
			PFsNode file = FsOpenFile(path);																											// Let's open the file!
			
			MemFree((UIntPtr)path);
			
			if ((file == Null) || ((file->flags & FS_FLAG_FILE) != FS_FLAG_FILE)) {
				ConWriteFormated(NlsGetMessage(NLS_SHELL_SETIP_ERR2), argv[1]);																			// ...
				continue;
			}
			
			UInt8 newip[4] = { 0 };																														// Let's convert the user input!
			
			if (!ToIPv4(argv[2], newip)) {
				FsCloseFile(file);																														// Failed...
				ConWriteFormated(NlsGetMessage(NLS_SHELL_SETIP_ERR3));
				continue;
			}
			
			FsControlFile(file, 1, newip, Null);																										// Change the IPv4 address
			FsCloseFile(file);																															// And close the file :)
			ConWriteFormated(L"\r\n");
		} else if (StrGetLength(argv[0]) == 6 && StrCompare(argv[0], L"setnet")) {																		// Set the default network device
			if (argc < 2) {																																// Show usage?
				ConWriteFormated(NlsGetMessage(NLS_SHELL_SETNET_USAGE), argv[0]);																		// Yes
				continue;
			}
			
			PWChar path = argv[1][0] == '\\' ? FsJoinPath(argv[1], Null) : FsJoinPath(L"\\Devices\\", argv[1]);											// Try to "create" the full file path string
			
			if (path == Null) {
				ConWriteFormated(NlsGetMessage(NLS_SHELL_SETIP_ERR1));																					// Failed...
				continue;
			}
			
			PFsNode file = FsOpenFile(path);																											// Let's open the file!
			
			MemFree((UIntPtr)path);
			
			if ((file == Null) || ((file->flags & FS_FLAG_FILE) != FS_FLAG_FILE)) {
				ConWriteFormated(NlsGetMessage(NLS_SHELL_SETIP_ERR2), argv[1]);																			// ...
				continue;
			}
			
			PNetworkDevice dev = NetGetDevice(file);																									// Get the network device
			
			FsCloseFile(file);
			
			if (dev == Null) {
				ConWriteFormated(NlsGetMessage(NLS_SHELL_SETIP_ERR2), argv[1]);																			// ...
				continue;
			}
			
			NetSetDefaultDevice(dev);																													// Set the default device!
			ConWriteFormated(L"\r\n");
		} else if (StrGetLength(argv[0]) == 4 && StrCompare(argv[0], L"test")) {																		// Test the system
			UIntPtr tests = 0;
			UIntPtr pass = 0;
			UIntPtr fail = 0;
			
			CALL_TEST(Allocator0);																														// Run all the tests!
			CALL_TEST(Allocator1);
			CALL_TEST(Allocator2);
			CALL_TEST(Allocator3);
			
			ConWriteFormated(NlsGetMessage(NLS_SHELL_TEST_SUMMARY), tests, pass, fail);																	// Now print the summary
		} else if (StrGetLength(argv[0]) == 3 && StrCompare(argv[0], L"ver")) {																			// Print the system version
			ConSetRefresh(False);																														// Disable screen refresh
			ConWriteFormated(NlsGetMessage(NLS_OS_NAME), CHICAGO_ARCH);																					// Print some system informations
			ConWriteFormated(NlsGetMessage(NLS_OS_CODENAME), CHICAGO_CODENAME);
			ConWriteFormated(NlsGetMessage(NLS_OS_VSTR), CHICAGO_MAJOR, CHICAGO_MINOR, CHICAGO_BUILD);
			DispRefresh();																																// Refresh the screen
			ConSetRefresh(True);																														// Enable screen refresh
		} else if (StrGetLength(argv[0]) == 0) {
			ConWriteFormated(L"\r\n");																													// No command :)
		} else {
			ConWriteFormated(NlsGetMessage(NLS_SHELL_INVALID), argv[0]);																				// Invalid command!
		}
	}
}

Void ShellRun(Void) {
	PProcess proc = PsCreateProcess(L"Shell", (UIntPtr)ShellMain);																						// Create the shell process
	
	if (proc == Null) {
		DbgWriteFormated("PANIC! Couldn't init the shell\r\n");																							// Failed... PANIC!
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	PsAddProcess(proc);																																	// (Try to) add it!
}
