/* File author is Ítalo Lima Marconato Matias
 *
 * Created on August 01 of 2020, at 17:46 BRT
 * Last edited on December 02 of 2020, at 08:45 BRT */

#include <img.hxx>

Image::Image(IntPtr Width, IntPtr Height) {
	/* Zero values for any of the variables mean that the user want us to use the default value for said
	 * variable (800 for Width and 600 for Height). Could also be that they for some reason forgot to set
	 * said field, but let's hope that's not the case. */
	
	if (Width == 0) {
		Width = 800;
	}
	
	if (Height == 0) {
		Height = 600;
	}
	
	this->Width = Width;
	this->Height = Height;
	References = new IntPtr;
	Buffer = new UInt32[Width * Height];
	Allocated = Buffer != Null;
	
	if (References != Null) {
		(*References)++;
	}
}

Image::Image(IntPtr Width, IntPtr Height, UInt32 *Buffer, Boolean Alloc) {
	/* Same rule as before, but in the case we set any of the values to the default, we need to alloc manually
	 * the buffer, instead of using the user provided one. If the buffer that the user provided was supposed
	 * to be fully handled by us, we need to free it. */
	
	if (Width == 0) {
		Alloc = Alloc == True ? 3 : 2;
		Width = 800;
	}
	
	if (Height == 0) {
		Alloc = Alloc == True ? 3 : 2;
		Height = 600;
	}
	
	this->Width = Width;
	this->Height = Height;
	References = new IntPtr;
	this->Buffer = Alloc < 2 ? Buffer : new UInt32[Width * Height];
	Allocated = (Alloc < 2 ? Alloc : True) && this->Buffer != Null;
	
	if (Alloc > 2 && Buffer != Null) {
		delete[] Buffer;
	}
	
	if (References != Null) {
		(*References)++;
	}
}

Void Image::Cleanup(Void) {
	/* The Allocated variable should always be set to False if the buffer is Null/failed to be allocated, so
	 * we don't need to handle that. The reference counter is what allows us to the stuff like we do on the
	 * display initialization, if it wasn't for that, we would have some random memory overwriting it (and I
	 * did had this problem in the start). */
	
	if (Allocated && (References == Null || --(*References) == 0)) {
		if (References != Null) {
			delete References;
		}
		
		delete[] Buffer;
	} else if (References != Null && *References == 0) {
		delete References;
	}
}

Image::~Image(Void) {
	Cleanup();
}

Image &Image::operator =(const Image &Source) {
	/* No need to overwrite anything if someone is trying to do this=this, but if we're actually getting a whole
	 * new buffer, we need to call the deconstructor (free the references pointer and the buffer, if necessary),
	 * and call the constructor again (copy the value from all the source variables into our variables). */
	
	if (Source.Buffer != Buffer) {
		Cleanup();
		
		Width = Source.Width;
		Height = Source.Height;
		Buffer = Source.Buffer;
		Allocated = Source.Allocated;
		References = Source.References;
		
		if (References != Null) {
			(*References)++;
		}
	}
	
	return *this;
}

UInt32 Image::GetPixel(const Vector2D<IntPtr> &Point) {
	/* In all of the function, we need to make sure that the given point is inside of the image area, else,
	 * we're probably going to page fault. */
	
	IntPtr x = Point.X >= Width ? Width - 1 : Point.X, y = Point.Y >= Height ? Height - 1 : Point.Y;
	
	return Buffer[y * Width + x];
}

Void Image::PutPixel(const Vector2D<IntPtr> &Point, UInt32 Color) {
	IntPtr x = Point.X >= Width ? Width - 1 : Point.X, y = Point.Y >= Height ? Height - 1 : Point.Y;
	
	Buffer[y * Width + x] = Color;
}

