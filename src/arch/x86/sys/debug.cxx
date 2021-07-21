/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 20 of 2021, at 19:49 BRT
 * Last edited on July 20 of 2021, at 20:07 BRT */

#include <arch/port.hxx>
#include <sys/arch.hxx>
#include <vid/img.hxx>

using namespace CHicago;

static Boolean Initialized = False;

#define PROCESS_COLOR(Index, Comp) \
    Buffer[Index] = '0' + ((Comp) % 10); \
    Buffer[(Index) - 1] = '0' + (((Comp) /= 10) % 10); \
    Buffer[(Index) - 2] = '0' + (((Comp) / 10) % 10)

static Void SetupRGB(UInt32 Color, Char *Buffer) {
    /* Each "entry" should be exactly 3 characters long in the buffer, with the positions always the same. */

    UInt8 a, r, g, b;
    EXTRACT_ARGB(Color, a, r, g, b);
    (Void)a;

    PROCESS_COLOR(9, r);
    PROCESS_COLOR(13, g);
    PROCESS_COLOR(17, b);
}

Void Arch::InitializeDebug(Void) {
    /* Initialize and test the serial port (COM1), we don't even want to try using it if it's unusable. */

    Port::OutByte(0x3F9, 0x00);
    Port::OutByte(0x3FB, 0x80);
    Port::OutByte(0x3F8, 0x00);
    Port::OutByte(0x3F9, 0x00);
    Port::OutByte(0x3FB, 0x03);
    Port::OutByte(0x3FA, 0xC7);
    Port::OutByte(0x3FC, 0x0B);
    Port::OutByte(0x3FC, 0x1E);
    Port::OutByte(0x3F8, 0xAE);

    if (Port::InByte(0x3F8) == 0xAE) {
        Port::OutByte(0x3FC, 0x0F);
        Initialized = True;
    }
}

Void Arch::WriteDebug(Char Data) {
    if (!Initialized) return;
    while (!(Port::InByte(0x3FD) & 0x20)) ARCH_PAUSE();
    Port::OutByte(0x3F8, Data);
}

Void Arch::ClearDebug(UInt32 Color) {
    /* For setting the background and foreground colors (and clearing the debug terminal), we use ANSI escape codes. */

    if (!Initialized) return;
    Char data[] = "\033[48;2;000;000;000m\033[2J\033[H";
    SetupRGB(Color, data);
    for (Char ch : data) WriteDebug(ch);
}

Void Arch::SetDebugBackground(UInt32 Color) {
    if (!Initialized) return;
    Char data[] = "\033[48;2;000;000;000m";
    SetupRGB(Color, data);
    for (Char ch : data) WriteDebug(ch);
}

Void Arch::SetDebugForeground(UInt32 Color) {
    if (!Initialized) return;
    Char data[] = "\033[38;2;000;000;000m";
    SetupRGB(Color, data);
    for (Char ch : data) WriteDebug(ch);
}
