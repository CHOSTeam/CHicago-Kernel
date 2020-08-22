/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 07 of 2020, at 12:05 BRT
 * Last edited on August 11 of 2020, at 17:08 BRT */

#ifndef __CHICAGO_DISPLAY_HXX__
#define __CHICAGO_DISPLAY_HXX__

#include <chicago/img.hxx>
#include <chicago/string.hxx>

class Display {
public:
	/* For now, our display interface is pretty simple, it just have a SetResolution function, later we should expand it
	 * to have at least 2D acceleration functions, and a way to get the supported functions. */

	struct packed Mode {
		UIntPtr Width, Height;
		Void *Buffer;
	};

	struct packed Impl {
		Status (*SetResolution)(UIntPtr, UIntPtr, Mode*);
	};
	
	/* The video driver should call the Register function, to indicate to us the current mode (so we can allocate the
	 * frontbuffer, and to set all the internal function pointers. */
	
	static Status Register(Impl &Funcs, const List<Mode> &SupportedModes, Mode &CurrentMode);
	
	/* While our *Int functions do most of the job, we should check if they even exists, and we should also update our
	 * internal variables after functions like SetResInt, so let's make wrapper functions around them. */
	
	static Status SetResolution(UIntPtr Width, UIntPtr Height);
	
	/* We should have a way to access the backbuffer and the frontbuffer, as I don't want to make the variables themselves
	 * public, let's make some accessors... */
	 
	static Void Update(Void) { StrCopyMemory32(CurrentMode.Buffer, FrontBuffer.GetBuffer(),
											   CurrentMode.Width * CurrentMode.Height); }
	
	static Void *GetBackBuffer(Void) { return CurrentMode.Buffer; }
	static Image &GetFrontBuffer(Void) { return FrontBuffer; }
	static UIntPtr GetWidth(Void) { return CurrentMode.Width; }
	static UIntPtr GetHeight(Void) { return CurrentMode.Height; }
private:
	static Mode CurrentMode;
	static List<Mode> SupportedModes;
	static Image &FrontBuffer;
	static Impl Funcs;
};

#endif
