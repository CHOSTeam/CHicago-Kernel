/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 07 of 2020, at 12:50 BRT
 * Last edited on October 09 of 2020, at 22:13 BRT */

#include <chicago/display.hxx>
#include <chicago/textout.hxx>

/* As always, we need to init all the static fields of the class... */

Display::Mode Display::CurrentMode = { 0, 0, Null };
List<Display::Mode> Display::SupportedModes;
static Image Default;
Image &Display::FrontBuffer = Default;
Display::Impl Display::Funcs = { Null };

Status Display::Register(Display::Impl &Funcs, const List<Mode> &SupportedModes, Display::Mode &CurrentMode) {
	/* Before actually registering the new internal functions, we should try allocating the new frontbuffer,
	 * if it doesn't fail, we can proceed by registering everything, and copying the supplied CurrentMode. */
	
	UInt32 *nfb = new UInt32[CurrentMode.Width * CurrentMode.Height];
	
	if (nfb == Null) {
		return Status::OutOfMemory;
	}
	
	StrCopyMemory(&Display::Funcs, &Funcs, sizeof(Display::Impl));
	StrCopyMemory(&Display::CurrentMode, &CurrentMode, sizeof(Display::Mode));
	
	/* Let's hope that the SupportedModes list is copied succefully, else, we may have some problems later...
	 * That is, the change resolution screen are going to be empty lol. */
	
	Display::SupportedModes.Clear();
	Display::SupportedModes.Add(SupportedModes);
	
	FrontBuffer = Image(CurrentMode.Width, CurrentMode.Height, nfb, True);
	
	return Status::Success;
}

Status Display::SetResolution(UIntPtr Width, UIntPtr Height) {
	/* If the SetResolution func is Null, the display driver doesn't support changing the resolution, in the
	 * case it is supported, we still don't have a lot to do, we only need to create a new frontbuffer, we
	 * don't need to notify any process (YET). */
	
	if (Funcs.SetResolution == Null) {
		return Status::Unsupported;
	} else if (CurrentMode.Width == Width && CurrentMode.Height == Height) {
		return Status::Success;
	}
	
	/* Let's check if the resolution that we specified is even supported. */
	
	for (const Mode &mode : SupportedModes) {
		if (mode.Width == Width && mode.Height == Height) {
			goto c;
		}
	}
	
	return Status::Unsupported;
	
	/* Let's allocate the new frontbuffer already, is easier to just delete it if something goes wrong, instead
	 * of setting the old video mode back if the allocation was to fail later. */
	
c:	UInt32 *nfb = new UInt32[Width * Height];
	Status status = Status::OutOfMemory;
	
	if (nfb == Null) {
		return status;
	} else if ((status = Funcs.SetResolution(Width, Height, &CurrentMode)) != Status::Success) {
		delete nfb;
		return status;
	}
	
	FrontBuffer = Image(Width, Height, nfb, True);
	
	return Status::Success;
}
