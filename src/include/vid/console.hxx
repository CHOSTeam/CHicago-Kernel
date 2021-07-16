/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 05 of 2021, at 13:26 BRT
 * Last edited on July 16 of 2021, at 09:47 BRT */

#pragma once

#include <sys/boot.hxx>
#include <util/textout.hxx>
#include <vid/img.hxx>

namespace CHicago {

class TextConsole : public TextOutput {
public:
    TextConsole();
    TextConsole(const BootInfo&, UInt32 = 0, UInt32 = 0xFFFFFFFF);

    Void Clear();

    Void SetBackground(UInt32);
    Void SetForeground(UInt32);
    Void RestoreBackground();
    Void RestoreForeground();

    inline UInt32 GetBackground() const { return Background; }
    inline UInt32 GetForeground() const { return Foreground; }
private:
    Boolean WriteInt(Char) override;

    Image Back, Front;
    UInt16 X, BackY, FrontY;
    UInt32 Background, Foreground, BackgroundSP, ForegroundSP, BackgroundStack[32], ForegroundStack[32];
};

extern TextConsole Debug;

}
