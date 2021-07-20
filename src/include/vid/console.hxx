/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 05 of 2021, at 13:26 BRT
 * Last edited on July 20 of 2021, at 13:46 BRT */

#pragma once

#include <sys/boot.hxx>
#include <util/textout.hxx>
#include <vid/img.hxx>

namespace CHicago {

class DebugConsole : public TextOutput {
public:
    DebugConsole(Void) = default;
    DebugConsole(const BootInfo&, UInt32 = 0, UInt32 = 0xFFFFFFFF);

    Void Clear(Void);
    Void DisableGraphics(Void);
private:
    Boolean WriteInt(Char) override;
    Void SetBackgroundInt(UInt32) override;
    Void SetForegroundInt(UInt32) override;
    Void RestoreBackgroundInt(Void) override;
    Void RestoreForegroundInt(Void) override;

    Image Back {}, Front {};
    Boolean Initialized = False;
    UInt16 X = 0, BackY = 0, FrontY = 0;
    UInt32 Background = 0, Foreground = 0, BackgroundSP = 0, ForegroundSP = 0, BackgroundStack[32] {},
           ForegroundStack[32] {};
};

extern DebugConsole Debug;

}
