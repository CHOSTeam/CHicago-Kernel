// File author is Ítalo Lima Marconato Matias
//
// Created on September 04 of 2019, at 18:29 BRT
// Last edited on September 05 of 2019, at 19:32 BRT

#include <chicago/alloc.h>
#include <chicago/debug.h>
#include <chicago/display.h>
#include <chicago/gui.h>
#include <chicago/ipc.h>
#include <chicago/mm.h>
#include <chicago/panic.h>
#include <chicago/string.h>

extern PImage DispBackBuffer;

IntPtr GuiMouseX = 0;
IntPtr GuiMouseY = 0;
UIntPtr GuiDraggingX = 0;
UIntPtr GuiDraggingY = 0;
UInt8 GuiMouseButtons = 0;
PImage GuiDefaultTheme = Null;
Boolean GuiInitialized = False;
PGuiWindowInt GuiDragging = Null;
Lock GuiRefreshLock = { False, Null };
Volatile Boolean GuiShouldRefresh = True;
List GuiWindowList = { Null, Null, 0, False, True };

static Void GuiWriteString(UIntPtr x, UIntPtr y, UIntPtr bg, UIntPtr fg, PWChar data) {
	ImgWriteStringPixel(DispBackBuffer, &x, &y, bg, fg, data);																			// Just redirectory to ImgWriteStringPixel
}

static Void GuiBitBlit(UIntPtr sx, UIntPtr sy, UIntPtr dx, UIntPtr dy, UIntPtr w, UIntPtr h) {
	DispBitBlit(GuiDefaultTheme, sx, sy, dx, dy, w, h, BITBLIT_MODE_COPY);																// Just redirect to DispBitBlit
}

static Void GuiBitBlitTransparent(UIntPtr sx, UIntPtr sy, UIntPtr dx, UIntPtr dy, UIntPtr w, UIntPtr h) {
	for (UIntPtr y = 0; y < h; y++) {																									// Yeah, we need to do it manually :(
		for (UIntPtr x = 0; x < w; x++) {
			UIntPtr pixel = ImgGetPixel(GuiDefaultTheme, sx + x, sy + y);																// Get the pixel
			
			if (pixel != 0xFF010101) {																									// Transparent pixel?
				DispPutPixel(dx + x, dy + y, pixel);																					// Nope :)
			}
		}
	}
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

static Boolean GuiCheckCollision(UIntPtr x1, UIntPtr y1, UIntPtr x2, UIntPtr y2, UIntPtr w, UIntPtr h) {
	return (x1 >= x2) && (x1 < x2 + w) && (y1 >= y2 && y1 < y2 + h);																	// Boundary/collision check
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
			Boolean top = i->next == Null;																								// Save if this window is in the top
			UIntPtr newd = wint->dir;																									// And the directory that we are going to switch into
			UIntPtr fg;
			
			if (oldd != newd) {																											// Switch the directory
				MmSwitchDirectory(newd);
			}
			
			DispFillRectangle(window->x, window->y, window->w, window->h, 0xFFEBEBE4);													// Yes, draw the background
			
			if (top) {																													// Now, let's draw the frame, this one is in the top?
				GuiBitBlit(0, 0, window->x, window->y, 4, 23);																			// Yes!
				GuiBitBlit(8, 0, window->x + window->w - 4, window->y, 4, 23);
				GuiBitBlit(0, 29, window->x, window->y + window->h - 4, 4, 4);
				GuiBitBlit(8, 29, window->x + window->w - 4, window->y + window->h - 4, 4, 4);
				GuiBitBlitRescale(4, 0, 4, 23, window->x + 4, window->y, window->w - 8, 23);
				GuiBitBlitRescale(4, 29, 4, 4, window->x + 4, window->y + window->h - 4, window->w - 8, 4);
				GuiBitBlitRescale(0, 23, 4, 6, window->x, window->y + 23, 4, window->h - 27);
				GuiBitBlitRescale(8, 23, 4, 6, window->x + window->w - 4, window->y + 23, 4, window->h - 27);
				fg = ImgGetPixel(GuiDefaultTheme, 4, 23);
			} else {
				GuiBitBlit(12, 0, window->x, window->y, 4, 23);																			// Nope :(
				GuiBitBlit(20, 0, window->x + window->w - 4, window->y, 4, 23);
				GuiBitBlit(12, 29, window->x, window->y + window->h - 4, 4, 4);
				GuiBitBlit(20, 29, window->x + window->w - 4, window->y + window->h - 4, 4, 4);
				GuiBitBlitRescale(16, 0, 4, 23, window->x + 4, window->y, window->w - 8, 23);
				GuiBitBlitRescale(16, 29, 4, 4, window->x + 4, window->y + window->h - 4, window->w - 8, 4);
				GuiBitBlitRescale(12, 23, 4, 6, window->x, window->y + 23, 4, window->h - 27);
				GuiBitBlitRescale(20, 23, 4, 6, window->x + window->w - 4, window->y + 23, 4, window->h - 27);
				fg = ImgGetPixel(GuiDefaultTheme, 16, 23);
			}
			
			GuiWriteString(window->x + 4, window->y + 3, DispGetPixel(window->x + 4, window->y + 1), fg, window->title);				// Draw the title
			
			if (oldd != newd) {																											// Switch back to our directory
				MmSwitchDirectory(oldd);
			}
		}
		
		GuiBitBlitTransparent(24, 0, GuiMouseX, GuiMouseY, 12, 20);																		// Draw the mouse cursor
		
		GuiShouldRefresh = False;																										// Done!
		
		DispRefresh();																													// Refresh
		PsUnlock(&GuiRefreshLock);																										// Unlock
	}
}

