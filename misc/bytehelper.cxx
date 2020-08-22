/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 03 of 2020, at 12:30 BRT
 * Last edited on August 02 of 2020, at 00:41 BRT */

#include <chicago/string.hxx>

Void ByteHelper::Convert(Float Value, Char &Unit, UIntPtr &Left, UIntPtr &Right) {
	/* This function takes in a value in bytes, and convert it into the smallest possible unit, so we can represent
	 * it in a more human-readable way. */
	
	if (Value >= 0x1000000000000000) {
		Value /= 0x1000000000000000;
		Unit = 'E';
	} else if (Value >= 0x4000000000000) {
		Value /= 0x4000000000000;
		Unit = 'P';
	} else if (Value >= 0x10000000000) {
		Value /= 0x10000000000;
		Unit = 'T';
	} else if (Value >= 0x40000000) {
		Value /= 0x40000000;
		Unit = 'G';
	} else if (Value >= 0x100000) {
		Value /= 0x100000;
		Unit = 'M';
	} else if (Value >= 0x400) {
		Value /= 0x400;
		Unit = 'K';
	} else {
		/* If the value is already in the smallest unit, just return it without doing anything. */
		
		Left = (UIntPtr)Value;
		Right = 0;
		Unit = 0;
		
		return;
	}
	
	/* To get the left side, just convert the value into an integer, to get the (two first) numbers after
	 * the floating point, subtract the left value we got from the float value, multiply the result by 10*<how many
	 * number after the floating point we want>, and convert the result into an integer, */
	
	Left = (UIntPtr)Value;
	Right = (UIntPtr)((Value - Left) * 100);
}
