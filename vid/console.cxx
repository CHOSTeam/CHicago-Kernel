/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 08 of 2021, at 00:14 BRT
 * Last edited on February 08 of 2021 at 11:08 BRT */

#include <textout.hxx>

TextConsole::TextConsole(Void) : Screen(), X(0), Y(0), Background(0), Foreground(0) { }

TextConsole::TextConsole(BootInfo &Info, UInt32 Background, UInt32 Foreground)
    : Screen(reinterpret_cast<UInt32*>(Info.FrameBuffer.Address), Info.FrameBuffer.Width, Info.FrameBuffer.Height),
      X(0), Y(0), Background(Background), Foreground(Foreground), BackgroundSP(0), ForegroundSP(0) {
    Clear();
    SetMemory(BackgroundStack, 0, sizeof(BackgroundStack));
    SetMemory(ForegroundStack, 0, sizeof(ForegroundStack));
}

Void TextConsole::Clear(Void) {
    Screen.Clear(Background);
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
 
Boolean TextConsole::WriteInt(Char Data) {
    /* Handle both overflow on the X axis (move into the next line) and overflow on the Y axis (scroll the screen). */

    if (!Data) {
        return Screen.GetBuffer() != Null;
    }

    if (X + DefaultFont.GlyphInfo[Data].Advance > Screen.GetWidth()) {
        Y += DefaultFont.Height;
        X = 0;
    }

    if (Y + DefaultFont.Height > Screen.GetHeight()) {
        Screen.Scroll(DefaultFont.Height, Background);
        Y = ((Screen.GetHeight() / DefaultFont.Height) - 1) * DefaultFont.Height;
        X = 0;
    }

    switch (Data) {
    case '\n': {
        Y += DefaultFont.Height;
    }
    case '\r': {
        X = 0;
        return True;
    }
    default: {
        if (!Screen.DrawCharacter(X, Y, Data, Foreground)) {
            return False;
        }

        break;
    }
    }

    X += DefaultFont.GlyphInfo[Data].Advance;

    return True;
}