Void Image::DrawLine(const Vector2D<IntPtr> &Start, const Vector2D<IntPtr> &End, UInt32 Color) {
	IntPtr sx = Start.X >= Width ? Width - 1 : Start.X, sy = Start.Y >= Height ? Height - 1 : Start.Y,
		   ex = End.X >= Width ? Width - 1 : End.X, ey = End.Y >= Height ? Height - 1 : End.Y;
	
	/* Fully horizontal/vertical lines can be easily made by just filling an entire area with SetMemory, or
	 * looping on Y and plotting the pixels. No need to any complex algorithm. */
	
	if (sy == ey) {
		StrSetMemory32(&Buffer[sy * Width + Math::Min(sx, ex)], Color, Math::Abs(ex - sx) + 1);
		return;
	} else if (sx == ex) {
		for (IntPtr y = Math::Min(sy, ey); y <= Math::Max(sy, ey); y++) {
			Buffer[y * Width + sx] = Color;
		}
		
		return;
	}
	
	/* For other random lines we can use Bresenham's line drawing algorithm. */
	
	IntPtr dx = Math::Abs(ex - sx), dy = Math::Abs(ey - sy), sgx = ex > sx ? 1 : -1, sgy = ey > sy ? 1 : -1,
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

Void Image::DrawRectangle(const Vector2D<IntPtr> &Start, IntPtr Width, IntPtr Height, UInt32 Color, Boolean Fill) {
	if (Buffer == Null) {
		return;
	}
	
	/* Unfillled rectangles are just 4 lines, filled rectangles are also just a bunch of lines (we can calc
	 * the start address, and increase it each iteration, while also StrSetMemory32ing the place we need to fill). */
	
	if (!Fill) {
		Vector2D<IntPtr> w(Width - 1, 0), h(0, Height - 1), wh(Width - 1, Height - 1);
		
		DrawLine(Start, Start + w, Color);
		DrawLine(Start, Start + h, Color);
		DrawLine(Start + w, Start + wh, Color);
		DrawLine(Start + h, Start + wh, Color);
		
		return;
	}
	
	IntPtr x = Start.X >= this->Width ? this->Width - 1 : Start.X,
		   y = Start.Y >= this->Height ? this->Height - 1 : Start.Y,
		   sx = x + Width - 1 > this->Width ? this->Width - x : Width,
		   sy = y + Height - 1 > this->Height ? this->Height - y : Height,
		   start = y * this->Width + x;
	
	for (IntPtr i = 0; i < sy; i++, start += this->Width) {
		StrSetMemory32(&Buffer[start], Color, sx);
	}
}

Void Image::DrawCharacter(const Vector2D<IntPtr> &Start, Char Value, UInt32 Color) {
	if (Buffer == Null) {
		return;
	}
	
	/* Drawing character with the new static font format is a bit different than the old way. In the old way, each
	 * pixel of the character was encoded as one bit, 1 meant that it was foreground, and 0 meant that it was
	 * background/we could ignore it. Now, we're converting/rendering TrueType fonts into the struct that we use,
	 * and it is not a monochrome bitmap anymore, now, each byte represents the brightness level of the pixel, 0
	 * means that it is background, and 1-255 means that it is foreground, we need to get that brightness level and
	 * transform it into a 0-1 value, that we can treat as the alpha value of the pixel. With that, we can just call
	 * BlendARGB, and let it do the job of changing the brightness for us. */
	
	UIntPtr start = (Value < 0 ? 0 : Value) * DefaultFontData.Width * DefaultFontData.Height;
	
	for (Vector2D<IntPtr> point; point.Y < DefaultFontData.Height; point.Y++) {
		if (point.Y >= Height) {
			break;
		}
		
		for (point.X = 0; point.X < DefaultFontData.Width; point.X++) {
			if (point.X >= Width) {
				break;
			}
			
			Vector2D<IntPtr> p = Start + point;
			UInt8 bright = DefaultFontData.Glyphs[start + point.Y * DefaultFontData.Width + point.X];
			
			if (bright == 0) {
				continue;
			}
			
			Buffer[p.Y * Width + p.X] = BlendARGB(Buffer[p.Y * Width + p.X], Color, (Float)bright / 255);
		}
	}
}

Void Image::DrawString(Vector2D<IntPtr> Start, const String &Value, UInt32 Color) {
	if (Buffer == Null) {
		return;
	}
	
	for (Char ch : Value) {
		if (Start.X >= Width || Start.Y >= Height) {
			break;
		}
		
		DrawCharacter(Start, ch, Color);
		
		Start.X += DefaultFontData.Width;
	}
}
