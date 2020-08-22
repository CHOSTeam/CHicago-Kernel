/* File author is Ítalo Lima Marconato Matias
 * 
 * Created on May 11 of 2018, at 13:21 BRT
 * Last edited on June 28 of 2020, at 00:36 BRT */

ENTRY(KernelEntry)

SECTIONS
{
	. = 0xC0100000;
	
	.init ALIGN(4K) : AT(ADDR(.init) - 0xC0000000)
	{
		KEEP(*(SORT_NONE(.init)))
	}
	
	.text ALIGN(4K) : AT(ADDR(.text) - 0xC0000000)
	{
		*(.text.unlikely .text.*_unlikely .text.unlikely.*)
		*(.text.exit .text.exit.*)
		*(.text.startup .text.startup.*)
		*(.text.hot .text.hot.*)
		*(.text .stub .text.* .gnu.linkonce.t.*)
		*(.gnu.warning)
	}
	
	.fini ALIGN(4K) : AT(ADDR(.fini) - 0xC0000000)
	{
		KEEP(*(SORT_NONE(.fini)))
	}
	
	.ctors ALIGN(4K) : AT(ADDR(.ctors) - 0xC0000000)
	{
		KEEP(*crtbegin.o(.ctors))
		KEEP(*crtbegin?.o(.ctors))
		KEEP(*(EXCLUDE_FILE(*crtend.o *crtend?.o) .ctors))
		KEEP(*(SORT(.ctors.*)))
		KEEP(*(.ctors))
	}
	
	.dtors ALIGN(4K) : AT(ADDR(.dtors) - 0xC0000000)
	{
		KEEP(*crtend.o(.dtors))
		KEEP(*crtend?.o(.dtors))
		KEEP(*(EXCLUDE_FILE(*crtend.o *crtend?.o) .ctors))
		KEEP(*(SORT(.dtors.*)))
		KEEP(*(.dtors))
	}
	
	.jcr ALIGN(4K) : AT(ADDR(.jcr) - 0xC0000000)
	{
		KEEP(*(.jcr))
	}
	
	.rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xC0000000)
	{
		*(.rodata .rodata.* .gnu.linkonce.r.*)
	}
	
	.data ALIGN(4K) : AT(ADDR(.data) - 0xC0000000)
	{
		*(.data .data.* .gnu.linkonce.d.*)
		SORT(CONSTRUCTORS)
	}
	
	.bss ALIGN(4K) : AT(ADDR(.bss) - 0xC0000000)
	{
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(COMMON)
	}
	
	/DISCARD/ :
	{
		*(.note.GNU-stack)
		*(.gnu_debuglink)
		*(.gnu.lto_*)
		*(.comment)
		*(.note.gnu.build-id)
	}
}
