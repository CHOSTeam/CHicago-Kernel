/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 08 of 2021, at 00:14 BRT
 * Last edited on February 09 of 2021 at 18:40 BRT */

#include <textout.hxx>

TextConsole::TextConsole(Void) : Back(), Front(), X(0), Y(0), Background(0), Foreground(0) { }

TextConsole::TextConsole(BootInfo &Info, UInt32 Background, UInt32 Foreground)
    : Back(reinterpret_cast<UInt32*>(Info.FrameBuffer.BackBuffer), Info.FrameBuffer.Width, Info.FrameBuffer.Height),
      Front(reinterpret_cast<UInt32*>(Info.FrameBuffer.FrontBuffer), Info.FrameBuffer.Width, Info.FrameBuffer.Height),
      X(0), Y(0), Background(Background), Foreground(Foreground), BackgroundSP(0), ForegroundSP(0) {
    Clear();
    Update();
    SetMemory(BackgroundStack, 0, sizeof(BackgroundStack));
    SetMemory(ForegroundStack, 0, sizeof(ForegroundStack));
}

Void TextConsole::Clear(Void) {
    Front.Clear(Background);
}

Void TextConsole::Update(Void) {
    CopyMemory32(Back.GetBuffer(), Front.GetBuffer(), Front.GetWidth() * Front.GetHeight());
}

Void TextConsole::SetBackground(UInt32 Color) {
    /* We have to push the current background color to our internal stack, so later the RestoreBackground function can
     * properly restore it. */

    if (BackgroundSP < sizeof(BackgroundStack) / sizeof(UInt32)) {
        BackgroundStack[BackgroundSP++] = Background;
    }

    Background = Color;
}

Void TextConsole::SetForeground(UInt32 Color) {
    if (ForegroundSP < sizeof(ForegroundStack) / sizeof(UInt32)) {
        ForegroundStack[ForegroundSP++] = Foreground;
    }

    Foreground = Color;
}

Void TextConsole::RestoreBackground(Void) {
    /* Let's just not do anything (instead of setting to some default value) if the stack is empty. */

    if (BackgroundSP > 0) {
        Background = BackgroundStack[--BackgroundSP];
    }
}

Void TextConsole::RestoreForeground(Void) {
    if (ForegroundSP > 0) {
        Foreground = ForegroundStack[--ForegroundSP];
    }
}

Void TextConsole::AfterWrite(Void) {
    Update();
}

Boolean TextConsole::WriteInt(Char Data) {
    /* Handle both overflow on the X axis (move into the next line) and overflow on the Y axis (scroll the screen). */

    if (!Data) {
        return Front.GetBuffer() != Null;
    }

    if (X + DefaultFont.GlyphInfo[(UInt8)Data].Advance > Front.GetWidth()) {
        Y += DefaultFont.Height;
        X = 0;
    }

    if (Y + DefaultFont.Height > Front.GetHeight()) {
        Front.Scroll(DefaultFont.Height, Background);
        Y = ((Front.GetHeight() / DefaultFont.Height) - 1) * DefaultFont.Height;
        X = 0;
    }

    switch (Data) {
    case '\n': {
        Y += DefaultFont.Height;
        X = 0;
        return True;
    }
    case '\r': {
        X = 0;
        return True;
    }
    default: {
        if (!Front.DrawCharacter(X, Y, Data, Foreground)) {
            return False;
        }

        break;
    }
    }

    X += DefaultFont.GlyphInfo[(UInt8)Data].Advance;

    return True;
}
