/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:47 BRT
 * Last edited on July 20 of 2021, at 16:06 BRT */

#include <arch/acpi.hxx>
#include <arch/desctables.hxx>
#include <arch/port.hxx>
#include <sys/arch.hxx>
#include <sys/panic.hxx>
#include <vid/img.hxx>

#define PROCESS_COLOR(Index, Comp) \
    Buffer[Index] = '0' + ((Comp) % 10); \
    Buffer[(Index) - 1] = '0' + (((Comp) /= 10) % 10); \
    Buffer[(Index) - 2] = '0' + (((Comp) / 10) % 10)

using namespace CHicago;

extern "C" Void SmpTrampoline(Void);
extern "C" CoreInfo *SmpTrampolineCoreInfo;

Gdt BspGdt {};

static Void PanicHandler(Registers&) { Arch::Halt(True); }
Void Arch::FinishCore(Void) { AtomicStore(Smp::GetCurrentCore().Status, True); }

Void Arch::InitializeCore(Void) {
    auto &info = **reinterpret_cast<CoreInfo**>(0x8000 + reinterpret_cast<UIntPtr>(&SmpTrampolineCoreInfo) -
                                                         reinterpret_cast<UIntPtr>(SmpTrampoline));

    /* Initialize the FPU (SSE+AVX2) support. */

    UInt16 cw0 = 0x37E, cw1 = 0x37A;
    asm volatile("fninit; fldcw %0; fldcw %1\n"
#ifdef __i386__
                 "xor %%ecx, %%ecx; xgetbv; or $0x07, %%eax; xsetbv\n"
#else
                 "xor %%rcx, %%rcx; xgetbv; or $0x07, %%rax; xsetbv\n"
#endif
                 :: "m"(cw0), "m"(cw1)
#ifdef __i386__
                  : "%eax", "%ecx", "%edx");
#else
                  : "%rax", "%rcx", "%rdx");
#endif

    /* The IDT struct can be used as is, but the GDT needs to be per core (because of the TSS). */

    info.Gdt->Initialize();

#ifdef __i386__
    info.Gdt->LoadSpecialSegment(False, reinterpret_cast<UIntPtr>(&info));
#else
    WriteMsr(0xC0000101, reinterpret_cast<UIntPtr>(&info));
    WriteMsr(0xC0000102, reinterpret_cast<UIntPtr>(&info));
#endif

    IdtReload();
    Smp::SetupLApic();
    asm volatile("sti");
}

Void Arch::Initialize(const BootInfo&) {
    /* Initialize the descriptor tables (GDT and IDT, as they are required for pretty much everything, including
     * gracefully handling exceptions/errors). */

    BspGdt.Initialize();
    Debug.Write("{}initialized the global descriptor table{}\n", SetForeground { 0xFF00FF00 }, RestoreForeground{});

    IdtInit();
    IdtSetHandler(0xDE, PanicHandler);
    Debug.Write("{}initialized the interrupt descriptor table{}\n", SetForeground { 0xFF00FF00 }, RestoreForeground{});
}

Void Arch::WriteDebug(Char Data) {
    Port::OutByte(0x3F8, Data);
}

static Void SetupRGB(UInt32 Color, Char *Buffer) {
    /* Each "entry" should be exactly 3 characters long in the buffer, with the positions always the same. */

    UInt8 a, r, g, b;
    EXTRACT_ARGB(Color, a, r, g, b);
    (Void)a;

    PROCESS_COLOR(9, r);
    PROCESS_COLOR(13, g);
    PROCESS_COLOR(17, b);
}

Void Arch::ClearDebug(UInt32 Color) {
    /* For setting the background and foreground colors (and clearing the debug terminal), we use ANSI escape codes. */

    Char data[] = "\033[48;2;000;000;000m\033[2J\033[H";
    SetupRGB(Color, data);
    for (Char ch : data) WriteDebug(ch);
}

Void Arch::SetDebugBackground(UInt32 Color) {
    Char data[] = "\033[48;2;000;000;000m";
    SetupRGB(Color, data);
    for (Char ch : data) WriteDebug(ch);
}

Void Arch::SetDebugForeground(UInt32 Color) {
    Char data[] = "\033[38;2;000;000;000m";
    SetupRGB(Color, data);
    for (Char ch : data) WriteDebug(ch);
}

UIntPtr Arch::GetCoreId(Void) {
    return Smp::GetCoreList().GetLength() <= 1 ? 0 : Smp::GetCurrentCore().Id;
}

Void Arch::EnterPanicState(Void) {
    /* Halt all the other processors but ourselves (as we got a kernel panic/fatal error). */

    static SpinLock lock;
    lock.Acquire();
    Smp::SendIpi(2, 0, 0xFE);
}

no_return Void Arch::Halt(Boolean Full) {
    if (Full) while (True) asm volatile("cli; hlt");
    else while (True) asm volatile("hlt");
}

Void Arch::Sleep(TimeUnit Unit, UInt64 Count) {
    /* HPET has a precision on the femtoseconds range, so we don't really need to worry about it sleeping too little
     * or too much. */

    for (UInt64 dest = Hpet::GetRegister(0xF0) + Count *
                       (Unit == TimeUnit::Nanoseconds ? 1000000 : (Unit == TimeUnit::Microseconds ? 1000000000 :
                       (Unit == TimeUnit::Milliseconds ? 1000000000000 : 1000000000000000))) / Hpet::GetFrequency();
         Hpet::GetRegister(0xF0) < dest;) ARCH_PAUSE();
}

UInt64 Arch::GetUpTime(TimeUnit Unit) {
    return (Hpet::GetRegister(0xF0) * Hpet::GetFrequency()) /
           (Unit == TimeUnit::Nanoseconds ? 1000000 : (Unit == TimeUnit::Microseconds ? 1000000000 :
           (Unit == TimeUnit::Milliseconds ? 1000000000000 : 1000000000000000)));
}