static Void GuiMouseThread(Void) {
	MousePacket packet;																													// This is where the mouse info will go
	
	while (True) {																														// Let's start!
		RawMouseDeviceRead(&packet);																									// Wait until mouse input
		
		GuiMouseX += packet.offx;																										// Update the mouse position
		GuiMouseY += packet.offy;
		
		if (GuiMouseX < 0) {																											// Fix any underflow
			GuiMouseX = 0;
		}
		
		if (GuiMouseY < 0) {
			GuiMouseY = 0;
		}
		
		if ((UIntPtr)GuiMouseX >= DispGetWidth()) {																						// And any overflow
			GuiMouseX = DispGetWidth() - 1;
		}
		
		if ((UIntPtr)GuiMouseY >= DispGetHeight()) {
			GuiMouseY = DispGetHeight() - 1;
		}
		
		if ((GuiMouseButtons & 0x01) != 0x01 && (packet.buttons & 0x01) == 0x01) {														// Left button press?
			for (IntPtr i = GuiWindowList.length > 0 ? (GuiWindowList.length - 1) : 0; i >= 0; i--) {									// Yes, let's set the new top window!
				PGuiWindowInt wint = (PGuiWindowInt)ListGet(&GuiWindowList, i);															// Get the internal window struct
				UIntPtr oldd = MmGetCurrentDirectory();																					// The current directory
				PGuiWindow window = wint->window;																						// The window
				UIntPtr newd = wint->dir;																								// And the directory that we are going to switch into
				Boolean top = False;

				if (oldd != newd) {																										// Switch the directory
					MmSwitchDirectory(newd);
				}
				
				if (GuiCheckCollision(GuiMouseX, GuiMouseY, window->x, window->y, window->w, window->h)) {								// Check if this is the new top one
					top = True;
					
					if (GuiCheckCollision(GuiMouseX, GuiMouseY, window->x, window->y, window->w + 8, 23)) {								// Left mouse press on the title bar?
						GuiDraggingX = GuiMouseX - window->x;																			// Yes, so now we are dragging this window!
						GuiDraggingY = GuiMouseY - window->y;
						
						GuiDragging = wint;
					}
				}
				
				if (oldd != newd) {																										// Switch back to our directory
					MmSwitchDirectory(oldd);
				}
				
				if (top) {																												// Set this as the new top one?
					if (ListRemove(&GuiWindowList, i)) {																				// Yes, let's try to remove it
						ListAdd(&GuiWindowList, wint);																					// And add it again
					}
					
					break;
				}
			}
		} else if ((packet.buttons & 0x01) != 0x01) {																					// Left button release?
			GuiDragging = Null;																											// Yes, so we aren't dragging anything anymore
		}
		
		if (GuiDragging != Null) {																										// Update the position of the window that we are dragging?
			UIntPtr oldd = MmGetCurrentDirectory();																						// Yes, get the current directory
			PGuiWindow window = GuiDragging->window;																					// The window
			UIntPtr newd = GuiDragging->dir;																							// And the directory that we are going to switch into
			
			if (oldd != newd) {																											// Switch the directory
				MmSwitchDirectory(newd);
			}
			
			if (((IntPtr)GuiMouseX - (IntPtr)GuiDraggingX) < 0) {																		// Let's update! Underflow at X?
				window->x = 0;																											// Yeah :(
			} else {
				window->x = GuiMouseX - GuiDraggingX;																					// Nope :)
			}
			
			if (((IntPtr)GuiMouseY - (IntPtr)GuiDraggingY) < 0) {																		// Let's update! Underflow at Y?
				window->y = 0;																											// Yeah :(
			} else {
				window->y = GuiMouseY - GuiDraggingY;																					// Nope :)
			}
			
			if (oldd != newd) {																											// Switch back to our directory
				MmSwitchDirectory(oldd);
			}
		}
		
		GuiMouseButtons = packet.buttons;																								// Save the buttons state
		
		GuiRefresh();																													// And refresh
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
	
	th = PsCreateThread((UIntPtr)GuiMouseThread, 0, False);																				// Create the mouse handle thread
	
	if (th == Null) {
		PsCurrentProcess->id = 0;
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
	
	GuiAddWindow(GuiCreateWindow(Null, 200, 300, 300, 200));																			// Create some test windows
	GuiAddWindow(GuiCreateWindow(Null, 400, 400, 400, 400));
	GuiAddWindow(GuiCreateWindow(Null, 600, 20, 600, 600));
}
