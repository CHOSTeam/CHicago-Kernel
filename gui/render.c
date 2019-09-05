// File author is Ítalo Lima Marconato Matias
//
// Created on September 04 of 2019, at 18:29 BRT
// Last edited on September 05 of 2019, at 14:45 BRT

#include <chicago/alloc.h>
#include <chicago/debug.h>
#include <chicago/display.h>
#include <chicago/gui.h>
#include <chicago/ipc.h>
#include <chicago/mm.h>
#include <chicago/panic.h>
#include <chicago/string.h>

UIntPtr GuiDirectory = 0;
PImage GuiDefaultTheme = Null;
Boolean GuiInitialized = False;
Lock GuiRefreshLock = { False, Null };
Volatile Boolean GuiShouldRefresh = True;
List GuiWindowList = { Null, Null, 0, False, True };

static Void GuiBitBlit(UIntPtr sx, UIntPtr sy, UIntPtr dx, UIntPtr dy, UIntPtr w, UIntPtr h) {
	DispBitBlit(GuiDefaultTheme, sx, sy, dx, dy, w, h, BITBLIT_MODE_COPY);																// Just redirect to DispBitBlit
}

static Void GuiBitBlitRescale(UIntPtr sx, UIntPtr sy, UIntPtr sw, UIntPtr sh, UIntPtr dx, UIntPtr dy, UIntPtr dw, UIntPtr dh) {
	PImage img = ImgCreate(sw, sh, GuiDefaultTheme->bpp);																				// Try to create the img struct
	
	if (img == Null) {
		return;
	}
	
	ImgBitBlit(img, GuiDefaultTheme, sx, sy, 0, 0, sw, sh, BITBLIT_MODE_COPY);															// Copy the image
	
	PImage res = ImgRescale(img, dw, dh);																								// Try to rescale
	
	if (res == Null) {
		MemFree((UIntPtr)img);																											// Failed...
	}
	
	DispBitBlit(res, 0, 0, dx, dy, dw, dh, BITBLIT_MODE_COPY);																			// Call DispBitBlit
	MemFree((UIntPtr)res);																												// And free everything
	MemFree((UIntPtr)img);
}

static Void GuiRenderThread(Void) {
	while (True) {																														// Enter the render loop!
		while (!GuiShouldRefresh) {																										// Wait until we need to do something
			PsSwitchTask(Null);
		}
		
		PsLock(&GuiRefreshLock);																										// Lock
		DispFillRectangle(0, 0, DispGetWidth(), DispGetHeight(), 0xFF000000);															// Clear the screen
		
		ListForeach(&GuiWindowList, i) {																								// Let's draw all the windows
			PGuiWindowInt wint = (PGuiWindowInt)i->data;																				// Get the internal window struct
			UIntPtr oldd = MmGetCurrentDirectory();																						// The current directory
			PGuiWindow window = wint->window;																							// The window
			UIntPtr newd = wint->dir;																									// And the directory that we are going to switch into
			
			if (oldd != newd) {																											// Switch the directory
				MmSwitchDirectory(newd);
			}
			
			DispFillRectangle(window->x, window->y, window->w, window->h, 0xFFEBEBE4);													// Draw the background
			GuiBitBlit(0, 0, window->x, window->y, 4, 23);																				// Draw the top left corner of the frame
			GuiBitBlit(8, 0, window->x + window->w - 4, window->y, 4, 23);																// Draw the top right corner of the frame
			GuiBitBlit(0, 29, window->x, window->y + window->h - 4, 4, 4);																// Draw the bottom left corner of the frame
			GuiBitBlit(8, 29, window->x + window->w - 4, window->y + window->h - 4, 4, 4);												// Draw the bottom right corner of the frame
			GuiBitBlitRescale(4, 0, 4, 23, window->x + 4, window->y, window->w - 8, 23);												// Draw the top part of the frame
			GuiBitBlitRescale(4, 29, 4, 4, window->x + 4, window->y + window->h - 4, window->w - 8, 4);									// Draw the bottom part of the frame
			GuiBitBlitRescale(0, 23, 4, 6, window->x, window->y + 23, 4, window->h - 27);												// Draw the left part of the frame
			GuiBitBlitRescale(8, 23, 4, 6, window->x + window->w - 4, window->y + 23, 4, window->h - 27);								// Draw the right part of the frame
			
			if (oldd != newd) {																											// Switch back to our directory
				MmSwitchDirectory(oldd);
			}
		}
		
		GuiShouldRefresh = False;																										// Done!
		
		DispRefresh();																													// Refresh
		PsUnlock(&GuiRefreshLock);																										// Unlock
	}
}

