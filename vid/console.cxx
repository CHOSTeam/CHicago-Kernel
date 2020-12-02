/* File author is Ítalo Lima Marconato Matias
 *
 * Created on August 07 of 2020, at 19:20 BRT
 * Last edited on October 24 of 2020, at 14:27 BRT */

#include <display.hxx>
#include <textout.hxx>

static ConsoleImpl ConsoleImp;

Void ConsoleImpl::InitConsoleInterface(UInt32 Background, UInt32 Foreground, Boolean CursorEnabled) {
	/* The console can be "initialized" multiple times, but you can also set the background, foreground
	 * and everything else by just accessing the Console variable.
	 * When we re-add processes/threads we should probably make this function reset the console ownership,
	 * because probably the kernel wants to get exclusive access to the display to show the panic function. */
	
	ConsoleImp.Background = Background;
	ConsoleImp.Foreground = Foreground;
	ConsoleImp.CursorEnabled = CursorEnabled;
	ConsoleImp.Max.X = Display::GetFrontBuffer()->GetWidth() / DefaultFontData.Width;
	ConsoleImp.Max.Y = Display::GetFrontBuffer()->GetHeight() / DefaultFontData.Height;
	Console = &ConsoleImp;
	
	Display::GetFrontBuffer()->Clear(Background);
	
	if (CursorEnabled) {
		Display::GetFrontBuffer()->DrawRectangle(0, DefaultFontData.Width, DefaultFontData.Height,
												ConsoleImp.Background, True);
	}
	
	Display::Update();
}

Void ConsoleImpl::SetPosition(const Vector2D<IntPtr> &Point) {
	Position = Vector2D<IntPtr>(Point.X < 0 ? 0 : (Point.X >= Max.X ? Max.X - 1 : Point.X),
								Point.Y < 0 ? 0 : (Point.Y >= Max.Y ? Max.Y - 1 : Point.Y));
}

Void ConsoleImpl::AfterWrite(Void) {
	Display::Update();
}

Void ConsoleImpl::WriteInt(Char Value) {
	/* We always have to "undraw" the cursor in the beginning, but we only have to draw it in the end if the
	 * cursor is enabled. Let's save the size of the font into a vector, and the display buffer as well, just
	 * to remove a bit of redundancy. */
	
	Vector2D<IntPtr> fsize(DefaultFontData.Width, DefaultFontData.Height);
	Image *buf = Display::GetFrontBuffer();
	
	buf->DrawRectangle(Position * fsize, DefaultFontData.Width, DefaultFontData.Height, Background, True);
	
	switch (Value) {
	case '\b': {
		/* Going backwards is just decreasing the X/Y variables, depending on if they are/aren't zero, and
		 * "undrawing" the character that is at the position that we moved into. */
		
		if (Position.X > 0) {
			Position.X--;
		} else if (Position.Y > 0) {
			Position.Y--;
			Position.X = Max.X - 1;
		}
		
		buf->DrawRectangle(Position * fsize, DefaultFontData.Width, DefaultFontData.Height, Background, True);
		
		break;
	}
	case '\n': {
		/* New Line and Carriage Return are connected here (you only need \n to go to the next line), New Line
		 * increases the Y position (as you may already imagine), and Carriage Return sets the X position to zero. */
		
		Position.Y++;
	}
	case '\r': {
		Position.X = 0;
		break;
	}
	case '\t': {
		/* Tab size is 4, you can change it by changing the 4 into whatever value you want, and the 3 into the
		 * value you choose - 1. */
		
		Position.X = (Position.X + 4) & ~3;
		
		break;
	}
	default: {
		buf->DrawCharacter(Position * fsize, Value, Foreground);
		Position.X++;
		break;
	}
	}
	
	/* Auto NL/CR if our X position is too high, and scroll up if the Y position is too high. */
	
	if (Position.X >= Max.X) {
		Position.X = 0;
		Position.Y++;
	}
	
	if (Position.Y >= Max.Y) {
		buf->Scroll(DefaultFontData.Height, Background);
		Position.Y = Max.Y - 1;
	}
	
	if (CursorEnabled) {
		buf->DrawRectangle(Position * fsize, DefaultFontData.Width, DefaultFontData.Height, Foreground, True);
	}
}
