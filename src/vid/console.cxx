/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 08 of 2021, at 00:14 BRT
 * Last edited on July 06 of 2021, at 20:03 BRT */

#include <vid/console.hxx>

namespace CHicago {

TextConsole Debug;

TextConsole::TextConsole() : Back(), Front(), X(0), BackY(0), FrontY(0), Background(0), Foreground(0),
                             BackgroundSP(0), ForegroundSP(0), BackgroundStack(), ForegroundStack() { }

TextConsole::TextConsole(BootInfo &Info, UInt32 Background, UInt32 Foreground)
    : Back(reinterpret_cast<UInt32*>(Info.FrameBuffer.BackBuffer), Info.FrameBuffer.Width, Info.FrameBuffer.Height),
      Front(reinterpret_cast<UInt32*>(Info.FrameBuffer.FrontBuffer), Info.FrameBuffer.Width, Info.FrameBuffer.Height),
      X(0), BackY(0), FrontY(0), Background(Background), Foreground(Foreground), BackgroundSP(0), ForegroundSP(0),
      BackgroundStack(), ForegroundStack() { Clear(); }

Void TextConsole::Clear() {
    Back.Clear(Background);
    Front.Clear(Background);
    X = BackY = FrontY = 0;
}

Void TextConsole::SetBackground(UInt32 Color) {
    /* We have to push the current background color to our internal stack, so later the RestoreBackground function can
     * properly restore it. */

    if (BackgroundSP < sizeof(BackgroundStack) / sizeof(UInt32)) BackgroundStack[BackgroundSP++] = Background;
    Background = Color;
    Clear();
}

Void TextConsole::SetForeground(UInt32 Color) {
    if (ForegroundSP < sizeof(ForegroundStack) / sizeof(UInt32)) ForegroundStack[ForegroundSP++] = Foreground;
    Foreground = Color;
}

Void TextConsole::RestoreBackground() {
    /* Let's just not do anything (instead of setting to some default value) if the stack is empty. */

    if (BackgroundSP > 0) Background = BackgroundStack[--BackgroundSP];
    Clear();
}

Void TextConsole::RestoreForeground() {
    if (ForegroundSP > 0) Foreground = ForegroundStack[--ForegroundSP];
}

Boolean TextConsole::WriteInt(Char Data) {
    /* Handle both overflow on the X axis (move into the next line) and overflow on the Y axis (scroll the screen). */

    if (!Data) return Front.GetBuffer() != Null;

    if (X + DefaultFont.GlyphInfo[(UInt8)Data].Advance > Back.GetWidth())
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

    X += DefaultFont.GlyphInfo[(UInt8)Data].Advance;

    for (UInt16 i = 0; i < DefaultFont.Height; i++, bpos += Back.GetWidth(), fpos += Back.GetWidth())
        CopyMemory(bpos, fpos, DefaultFont.GlyphInfo[(UInt8)Data].Advance * 4);

    return True;
}

}
