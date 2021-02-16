/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 26 of 2020, at 13:16 BRT
 * Last edited on February 15 of 2021, at 11:03 BRT */

#pragma once

#include <boot.hxx>
#include <img.hxx>

/* To inherit this class you only need to implement the ->WriteInt(Char) function (AfterWrite is empty by default and
 * is not fully virtual, so you don't need to overwrite it if you don't want). */

class TextOutput {
public:
    Void Write(Char);
    Void Write(const String&, ...);
private:
    virtual Void AfterWrite(Void) { }
    virtual Boolean WriteInt(Char) = 0;
};

class TextConsole : public TextOutput {
public:
    TextConsole(Void);
    TextConsole(BootInfo&, UInt32 = 0, UInt32 = 0xFFFFFFFF);

    Void Clear(Void);

    Void SetBackground(UInt32);
    Void SetForeground(UInt32);
    Void RestoreBackground(Void);
    Void RestoreForeground(Void);

    UInt32 GetBackground(Void) const { return Background; }
    UInt32 GetForeground(Void) const { return Foreground; }
private:
    Boolean WriteInt(Char) override;

    Image Back, Front;
    UInt16 X, BackY, FrontY;
    UInt32 Background, Foreground, BackgroundSP, ForegroundSP, BackgroundStack[32], ForegroundStack[32];
};

extern TextConsole Debug;