static Boolean GuiGetWindow(PGuiWindow window, PProcess proc, PUIntPtr out) {
	if (window == Null || proc == Null) {																								// Sanity check
		return False;
	}
	
	UIntPtr idx = 0;																													// Now, let's iterate the window list
	
	ListForeach(&GuiWindowList, i) {
		PGuiWindowInt wint = (PGuiWindowInt)i->data;																					// Get the wint struct
		
		if (wint->window == window && wint->proc == proc) {																				// Check if it is what we are searching
			if (out != Null) {																											// Yes, it is! Save the idx if we need to
				*out = idx;
			}
			
			return True;
		}
		
		idx++;
	}
	
	return False;
}

static Boolean GuiGetWindowProc(PProcess proc, PUIntPtr out) {
	if (proc == Null) {																													// Sanity check
		return False;
	}
	
	UIntPtr idx = 0;																													// Now, let's iterate the window list
	
	ListForeach(&GuiWindowList, i) {
		if (((PGuiWindowInt)i->data)->proc == proc) {																					// Check if it is what we are searching
			if (out != Null) {																											// Yes, it is! Save the idx if we need to
				*out = idx;
			}
			
			return True;
		}
		
		idx++;
	}
	
	return False;
}

static Void GuiInitThread(Void) {
	GuiDirectory = MmGetCurrentDirectory();																								// Save the directory of this process
	
	if ((GuiDefaultTheme = ImgLoadBMPBuf(GuiDefaultThemeImage)) == Null) {																// Let's init/load the default theme
		PsCurrentProcess->id = 0;																										// Failed, set this process id to 0 and panic (a bit hacky lol)
		DbgWriteFormated("PANIC! Couldn't init the GUI\r\n");
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	PIpcPort port = IpcCreatePort(L"Gui");																								// Create the IPC request port
	
	if (port == Null) {
		PsCurrentProcess->id = 0;																										// Failed, set this process id to 0 and panic (a bit hacky lol)
		DbgWriteFormated("PANIC! Couldn't init the GUI\r\n");
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	PThread th = PsCreateThread((UIntPtr)GuiRenderThread, 0, False);																	// Create the render thread
	
	if (th == Null) {
		PsCurrentProcess->id = 0;																										// Failed, set this process id to 0 and panic (a bit hacky lol)
		DbgWriteFormated("PANIC! Couldn't init the GUI\r\n");
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	PsAddThread(th);																													// Add the thread
	
	GuiInitialized = True;																												// We're ok now!
	
	while (True) {																														// Let's handle the incoming requests (IPC)
		PIpcMessage msg = IpcReceiveMessage(L"Gui");																					// Wait until we receive something
		
		if (msg == Null) {																												// First, sanity check
			continue;
		}
		
		PGuiWindowInt wint = (PGuiWindowInt)msg->buffer;																				// Get the wint struct
		
		if (msg->msg == 0) {																											// GuiAddWindow?
			if (GuiGetWindow(wint->window, wint->proc, Null) || !ListAdd(&GuiWindowList, wint)) {										// Yes, check if it isn't already in the window list, after that, try to add it
				MmFreeUserMemory((UIntPtr)wint);																						// ...
			} else {
				GuiRefresh();																											// Refresh!
			}
		} else if (msg->msg == 1) {																										// GuiRemoveWindow?
			UIntPtr idx = 0;																											// Yes, let's get the idx of the window struct in the list
			
			if (GuiGetWindow(wint->window, wint->proc, &idx)) {																			// Try to get the index
				goto e1;
			}
			
			PGuiWindowInt wints = ListRemove(&GuiWindowList, idx);																		// Remove it and get the wint struct
			
			if (wints != Null) {
				MmFreeUserMemory((UIntPtr)wints);																						// Free the struct and refresh
				GuiRefresh();
			}
			
e1:			MmFreeUserMemory((UIntPtr)wint);																							// Free the wint struct
		} else if (msg->msg == 2) {																										// GuiRemoveProcess?
			UIntPtr idx = 0;																											// Yes, let's get all the windows from the specified process and remove them
			
			while (GuiGetWindowProc(wint->proc, &idx)) {
				PGuiWindowInt wints = ListRemove(&GuiWindowList, idx);																	// Remove and get the wint struct

				if (wints != Null) {
					MmFreeUserMemory((UIntPtr)wints);																					// Free the struct
				}
			}
			
			MmFreeUserMemory((UIntPtr)wint);																							// Free the wint struct
			GuiRefresh();																												// Refresh!
		} else {
			MmFreeUserMemory((UIntPtr)wint);																							// Free the wint struct
		}
		
		MmFreeUserMemory((UIntPtr)msg);																									// Free the msg header
	}
}

static PGuiWindowInt GuiCreateWint(PGuiWindow window, PProcess proc, UIntPtr dir) {
	PGuiWindowInt wint = (PGuiWindowInt)MemAllocate(sizeof(GuiWindowInt));																// Alloc memory for the wint struct
	
	if (wint == Null) {
		return Null;
	}
	
	wint->window = window;																												// Setup everything
	wint->proc = proc;
	wint->dir = dir;
	
	return wint;
}

Void GuiAddWindow(PGuiWindow window) {
	if (window == Null) {																												// Sanity check
		return;
	}
	
	while (!GuiInitialized) {																											// Wait until we can do something
		PsSwitchTask(Null);
	}
	
	PGuiWindowInt wint = GuiCreateWint(window, PsCurrentProcess, MmGetCurrentDirectory());												// Create the wint struct
	
	if (wint == Null) {
		return;																															// Failed...
	}
		
	IpcSendMessage(L"Gui", 0, sizeof(GuiWindowInt), (PUInt8)wint);																		// Send the message
	MemFree((UIntPtr)wint);																												// And free that wint struct (we don't need it anymore)
}

Void GuiRemoveWindow(PGuiWindow window) {
	if (window == Null) {																												// Sanity check
		return;
	}
	
	while (!GuiInitialized) {																											// Wait until we can do something
		PsSwitchTask(Null);
	}
	
	PGuiWindowInt wint = GuiCreateWint(window, PsCurrentProcess, 0);																	// Create the wint struct
	
	if (wint == Null) {
		return;																															// Failed...
	}
		
	IpcSendMessage(L"Gui", 1, sizeof(GuiWindowInt), (PUInt8)wint);																		// Send the message
	MemFree((UIntPtr)wint);																												// And free that wint struct (we don't need it anymore)
}

Void GuiRemoveProcess(PProcess proc) {
	if (proc == Null) {																													// Sanity check
		return;
	}
	
	while (!GuiInitialized) {																											// Wait until we can do something
		PsSwitchTask(Null);
	}
	
	PGuiWindowInt wint = GuiCreateWint(Null, proc, 0);																					// Create the wint struct
	
	if (wint == Null) {
		return;																															// Failed...
	}
		
	IpcSendMessage(L"Gui", 2, sizeof(GuiWindowInt), (PUInt8)wint);																		// Send the message
	MemFree((UIntPtr)wint);																												// And free that wint struct (we don't need it anymore)
}

Void GuiRefresh(Void) {
	PsLock(&GuiRefreshLock);																											// Lock
	GuiShouldRefresh = True;																											// Set the refresh flag
	PsUnlock(&GuiRefreshLock);																											// Unlock
}

Void GuiInit(Void) {
	PProcess proc = PsCreateProcess(L"CHicagoGui", (UIntPtr)GuiInitThread);																// Create the process that will init the renderer and control everything else
	
	if (proc == Null) {
		DbgWriteFormated("PANIC! Couldn't init the GUI\r\n");																			// Failed...
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	PsAddProcess(proc);																													// Add it
	GuiAddWindow(GuiCreateWindow(Null, 100, 100, 400, 400));																			// Create a test window
}
