/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:22 BRT
 * Last edited on July 20 of 2021, at 16:06 BRT */

#include <sys/arch.hxx>
#include <sys/mm.hxx>
#include <sys/panic.hxx>

using namespace CHicago;

extern "C" Void SmpEntry(Void) {
    /* The "caller" is probably expecting us to set some kind of arch-specific signal to say that we initialized and
     * reached this place, so let's call the arch-specific init core/cpu function. */

    Arch::InitializeCore();
    Debug.Write("{}core {} is alive{}\n", SetForeground { 0xFF00FF00 }, Arch::GetCoreId(), RestoreForeground{});

    Arch::FinishCore();
    Arch::Halt(False);
}

extern "C" Void KernelEntry(const BootInfo &Info) {
    /* Hello, World! The loader just exited the EFI environment, and gave control to the kernel! Right now the MMU
     * should be enabled, with everything we need (boot info struct, framebuffer, boot image file, etc) already mapped,
     * and we should be running on higher-half (probably). Before anything, let's call the _init function, this
     * function runs all the global constructors and things like that. */

    if (Info.Magic != BOOT_INFO_MAGIC) Arch::Halt(True);

#ifdef USE_INIT_ARRAY
    for (UIntPtr *i = &__init_array_start; i < &__init_array_end; i++) reinterpret_cast<Void(*)()>(*i)();
#else
    _init();
#endif

    /* Initialize the debug interface (change this later to also possibly not use the screen). */

    Image back(reinterpret_cast<UInt32*>(Info.FrameBuffer.BackBuffer), Info.FrameBuffer.Width, Info.FrameBuffer.Height),
          front(reinterpret_cast<UInt32*>(Info.FrameBuffer.FrontBuffer), Info.FrameBuffer.Width,
                                          Info.FrameBuffer.Height);
    UIntPtr w, h;

    Debug = DebugConsole(Info, 0, 0xFFFFFF00);
    Debug.Write("{}initializing the kernel, arch = {}, version = {}\ncore 0 is alive{}\n",
                SetForeground { 0xFF00FF00 }, ARCH, VERSION, RestoreForeground{});

    /* Initialize the arch-specific bits. */

    Arch::Initialize(Info);

    /* Initialize some important early system bits (backtrace symbol resolver, memory manager, etc). */

    StackTrace::Initialize(Info);
    PhysMem::Initialize(Info);
    VirtMem::Initialize(Info);

    /* Initialize/map all the ACPI tables that we need for now. */

    Acpi::Initialize(Info);

    /* By now we should have the timer setup, so we can take over the debug console (on the graphics frontend), and
     * start displaying other things to the screen. */

    Debug.DisableGraphics();
    Image::GetStringSize(w, h, "Starting CHicago...");
    front.DrawString((front.GetWidth() - w) / 2, front.GetHeight() - h, 0xFFFFFFFF, "Starting CHicago...");
    CopyMemory(back.GetBuffer(), front.GetBuffer(), back.GetWidth() * back.GetHeight() * 4);
    Debug.Write("disabled graphical debug output and displayed the boot screen\n");

    Arch::Halt(False);
}
