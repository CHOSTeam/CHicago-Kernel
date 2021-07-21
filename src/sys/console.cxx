/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 08 of 2021, at 00:14 BRT
 * Last edited on July 20 of 2021, at 15:40 BRT */

#include <vid/console.hxx>

namespace CHicago {

DebugConsole Debug;

DebugConsole::DebugConsole(const BootInfo &Info, UInt32 Background, UInt32 Foreground)
    : Back(reinterpret_cast<UInt32*>(Info.FrameBuffer.BackBuffer), Info.FrameBuffer.Width, Info.FrameBuffer.Height),
      Front(reinterpret_cast<UInt32*>(Info.FrameBuffer.FrontBuffer), Info.FrameBuffer.Width, Info.FrameBuffer.Height),
      Initialized(True), Background(Background), Foreground(Foreground) { Clear(); }

Void DebugConsole::Clear(Void) {
    Lock.Acquire();

    if (Front.GetBuffer() != Null) {
        Back.Clear(Background);
        Front.Clear(Background);
    }

    X = BackY = FrontY = BackgroundSP = ForegroundSP = 0;
    Arch::ClearDebug(Background);
    Lock.Release();
}

Void DebugConsole::DisableGraphics(Void) {
    Lock.Acquire();

    if (Front.GetBuffer() != Null) {
        Back.Clear(0);
        Front.Clear(0);
        Back = Front = {};
    }

    Lock.Release();
}

Boolean DebugConsole::WriteInt(Char Data) {
    /* Handle both overflow on the X axis (move into the next line) and overflow on the Y axis (scroll the screen). */

    if (!Data) return Initialized;

    Arch::WriteDebug(Data);

    if (Front.GetBuffer() == Null) return True;
    else if (X + DefaultFont.GlyphInfo[static_cast<UInt8>(Data)].Advance > Back.GetWidth())
        FrontY += DefaultFont.Height, BackY += DefaultFont.Height, X = 0;

    if (BackY + DefaultFont.Height > Back.GetHeight()) {
        /* FrontY controls our scrolling, as it contains the current position that this function can write to (before
         * copying the written data into the framebuffer), when we scroll, we have to copy everything from start to
         * end of the writable section into the beginning of the framebuffer (remembering that as we are a ring buffer,
         * it may start in the middle of the buffer, and end at the start, and for that we need two different
         * CopyMemory() calls). */

        if (FrontY + DefaultFont.Height > Back.GetHeight()) FrontY = 0;

        UIntPtr size = Back.GetHeight() / DefaultFont.Height;

        X = 0;
        BackY = (size - 1) * DefaultFont.Height;

        CopyMemory(Back.GetBuffer(), &Front.GetBuffer()[(FrontY + DefaultFont.Height) * Back.GetWidth()],
                   (Back.GetHeight() - FrontY - DefaultFont.Height) * Back.GetWidth() * 4);
        CopyMemory(&Back.GetBuffer()[(size - (FrontY / DefaultFont.Height) - 1) *
                   DefaultFont.Height * Back.GetWidth()], Front.GetBuffer(), FrontY * Back.GetWidth() * 4);

        Front.DrawRectangle(0, FrontY, Back.GetWidth(), DefaultFont.Height, Background, True);
        Back.DrawRectangle(0, BackY, Back.GetWidth(), DefaultFont.Height, Background, True);
    }

    switch (Data) {
    case '\n': BackY += DefaultFont.Height; FrontY += DefaultFont.Height;
    case '\r': X = 0; return True;
    default:
        if (!Front.DrawCharacter(X, FrontY, Data, Foreground)) return False;
        break;
    }

    /* Copy the data from the double buffer into the main framebuffer (this is so that we don't need to read the video
     * memory, while blending the pixels). */

    UInt32 *bpos = &Back.GetBuffer()[BackY * Back.GetWidth() + X],
           *fpos = &Front.GetBuffer()[FrontY * Back.GetWidth() + X];

    X += DefaultFont.GlyphInfo[static_cast<UInt8>(Data)].Advance;

    for (UInt16 i = 0; i < DefaultFont.Height; i++, bpos += Back.GetWidth(), fpos += Back.GetWidth())
        CopyMemory(bpos, fpos, DefaultFont.GlyphInfo[static_cast<UInt8>(Data)].Advance * 4);

    return True;
}

Void DebugConsole::SetBackgroundInt(UInt32 Color) {
    if (BackgroundSP < sizeof(BackgroundStack) / sizeof(UInt32)) BackgroundStack[BackgroundSP++] = Background;
    Background = Color;
    Arch::SetDebugBackground(Color);
}

Void DebugConsole::SetForegroundInt(UInt32 Color) {
    if (ForegroundSP < sizeof(ForegroundStack) / sizeof(UInt32)) ForegroundStack[ForegroundSP++] = Foreground;
    Foreground = Color;
    Arch::SetDebugForeground(Color);
}

Void DebugConsole::RestoreBackgroundInt(Void) {
    if (BackgroundSP > 0) Background = BackgroundStack[--BackgroundSP], Arch::SetDebugBackground(Background);
}

Void DebugConsole::RestoreForegroundInt(Void) {
    if (ForegroundSP > 0) Foreground = ForegroundStack[--ForegroundSP], Arch::SetDebugForeground(Foreground);
}

}
