// File author is Ítalo Lima Marconato Matias
//
// Created on October 19 of 2018, at 14:17 BRT
// Last edited on September 05 of 2019, at 18:18 BRT

#include <chicago/arch/idt.h>
#include <chicago/arch/port.h>

#include <chicago/device.h>

static UInt8 MouseCycle = 0;
static Int16 MouseByte[3];

static Void MouseHandler(PRegisters regs) {
	(Void)regs;
	
	switch (MouseCycle) {
		case 0: {
			MouseByte[0] = PortInByte(0x60);															// Mouse buttons
			MouseCycle++;
			break;
		}
		case 1: {
			MouseByte[1] = PortInByte(0x60);															// Mouse x (offset)
			MouseCycle++;
			break;
		}
		case 2: {
			MouseByte[2] = PortInByte(0x60);															// Mouse y (offset)
			
			if (MouseByte[0] & 0x80 || MouseByte[0] & 0x40) {											// Overflow?
				break;																					// Yes, so we can't use this packet
			}
			
			UInt8 buttons = 0;
			
			if ((MouseByte[0] & 0x01) == 0x01) {														// Left button click?
				buttons |= 0x01;
			}
			
			if ((MouseByte[0] & 0x02) == 0x02) {														// Right button click?
				buttons |= 0x02;
			}
			
			if ((MouseByte[0] & 0x04) == 0x04) {														// Middle button click?
				buttons |= 0x04;
			}
			
			UInt16 sx = ((MouseByte[0] << 4) & 0x100);													// Get the sign of the x pos
			UInt16 sy = ((MouseByte[0] << 3) & 0x100);													// Get the sign of the y pos
			
			RawMouseDeviceWrite(MouseByte[1] - sx, -(MouseByte[2] - sy), buttons);						// Let's use this packet!
			
			MouseCycle = 0;
			
			break;
		}
	}
}

static Void MouseWait(UInt8 type) {
	UIntPtr i = 100000;
	
	while (i--) {																						// Wait (and check) until timeout variable (i) is 0
		if ((PortInByte(0x64) & (type == 0 ? 1 : 2)) == (type == 0 ? 1 : 0)) {							// Check
			break;																						// Ok, we can exit this loop
		}
	}
}

static UInt8 MouseRead(Void) {
	MouseWait(0);																						// Wait to be able to read the data
	return PortInByte(0x60);																			// Read the data
}

static Void MouseWrite(UInt8 data) {
	MouseWait(1);																						// Wait to be able to write the data
	PortOutByte(0x64, 0xD4);																			// Tell the mouse we are going to send a command
	MouseWait(1);																						// Wait for the response
	PortOutByte(0x60, data);																			// And write
}

Void ArchInitMouse(Void) {
	MouseWait(1);																						// Enable the auxiliary mouse device
	PortOutByte(0x64, 0xA8);
	
	MouseWait(1);																						// Enable the interrupts
	PortOutByte(0x64, 0x20);
	MouseWait(0);
	
	UInt8 status = PortInByte(0x60) | 2;
	
	MouseWait(1);
	PortOutByte(0x64, 0x60);
	MouseWait(1);
	PortOutByte(0x60, status);
	
	MouseWrite(0xF6);																					// We're going to use the default settings
	MouseRead();
	
	MouseWrite(0xF4);																					// Enable the mouse
	MouseRead();
	
	IDTRegisterIRQHandler(12, MouseHandler);															// And register the mouse IRQ handler!
}
