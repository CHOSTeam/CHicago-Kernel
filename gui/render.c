// File author is Ítalo Lima Marconato Matias
//
// Created on September 04 of 2019, at 18:29 BRT
// Last edited on September 04 of 2019, at 20:17 BRT

#include <chicago/alloc.h>
#include <chicago/debug.h>
#include <chicago/display.h>
#include <chicago/gui.h>
#include <chicago/mm.h>
#include <chicago/panic.h>
#include <chicago/string.h>

PImage GuiDefaultTheme = Null;
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

static Void GuiRenderProcess(Void) {
	PsLock(&GuiRefreshLock);																											// Lock the refresh lock
	
	while (GuiDefaultTheme == Null) {																									// Let's load the default theme
		GuiDefaultTheme = ImgLoadBMPBuf(GuiDefaultThemeImage);
	}
	
	PsUnlock(&GuiRefreshLock);																											// Unlock the refresh lock
	
	while (True) {																														// Enter the render loop!
		if (!GuiShouldRefresh) {																										// Only do something if we need
			continue;
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

static Boolean GuiGetWindow(PGuiWindow window, PProcess proc, UIntPtr dir, PUIntPtr out) {
	if (window == Null) {																												// Sanity check
		return False;
	}
	
	UIntPtr idx = 0;																													// Now, let's iterate the window list
	
	ListForeach(&GuiWindowList, i) {
		PGuiWindowInt wint = (PGuiWindowInt)i->data;																					// Get the GuiWindowInt
		
		if (wint->window == window && wint->proc == proc && wint->dir == dir) {															// Check if everything is ok
			if (out != Null) {																											// Yes, it is! Save the idx if we need to
				*out = idx;
			}
			
			return True;
		}
		
		idx++;
	}
	
	return False;
}

static Boolean GuiGetProcessWindow(PProcess proc, PUIntPtr out) {
	if (proc == Null) {																													// Sanity check
		return False;
	}
	
	UIntPtr idx = 0;																													// Now, let's iterate the window list
	
	ListForeach(&GuiWindowList, i) {
		PGuiWindowInt wint = (PGuiWindowInt)i->data;																					// Get the GuiWindowInt
		
		if (wint->proc == proc) {																										// Check if everything is ok
			if (out != Null) {																											// Yes, it is! Save the idx if we need to
				*out = idx;
			}
			
			return True;
		}
		
		idx++;
	}
	
	return False;
}

Void GuiAddWindow(PGuiWindow window) {
	if (window == Null || GuiGetWindow(window, PsCurrentProcess, MmGetCurrentDirectory(), Null)) {										// Sanity check and check if this window isn't already added
		return;
	}
	
	PsLock(&GuiRefreshLock);																											// Lock the refresh lock
	PsLockTaskSwitch(old);																												// Lock task switching
	
	UIntPtr oldd = MmGetCurrentDirectory();																								// Save the current page directory
	
	if (oldd != MmKernelDirectory) {																									// Switch to the directory of the gui render
		MmSwitchDirectory(MmKernelDirectory);
	}
	
	PGuiWindowInt wint = (PGuiWindowInt)MmAllocUserMemory(sizeof(GuiWindowInt));														// Alloc the internal window struct
	
	if (wint != Null) {																													// Failed?
		wint->window = window;																											// Nope, setup it!
		wint->proc = PsCurrentProcess;
		wint->dir = oldd;
		
		if (!ListAdd(&GuiWindowList, wint)) {																							// Finally, try to add to the window list
			GuiShouldRefresh = True;
		} else {
			MmFreeUserMemory((UIntPtr)wint);
		}
	}
	
	if (oldd != MmKernelDirectory) {																									// Switch back to the process directory
		MmSwitchDirectory(oldd);
	}
	
	PsUnlockTaskSwitch(old);																											// Unlock task switching
	PsUnlock(&GuiRefreshLock);																											// Unlock the refresh lock
}

Void GuiRemoveWindow(PGuiWindow window) {
	if (window == Null) {																												// Sanity check
		return;
	}
	
	PsLock(&GuiRefreshLock);																											// Lock the refresh lock
	PsLockTaskSwitch(old);																												// Lock task switching
	
	UIntPtr idx = 0;
	UIntPtr oldd = MmGetCurrentDirectory();																								// Save the current page directory
	
	if (oldd != MmKernelDirectory) {																									// Switch to the directory of the gui render
		MmSwitchDirectory(MmKernelDirectory);
	}
	
	if (GuiGetWindow(window, PsCurrentProcess, oldd, &idx)) {																			// Try to get the window index
		PGuiWindowInt wint = ListRemove(&GuiWindowList, idx);																			// Try to remove it!
		
		if (wint != Null) {
			MmFreeUserMemory((UIntPtr)wint);																							// Success!
			GuiShouldRefresh = True;
		}
	}
	
	if (oldd != MmKernelDirectory) {																									// Switch back to the process directory
		MmSwitchDirectory(oldd);
	}
	
	PsUnlockTaskSwitch(old);																											// Unlock task switching
	PsUnlock(&GuiRefreshLock);																											// Unlock the refresh lock
}

Void GuiRemoveProcess(PProcess proc) {
	if (proc == Null) {																													// Sanity check
		return;
	}
	
	PsLock(&GuiRefreshLock);																											// Lock the refresh lock
	PsLockTaskSwitch(old);																												// Lock task switching
	
	UIntPtr idx = 0;
	UIntPtr oldd = MmGetCurrentDirectory();																								// Save the current page directory
	
	if (oldd != MmKernelDirectory) {																									// Switch to the directory of the gui render
		MmSwitchDirectory(MmKernelDirectory);
	}
	
	while (GuiGetProcessWindow(PsCurrentProcess, &idx)) {																				// Let's get all the windows from this process
		PGuiWindowInt wint = ListRemove(&GuiWindowList, idx);																			// And try to remove them!
		
		if (wint != Null) {
			MmFreeUserMemory((UIntPtr)wint);																							// Success!
			GuiShouldRefresh = True;
		}
	}
	
	if (oldd != MmKernelDirectory) {																									// Switch back to the process directory
		MmSwitchDirectory(oldd);
	}
	
	PsUnlockTaskSwitch(old);																											// Unlock task switching
	PsUnlock(&GuiRefreshLock);																											// Unlock the refresh lock
}

Void GuiRefresh(Void) {
	PsLock(&GuiRefreshLock);																											// Lock
	GuiShouldRefresh = True;																											// Set the refresh flag
	PsUnlock(&GuiRefreshLock);																											// Unlock
}

Void GuiInit(Void) {
	PProcess proc = PsCreateProcessInt(L"GuiRender", (UIntPtr)GuiRenderProcess, MmKernelDirectory);										// Create the gui render process
	
	if (proc == Null) {
		DbgWriteFormated("PANIC! Couldn't init the GUI\r\n");																			// Failed...
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	PsAddProcess(proc);																													// Add it
	
	GuiAddWindow(GuiCreateWindow(L"Test", 100, 150, 400, 400));																			// Add a test window
}
