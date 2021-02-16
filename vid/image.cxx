/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 07 of 2021, at 21:14 BRT
 * Last edited on February 16 of 2021 at 15:16 BRT */

#include <img.hxx>

using namespace CHicago;

Image::Image(Void) : Buffer(Null), Allocated(False), References(Null), Width(0), Height(0) { }
Image::Image(UInt32 *Buffer, UInt16 Width, UInt16 Height)
        : Buffer(Buffer), Allocated(False), References(Null), Width(Width), Height(Height) { }

Image::Image(UInt16 Width, UInt16 Height)
        : Buffer(new UInt32[Width * Height]), Allocated(True), References(new UIntPtr), Width(Width), Height(Height) {
    /* We just need to handle the case where the allocation failed (and we have to set everything back to zero. */

    if (Buffer == Null) {
        Allocated = False;
        this->Width = this->Height = 0;
    }

    if (References != Null) {
        (*References)++;
    }
}

Void Image::Cleanup(Void) {
    /* The Allocated variable should always be set to False if the buffer is Null/failed to be allocated, so we don't
     * need to handle that. The reference counter is what allows us to the stuff like we do on the display
     * initialization, if it wasn't for that, we would have some random memory overwriting it (and I did had this
     * problem in the start). */

    if (Allocated && (References == Null || --*References == 0)) {
        delete[] Buffer;
        delete References;
        References = Null;
    } else if (References != Null && *References == 0) {
        delete References;
        References = Null;
    }
}

Image::~Image(Void) {
    Cleanup();
}

Image &Image::operator =(const Image &Value) {
    if (this != &Value) {
        Cleanup();

        Buffer = Value.Buffer;
        Allocated = Value.Allocated;
        References = Value.References;
        Width = Value.Width;
        Height = Value.Height;

        if (References != Null) {
            (*References)++;
        }
    }

    return *this;
}

UInt32 Image::GetPixel(UInt16 X, UInt16 Y) {
    if (Buffer == Null || X >= Width || Y >= Height) {
        return 0;
    }

    return FIX_ARGB(Buffer[Y * Width + X]);
}

Void Image::PutPixel(UInt16 X, UInt16 Y, UInt32 Color) {
    if (Buffer != Null && X < Width && Y < Height) {
        Buffer[Y * Width + X] = FIX_ARGB(Color);
    }
}

Void Image::DrawLine(UInt16 StartX, UInt16 StartY, UInt16 EndX, UInt16 EndY, UInt32 Color) {
    /* Here we just need to return/do nothing if the line is completely outside the screen (or the screen is Null, for
     * some reason). */

    if (Buffer == Null || (StartX >= Width && EndX >= Width) || (StartY >= Height && EndY >= Height)) {
        return;
    }

    IntPtr sx = StartX >= Width ? Width - 1 : StartX, sy = StartY >= Height ? Height - 1 : StartY,
           ex = EndX >= Width ? Width - 1 : EndX, ey = EndY >= Height ? Height - 1 : EndY;

    Color = FIX_ARGB(Color);

	/* Fully horizontal/vertical lines can be easily made by just filling an entire area with SetMemory, or looping on Y
	 * and plotting the pixels. No need for any complex algorithm. */

    if (sy == ey) {
        SetMemory32(&Buffer[sy * Width + Min(sx, ex)], Color, Abs(ex - sx) + 1);
        return;
    } else if (sx == ex) {
        for (UInt16 y = Min(sy, ey); y <= Max(sy, ey); y++) {
            Buffer[y * Width + sx] = Color;
        }

        return;
    }

    /* For other random lines we can use the Bresenham's line drawing algorithm. */

    IntPtr dx = Abs(ex - sx), dy = Abs(ey - sy), sgx = ex > sx ? 1 : -1, sgy = ey > sy ? 1 : -1,
           e = (dx > dy ? dx : -dy) / 2, e2;

    while (True) {
        Buffer[sy * Width + sx] = Color;

        if (sx == ex && sy == ey) {
            break;
        } else if ((e2 = e) > -dx) {
           e -= dy;
           sx += sgx; 
        }

        if (e2 < dy) {
            e += dx;
            sy += sgy;
        }
    }
}

