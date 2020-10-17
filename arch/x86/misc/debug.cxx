/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 26 of 2020, at 18:40 BRT
 * Last edited on October 09 of 2020, at 20:40 BRT */

#include <chicago/arch/arch.hxx>
#include <chicago/arch/port.hxx>
#include <chicago/arch.hxx>

static DebugImpl DebugImp;

Void DebugImpl::WriteInt(Char Value) {
	/* Before sending the data, make sure that the port isn't busy doing something else (like sending
	 * other data, or receiving data). */
	
	while (!(Port::InByte(COM1_PORT + 5) & 0x20)) ;
	
	Port::OutByte(COM1_PORT, Value);
}

Void InitDebugInterface(Void) {
	/* COM1 is at the IO port 0x3F8, before using it, we need to configure it, first, set the divisor to
	 * 1 (115200bits/s). */
	
	Port::OutByte(COM1_PORT + 3, 0x80);
	Port::OutByte(COM1_PORT + 0, 0x01);
	Port::OutByte(COM1_PORT + 1, 0x00);
	
	/* Now, setup how we're going to send this data (data length 8-bits). */
	
	Port::OutByte(COM1_PORT + 3, 0x03);
	
	/* Finally, set the Debug interface to be us. */
	
	Debug = &DebugImp;
}
