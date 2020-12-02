/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 26 of 2020, at 22:25 BRT
 * Last edited on November 30 of 2020, at 15:41 BRT */

#ifndef __IMG_HXX__
#define __IMG_HXX__

#include <math.hxx>
#include <string.hxx>
#include <textout.hxx>

/* Helper macros for extracting/creating colors (extracting the A,R,G and B components, or creating colors
 * using said components. */

#define EXTRACT_ARGB(c, a, r, g, b) (a) = ((c) >> 24) & 0xFF; (r) = ((c) >> 16) & 0xFF; (g) = ((c) >> 8) & 0xFF; (b) = (c) & 0xFF
#define MAKE_ARGB(a, r, g, b) ((((a) & 0xFF) << 24) | (((r) & 0xFF) << 16) | (((g) & 0xFF) << 8) | ((b) & 0xFF))

struct FontData {
	UInt8 Width, Height;
	const UInt8 *Glyphs;
};

/* Different from way of storing coordinates, now we're going to use one class to store it (and we're going
 * to store it as doubles instead of unsigned integers, it's more precise, and let us do some pretty some
 * precise manipulation). */

template<class T> class Vector2D {
public:
	Vector2D<T>(Void) : X(0), Y(0) { };
	Vector2D<T>(T Value) : X(Value), Y(Value) { };
	Vector2D<T>(T X, T Y) : X(X), Y(Y) { };
	Vector2D<T>(const Vector2D &Source) { X = Source.X; Y = Source.Y; }
	
	inline T Dot(const Vector2D &Vect) const { return X * Vect.X + Y * Vect.Y; }
	inline T Cross(const Vector2D &Vect) const { return X * Vect.Y - Y * Vect.X; }
	inline Vector2D Squared(Void) const { return Vector2D<T>(X * X, Y * Y); }
	template<class U> inline Vector2D<U> Round(Void) const { return Vector2D<U>(Math::Round(X), Math::Round(Y)); }
	
	Vector2D &operator =(T Value) { X = Y = Value; return *this; };
	Vector2D &operator =(const Vector2D &Source) { X = Source.X; Y = Source.Y; return *this; }
	
	Vector2D operator -(Void) const { return Vector2D<T>(-X, -Y); }
	Vector2D operator +(T Value) const { return Vector2D<T>(X + Value, Y + Value); };
	Vector2D operator -(T Value) const { return Vector2D<T>(X - Value, Y - Value); };
	Vector2D operator *(T Value) const { return Vector2D<T>(X * Value, Y * Value); };
	Vector2D operator /(T Value) const { return Vector2D<T>(X / Value, Y / Value); };
	Vector2D operator +(const Vector2D &Source) const { return Vector2D<T>(X + Source.X, Y + Source.Y); }
	Vector2D operator -(const Vector2D &Source) const { return Vector2D<T>(X - Source.X, Y - Source.Y); }
	Vector2D operator *(const Vector2D &Source) const { return Vector2D<T>(X * Source.X, Y * Source.Y); }
	Vector2D operator /(const Vector2D &Source) const { return Vector2D<T>(X / Source.X, Y / Source.Y); }
	
	Vector2D &operator +=(T Value) { X += Value; Y += Value; return *this; }
	Vector2D &operator -=(T Value) { X -= Value; Y -= Value; return *this; }
	Vector2D &operator *=(T Value) { X *= Value; Y *= Value; return *this; }
	Vector2D &operator /=(T Value) { X /= Value; Y /= Value; return *this; }
	Vector2D &operator +=(const Vector2D &Source) { X += Source.X; Y += Source.Y; return *this; }
	Vector2D &operator -=(const Vector2D &Source) { X -= Source.X; Y -= Source.Y; return *this; }
	Vector2D &operator *=(const Vector2D &Source) { X *= Source.X; Y *= Source.Y; return *this; }
	Vector2D &operator /=(const Vector2D &Source) { X /= Source.X; Y /= Source.Y; return *this; }
	
	T X, Y;
};

class Image {
public:
	/* We need a 3 args constructor, that will manually alloc the image buffer, and a 4 args constructor,
	 * that will use the USER (or in this case probably the kernel) pre-allocated buffer.
	 * And also, we need a 0-args constructor, which is going to init everything to Null. */
	
