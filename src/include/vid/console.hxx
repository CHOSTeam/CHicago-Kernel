/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 05 of 2021, at 13:26 BRT
 * Last edited on July 17 of 2021, at 21:48 BRT */

#pragma once

#include <sys/boot.hxx>
#include <util/textout.hxx>
#include <vid/img.hxx>

namespace CHicago {

class TextConsole : public TextOutput {
public:
    TextConsole(Void) = default;
    TextConsole(const BootInfo&, UInt32 = 0, UInt32 = 0xFFFFFFFF);

    Void Clear(Void);

    inline UInt32 GetBackground(Void) const { return Background; }
    inline UInt32 GetForeground(Void) const { return Foreground; }
private:
    Boolean WriteInt(Char) override;
    Void SetBackgroundInt(UInt32) override;
    Void SetForegroundInt(UInt32) override;
    Void RestoreBackgroundInt(Void) override;
    Void RestoreForegroundInt(Void) override;

    Image Back {}, Front {};
    UInt16 X {}, BackY {}, FrontY {};
    UInt32 Background {}, Foreground {}, BackgroundSP {}, ForegroundSP {}, BackgroundStack[32] {},
           ForegroundStack[32] {};
};

extern TextConsole Debug;

}
