/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 07 of 2021, at 17:37 BRT
 * Last edited on February 22 of 2021 at 17:28 BRT */

#pragma once

#include <string.hxx>

/* Macros to extract and recreate ARGB colors (on both little endian and big endian machines). */

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define EXTRACT_ARGB(c, a, r, g, b) \
    (a) = ((c) >> 24) & 0xFF; \
    (r) = ((c) >> 16) & 0xFF; \
    (g) = ((c) >> 8) & 0xFF; \
    (b) = (c) & 0xFF

#define MAKE_ARGB(a, r, g, b) ((((a) & 0xFF) << 24) | (((r) & 0xFF) << 16) | (((g) & 0xFF) << 8) | ((b) & 0xFF))
#define FIX_ARGB(c) (c)
#else
#define EXTRACT_ARGB(c, a, r, g, b) \
    (a) = (c) & 0xFF; \
    (r) = ((c) >> 8) & 0xFF; \
    (g) = ((c) >> 16) & 0xFF; \
    (b) = ((c) >> 24) & 0xFF

#define MAKE_ARGB(a, r, g, b) (((a) & 0xFF) | (((r) & 0xFF) << 8) | (((g) & 0xFF) << 16) | (((b) & 0xFF) << 24))
#define FIX_ARGB(c) \
    ((c) & 0xFF) << 24) | ((((c) >> 8) & 0xFF) << 16) | ((((c) >> 16) & 0xFF) << 8) | (((c) >> 24) & 0xFF)
#endif

namespace CHicago {

struct FontGlyph {
    Int32 Offset, Width, Height, Left, Top, Advance;
};

struct FontData {
    Int32 Ascender, Descender, Height;
    const FontGlyph *GlyphInfo;
    const UInt8 *GlyphData;
};

extern FontData DefaultFont;

class Image {
public:
    Image(Void);
    Image(UInt16, UInt16);
    Image(UInt32*, UInt16, UInt16);

    ~Image(Void);

    Image &operator =(const Image&);

    /* Same basic alpha blending functions, now also with a dummy arch-independent round function (it's on the private
     * functions section, probably we can later reverse to __builtin__round again?). */

    static inline UInt32 Blend(UInt32 Background, UInt32 Foreground, Float Alpha, Boolean UseForeAlpha = False) {
        UInt8 a, r1, r2, g1, g2, b1, b2;

        /* The alpha value from the colors don't matter, as we expect the user to pass an alpha value in the range 0-1
         * to us (Well, the user can also pass UseForeAlpha = True if he wants to use the alpha value from the
         * foreground as the blend alpha value). */

        EXTRACT_ARGB(Background, a, r1, g1, b1);
        EXTRACT_ARGB(Foreground, a, r2, g2, b2);

        if (UseForeAlpha) {
            Alpha = static_cast<Float>(a) / 255;
        }

        return MAKE_ARGB(0xFF, static_cast<UInt8>(Round((Alpha * r2) + ((1. - Alpha) * r1))),
                               static_cast<UInt8>(Round((Alpha * g2) + ((1. - Alpha) * g1))),
                               static_cast<UInt8>(Round((Alpha * b2) + ((1. - Alpha) * b1))));
    }

    static inline UInt32 Blend(UInt32 Background, UInt32 Foreground) {
        return Blend(Background, Foreground, 0, True);
    }

	/* Now we start with the functions that manipulate the image, beginning with Clear, which fills the image with a
	 * solid color. We DO have a function that can do that in the most optimized way possible (well, at least would be
	 * if we weren't depending on the compiler for doing the optimization), so we can just call it. */

    inline Void Clear(UInt32 Color) {
        if (Buffer != Null) {
            DrawRectangle(0, 0, Width, Height, Color, True);
        }
    }

    inline Void Scroll(UInt32 Height, UInt32 Color) {
        if (Buffer != Null) {
            CopyMemory(Buffer, &Buffer[Height * Width], (this->Height - Height) * Width * 4);
            SetMemory32(&Buffer[(this->Height - Height) * Width], Color, Height * Width);
        }
    }

    UInt32 GetPixel(UInt16, UInt16);
    Void PutPixel(UInt16, UInt16, UInt32);

    Void DrawLine(UInt16, UInt16, UInt16, UInt16, UInt32);
    Void DrawRectangle(UInt16, UInt16, UInt16, UInt16, UInt32, Boolean = False);
    Boolean DrawCharacter(UInt16, UInt16, Char, UInt32);

    template<typename... T> UIntPtr DrawString(UInt16 X, UInt16 Y, UInt32 Color, const String &Format, T... Args) {
        if (Buffer == Null || X >= Width || Y >= Height) {
            return 0;
        }

        /* As we can't use a lambda that captures local variables as a function pointer, we need to save and pass the
         * X/Y values in another way... */

        UIntPtr ctx[4] { reinterpret_cast<UIntPtr>(this), X, Y, Color };

        return VariadicFormat([](Char Data, Void *Context) -> Boolean {
            /* We don't handle here reaching the end of the screen and going into the next line, nor scrolling when we
             * reach the end of the screen. And also we don't handle TAB anywhere (for now). */

            auto ctx = static_cast<UIntPtr*>(Context);

            switch (Data) {
                case '\n': ctx[2] += DefaultFont.Height;
                case '\r': ctx[1] = 0; return True;
                default: {
                    if (!reinterpret_cast<Image*>(ctx[0])->DrawCharacter(ctx[1], ctx[2], Data, ctx[3])) {
                        return False;
                    }

                    ctx[1] += DefaultFont.GlyphInfo[(UInt8)Data].Advance;

                    return True;
                }
            }
        }, static_cast<Void*>(ctx), Format, Args...);
    }

    /* Some extra functions that allow us to access all the internal info about the image that we may need to
     * access. */

    UInt32 *GetBuffer(Void) const { return Buffer; }
    UInt16 GetWidth(Void) const { return Width; }
    UInt16 GetHeight(Void) const { return Height; }
private:
    /* Some math functions that we're going to put here for now (later we should re-add the math.hxx header). */

    static inline IntPtr Round(Float Value) {
        return static_cast<IntPtr>(Value < 0 ? Value - 0.5 : Value + 0.5);
    }

    static inline IntPtr Abs(IntPtr Value) {
        return Value < 0 ? -Value : Value;
    }

    static inline UInt16 Min(UInt16 Left, UInt16 Right) {
        return Left < Right ? Left : Right;
    }

    static inline UInt16 Max(UInt16 Left, UInt16 Right) {
        return Left > Right ? Left : Right;
    }

    Void Cleanup(Void);

    UInt32 *Buffer;
    Boolean Allocated;
    UIntPtr *References;
    UInt16 Width, Height;
};

}