Void Image::DrawRectangle(UInt16 X, UInt16 Y, UInt16 Width, UInt16 Height, UInt32 Color, Boolean Fill) {
    /* Fix too big Width/Height values before going forward. */

    if (Buffer == Null || X >= this->Width || Y >= this->Height) {
        return;
    } else if (X + Width > this->Width) {
        Width = this->Width - X;
    }

    if (Y + Height > this->Height) {
        Height = this->Height - Y;
    }

    /* Unfilled rectangles are just 4 lines, filled rectangles are also just a bunch of lines (we can calc the start
     * address, and increase it each iteration, while also SetMemory()ing the place we need to fill). */

    if (!Fill) {
        DrawLine(X, Y, X + Width - 1, Y, Color);
        DrawLine(X, Y, X, Y + Height - 1, Color);
        DrawLine(X + Width - 1, Y, X + Width - 1, Y + Height - 1, Color);
        DrawLine(X, Y + Height - 1, X + Width - 1, Y + Height - 1, Color);
        return;
    }

    UInt32 *buf = &Buffer[Y * this->Width + X];

    for (UInt16 i = 0; i < Height; i++, buf += this->Width) {
        SetMemory32(buf, Color, Width - X);
    }
}

Boolean Image::DrawCharacter(UInt16 X, UInt16 Y, Char Data, UInt32 Color) {
    if (Buffer == Null || X >= Width || Y >= Height) {
        return False;
    }

	/* Drawing character with the new static font format is a bit different than the old way. In the old way, each
	 * pixel of the character was encoded as one bit, 1 meant that it was foreground, and 0 meant that it was
	 * background/we could ignore it. Now, we're converting/rendering TrueType fonts into the struct that we use, and
	 * it is not a monochrome bitmap anymore, now, each byte represents the brightness level of the pixel, 0 means
	 * that it is background, and 1-255 means that it is foreground, we need to get that brightness level and
	 * transform it into a 0-1 value, that we can treat as the alpha value of the pixel. With that, we can just call
	 * Blend, and let it do the job of changing the brightness for us. */

    const FontGlyph &info = DefaultFont.GlyphInfo[(UInt8)Data];
    const UInt8 *data = &DefaultFont.GlyphData[info.Offset];
    UInt16 gx = info.Left, gy = DefaultFont.Ascender - info.Top;
    UInt32 *start = &Buffer[(Y + gy) * Width + X + gx];

    for (UInt16 y = 0; y < info.Height; y++) {
        if (Y + gy + y >= Height) {
            return False;
        }

        for (UInt16 x = 0; x < info.Width; x++) {
            if (X + gx + x >= Width) {
                return False;
            }

            UInt8 bright = data[y * info.Width + x];
            UInt32 *pos = &start[y * Width + x];

            if (bright) {
                *pos = Blend(*pos, Color, static_cast<Float>(bright) / 255);
            }
        }
    }

    return True;
}

Void Image::DrawString(UInt16 X, UInt16 Y, UInt32 Color, const String &Format, ...) {
    if (Buffer == Null || X >= Width || Y >= Height) {
        return;
    }

    VariadicList args;
    VariadicStart(args, Format);

    /* As we can't use a lambda that captures local variables as a function pointer, we need to save and pass the X/Y
     * values in another way... */

    UIntPtr ctx[4] { reinterpret_cast<UIntPtr>(this), X, Y, Color };

    VariadicFormat(Format, args, [](Char Data, Void *Context) -> Boolean {
        /* We don't handle here reaching the end of the screen and going into the next line, nor scrolling when we
         * reach the end of the screen. And also we don't handle TAB anywhere (for now). */

        auto ctx = reinterpret_cast<UIntPtr*>(Context);

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
    }, reinterpret_cast<Void*>(ctx), Null, 0, 0);
    
    VariadicEnd(args);
}
