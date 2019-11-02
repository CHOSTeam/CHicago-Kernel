/*
   File author is Ítalo Lima Marconato Matias
  
   Created on May 11 of 2018, at 13:21 BRT
   Last edited on November 13 of 2018, at 16:46 BRT
*/

ENTRY(KernelEntry)

SECTIONS
{
	. = 0xC0100000;
	
	KernelStart = .;
	
	.text ALIGN(4K) : AT(ADDR(.text) - 0xC0000000)
	{
		*(.text.unlikely .text.*_unlikely .text.unlikely.*)
		*(.text.exit .text.exit.*)
		*(.text.startup .text.startup.*)
		*(.text.hot .text.hot.*)
		*(.text .stub .text.* .gnu.linkonce.t.*)
		*(.gnu.warning)
	}
	
	.rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xC0000000)
	{
		*(.rodata .rodata.* .gnu.linkonce.r.*)
	}
	
	.data ALIGN(4K) : AT(ADDR(.data) - 0xC0000000)
	{
		*(.data .data.* .gnu.linkonce.d.*)
	}
	
	.bss ALIGN(4K) : AT(ADDR(.bss) - 0xC0000000)
	{
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(COMMON)
	}
	
	KernelEnd = .;
	
	/DISCARD/ :
	{
		*(.note.GNU-stack)
		*(.gnu_debuglink)
		*(.gnu.lto_*)
		*(.comment)
		*(.eh_frame)
		*(.note.gnu.build-id)
	}
}
