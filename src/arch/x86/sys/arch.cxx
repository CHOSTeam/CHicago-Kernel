/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:47 BRT
 * Last edited on July 16 of 2021, at 16:36 BRT */

#include <arch/acpi.hxx>
#include <arch/desctables.hxx>
#include <sys/arch.hxx>
#include <sys/panic.hxx>

using namespace CHicago;

Void Arch::Initialize(const BootInfo&) {
    /* Initialize the descriptor tables (GDT and IDT, as they are required for pretty much everything, including
     * gracefully handling exceptions/errors).*/

    GdtInit();
    Debug.Write("initialized the global descriptor table\n");

    IdtInit();
    Debug.Write("initialized the interrupt descriptor table\n");
}

no_return Void Arch::Halt(Boolean Full) {
    if (Full) while (True) asm volatile("cli; hlt");
    else while (True) asm volatile("hlt");
}

Void Arch::Sleep(TimeUnit Unit, UInt64 Count) {
    /* HPET has a precision on the femtoseconds range, so we don't really need to worry about it sleeping too little
     * or too much. */

    for (UInt64 dest = Hpet::GetMainCounter() + Count *
                       (Unit == TimeUnit::Nanoseconds ? 1000000 : (Unit == TimeUnit::Microseconds ? 1000000000 :
                       (Unit == TimeUnit::Milliseconds ? 1000000000000 : 1000000000000000))) / Hpet::GetFrequency();
         Hpet::GetMainCounter() < dest;) ;
}

UInt64 Arch::GetUpTime(TimeUnit Unit) {
    return (Hpet::GetMainCounter() * Hpet::GetFrequency()) /
           (Unit == TimeUnit::Nanoseconds ? 1000000 : (Unit == TimeUnit::Microseconds ? 1000000000 :
           (Unit == TimeUnit::Milliseconds ? 1000000000000 : 1000000000000000)));
}
