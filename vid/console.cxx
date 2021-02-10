/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 08 of 2021, at 00:14 BRT
 * Last edited on February 10 of 2021 at 11:17 BRT */

#include <textout.hxx>

TextConsole::TextConsole(Void)
    : Back(), Front(), X(0), Y(0), UXStart(0), UXEnd(0), UYStart(0), UYEnd(0), Background(0), Foreground(0) { }

TextConsole::TextConsole(BootInfo &Info, UInt32 Background, UInt32 Foreground)
    : Back(reinterpret_cast<UInt32*>(Info.FrameBuffer.BackBuffer), Info.FrameBuffer.Width, Info.FrameBuffer.Height),
      Front(reinterpret_cast<UInt32*>(Info.FrameBuffer.FrontBuffer), Info.FrameBuffer.Width, Info.FrameBuffer.Height),
      HasUpdate(False), X(0), Y(0), UXStart(0), UXEnd(0), UYStart(0), UYEnd(0), Background(Background),
      Foreground(Foreground), BackgroundSP(0), ForegroundSP(0) {
    Clear();
    SetMemory(BackgroundStack, 0, sizeof(BackgroundStack));
    SetMemory(ForegroundStack, 0, sizeof(ForegroundStack));
}

Void TextConsole::Clear(Void) {
    Back.Clear(Background);
    Front.Clear(Background);
    UXStart = UXEnd = UYStart = UYEnd = 0;
    HasUpdate = False;
}

Void TextConsole::Update(Void) {
    /* There is no need to redraw the whole screen everytime (unless we just scrolled), this is why we have the UX/UY
     * variables (UpdateX and UpdateY), they indicate which region of the screen has to be updated. */

    if (!HasUpdate) {
        return;
    } else if (UXStart != UXEnd && UYStart != UYEnd) {
        UIntPtr start = UYStart * Front.GetWidth() + UXStart;
        UInt32 *bpos = &Back.GetBuffer()[start], *fpos = &Front.GetBuffer()[start];

        for (; UYStart < UYEnd; UYStart++, bpos += Front.GetWidth(), fpos += Front.GetWidth()) {
            CopyMemory(bpos, fpos, (UXEnd - UXStart) * 4);
        }
    }

    UXStart = UXEnd = UYStart = UYEnd = 0;
    HasUpdate = False;
}

Void TextConsole::SetBackground(UInt32 Color) {
    /* We have to push the current background color to our internal stack, so later the RestoreBackground function can
     * properly restore it. */

    if (BackgroundSP < sizeof(BackgroundStack) / sizeof(UInt32)) {
        BackgroundStack[BackgroundSP++] = Background;
    }

    Background = Color;

    Clear();
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

    Clear();
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

        /* Scrolling invalidates the whole screen, so let's set the update region to be 0;0 - w;h. */

        UXStart = UYStart = 0;
        UXEnd = Front.GetWidth() - 1;
        UYEnd = Front.GetHeight() - 1;

        Y = ((Front.GetHeight() / DefaultFont.Height) - 1) * DefaultFont.Height;
        X = 0;
    } else {
        /* Setup our update X/Y start values. */

        if (X < UXStart) {
            UXStart = X;
        }

        if (Y < UYStart) {
            UYStart = Y;
        }
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

    /* At the end, set our update X/Y end values. Also, if we reached this place, we did write something to the screen
     * (probably), so we can set that we want to update. */

    if (X > UXEnd) {
        UXEnd = X;
    }

    if (Y + DefaultFont.Height > UYEnd) {
        UYEnd = Y + DefaultFont.Height;
    }

    return (HasUpdate = True);
}
