/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 21 of 2020, at 12:06 BRT
 * Last edited on October 09 of 2020, at 20:47 BRT */

#include <chicago/arch/arch.hxx>
#include <chicago/arch/boot.hxx>
#include <chicago/arch/desctables.hxx>
#include <chicago/arch/mm.hxx>
#include <chicago/display.hxx>
#include <chicago/siafs.hxx>
#include <chicago/textout.hxx>

/* Some (arch-specific) variables that we don't have anywhere else to put. */

BootInfo *ArchBootInfo = Null;
static Image ArchFrameBuffer;

extern "C" Void KernelEntry(BootInfo *Info) {
	/* Hello, World! The loader just exited the EFI environment, and gave control to the kernel!
	 * We should be on protected/long mode (protected for 32-bits, long for 64-bits), with paging
	 * enabled (the page directory should contain the kernel, all the boot info, the framebuffer,
	 * and the EFI entry point, as it was required for not breaking everything), and already running
	 * on higher-half (0xC0000000 on x86-32 and 0xFFFF800000000000 on x86-64). Before anything, let's
	 * call the _init function, this function runs all the global constructors and things like that. */
	
	_init();
	
	/* Before even thinking about jumping to the kernel main function, we should initialize all the
	 * interfaces that it expects us to init before calling it (or actually, let's init the arch and
	 * the debug interfaces, check if we were really booted by our OS loader, or by something that gave
	 * us the right boot info struct, and then finish initializing the interfaces). */
	
	InitArchInterface();
	InitDebugInterface();
	
#ifdef DBG
	Debug->Write("[System] CHicago Operating System for %s version %s\n", ARCH, VERSION);
	
	if (sizeof(UIntPtr) < sizeof(UInt64)) {
		Debug->Write("[System] The system UIntPtr size is not at least 64-bits, file sizes and some other");
		Debug->Write(" numbers may be truncated to the UIntPtr size (%d-bits)\n", sizeof(UIntPtr) * 8);
	}
#endif
	
	ArchBootInfo = Info;
	
	if (Info->Magic != BOOT_INFO_MAGIC) {
		Arch->Halt();
	}
	
	/* Unmapping the efi entry on x86-32 is just a matter of clearing the present bit on the PD entry, and,
	 * on x86-64, while we also need to unset the present bit, the way to get to the PD entry is different,
	 * as we need to traverse PML4->PDP->PD. */
	
#ifdef ARCH_64
	((UInt64*)0xFFFFFFFFC0000000 +
			  (((Info->EfiMainAddress & ~0xFFF) >> 18) & 0x3FFFF000))[(Info->EfiMainAddress >> 21) & 0x1FF] &= ~0x01;
#else
	((UInt32*)0xFFFFF000)[(Info->EfiMainAddress & ~0xFFF) >> 22] &= ~0x01;
#endif
	
	asm volatile("invlpg (%0)" :: "r"(Info->EfiMainAddress & ~0xFFF) : "memory");
	
	/* And also let's initialize the description tables (GDT and IDT), to make sure that exceptions will be
	 * handled correctly, we just exited the EFI environment, and we're with both GDT and IDT from the EFI firmware. */
	
	GdtInit();
	IdtInit();
	
#ifdef DBG
	Debug->Write("[System] Initialized the x86 description tables\n");
#endif
	
	/* Initialize the memory manager, without it, we can't even init the filesystem manager.
	 * The two pieces that we have to do anything are the physical memory manager (initialize the memory size, max
	 * addressable physical address and all of the avaliable page regions) and the heap (pre-alloc the PD/PML4 for the heap,
	 * and set the start and end points of the heap). */
	
	PmmInit();
	
#ifdef DBG
	Debug->Write("[PhysMem] Initialized the physical memory manager\n");
	Debug->Write("          The max addressable physical location is 0x%x (usable memory size is %BB)\n",
				 PhysMem::GetMaxAddress(), PhysMem::GetSize());
	Debug->Write("          %BB are currently free (%BB are being used)\n", PhysMem::GetFree(), PhysMem::GetUsage());
#endif
	
	VmmInit();
	
#ifdef DBG
	Debug->Write("[VirtMem] Initialized the virtual memory manager\n");
	Debug->Write("          The heap starts at 0x%x and ends at 0x%x (size is %BB)\n",
				 Heap::GetStart(), Heap::GetEnd(), Heap::GetSize());
#endif

	/* Initialize our default display interface: the framebuffer. We need to init it after initializing the memory manager,
	 * as the Display::Register needs to allocate the front buffer. */
	
	Display::Mode mode = { Info->FrameBuffer.Width, Info->FrameBuffer.Height, (Void*)Info->FrameBuffer.Address };
	Display::Impl funcs = { Null };
	
	if (Display::Register(funcs, List<Display::Mode>(), mode) != Status::Success) {
		Debug->Write("PANIC! Failed to initialize the default display interface\n");
		Arch->Halt();
	}
	
	StrSetMemory32(mode.Buffer, 0, mode.Width * mode.Height);
	
#ifdef DBG
	Debug->Write("[Display] Initialized the default display interface\n");
	Debug->Write("          The current resolution is %dx%d\n", Info->FrameBuffer.Width, Info->FrameBuffer.Height);
	Debug->Write("          The backbuffer virtual address is 0x%x\n", Display::GetBackBuffer());
	Debug->Write("          The frontbuffer virtual address is 0x%x\n", Display::GetFrontBuffer().GetBuffer());
#endif
	
	/* Mount the initrd. */
	
	if (SiaFs::MountRamFs("/System/Boot", Info->BootSIA.Contents, Info->BootSIA.Size) != Status::Success) {
		Debug->Write("PANIC! Failed to mount the initrd\n");
		Arch->Halt();
	}
	
	/* Jump into the actual main function of the kernel, if it ever returns, just halt everything. */
	
	KernelMain();
	Arch->Halt();
}