	Image(const Image &Source) : Width(Source.Width), Height(Source.Height), References(Source.References),
								 Buffer(Source.Buffer), Allocated(Source.Allocated)
							   { if (References != Null) { (*References)++; } }
	Image(IntPtr, IntPtr);
	Image(IntPtr, IntPtr, UInt32*, Boolean = False);
	
	/* The destructor is required for when then 3+-args constructor(s) is/are used. */
	
	~Image(Void);
	
	/* And we also need the copy constructor... */
	
	Image &operator =(const Image&);
	
	/* Basic alpha blending, probably not the best way, but works at least it works for us.
	 * Also, this way at least work on both big-endian and little-endian systems. */
	
	static inline UInt32 BlendARGB(UInt32 Background, UInt32 Foreground, Float Alpha) {
		UInt8 a, r1, r2, g1, g2, b1, b2;
		
		/* The alpha value from the colors don't matter, as we expect the user to pass an alpha value
		 * in the range 0-1 to us. the (Void)a statement is required to prevent unused arg warnings. */
		
		EXTRACT_ARGB(Background, a, r1, g1, b1);
		EXTRACT_ARGB(Foreground, a, r2, g2, b2);
		(Void)a;
		
		return MAKE_ARGB(0xFF, (UInt8)Math::Round((Alpha * r2) + ((1.f - Alpha) * r1)),
							   (UInt8)Math::Round((Alpha * g2) + ((1.f - Alpha) * g1)),
							   (UInt8)Math::Round((Alpha * b2) + ((1.f - Alpha) * b1)));
	}
	
	static inline UInt32 BlendARGB(UInt32 Background, UInt32 Foreground) {
		return BlendARGB(Background, Foreground, (Float)((Foreground >> 24) & 0xFF) / 255);
	}
	
	/* Now we start with the functions that manipulate the image, beginning with Clear, which fills the
	 * image with a solid color. We DO have a function that can do that in the most optimzed way possible
	 * (well, at least would be if we weren't depending on the compiler for doing the optimization), so we
	 * can just call it. */
	
	inline Void Clear(UInt32 Color) {
		if (Buffer != Null) {
			StrSetMemory32(Buffer, Color, Width * Height);
		}
	}
	
	inline Void Scroll(IntPtr Height, UInt32 Color) { 
		if (Buffer != Null) {
			UInt32 lsz = Width * Height;
			StrCopyMemory32(Buffer, &Buffer[lsz], Width * this->Height - lsz);
			StrSetMemory32(&Buffer[Width * this->Height - lsz], Color, lsz);
		}
	}
	
	UInt32 GetPixel(const Vector2D<IntPtr>&);
	Void PutPixel(const Vector2D<IntPtr>&, UInt32);
	
	Void DrawLine(const Vector2D<IntPtr>&, const Vector2D<IntPtr>&, UInt32);
	Void DrawRectangle(const Vector2D<IntPtr>&, IntPtr, IntPtr, UInt32, Boolean = False);
	Void DrawCharacter(const Vector2D<IntPtr>&, Char, UInt32);
	Void DrawString(Vector2D<IntPtr>, const String&, UInt32);
	
	/* Some extra functions that allow us to access (almost) all the internal info about the image. */
	
	IntPtr GetWidth(Void) const { return Width; }
	IntPtr GetHeight(Void) const { return Height; }
	Void *GetBuffer(Void) const { return Buffer; }
private:
	IntPtr Width, Height, *References;
	UInt32 *Buffer;
	Boolean Allocated;
};

/* Console stuff also goes here, why not? */

class ConsoleImpl : public TextOutput {
public:
	static Void InitConsoleInterface(UInt32, UInt32, Boolean = False);
	
	Void SetPosition(const Vector2D<IntPtr>&);
	Vector2D<IntPtr> GetPosition(Void) const { return Position; }
	
	Boolean CursorEnabled;
	UInt32 Background, Foreground;
private:
	Void AfterWrite(Void) override;
	Void WriteInt(Char) override;
	
	Vector2D<IntPtr> Max, Position;
};

extern const FontData DefaultFontData;

#endif
