/* File author is Ítalo Lima Marconato Matias
 * 
 * Created on May 11 of 2018, at 13:21 BRT
 * Last edited on June 26 of 2020, at 00:37 BRT */

ENTRY(KernelEntry)

SECTIONS
{
	. = 0xFFFF800000200000;
	
	.init ALIGN(4K) : AT(ADDR(.init) - 0xFFFF800000000000)
	{
		KEEP(*(SORT_NONE(.init)))
	}
	
	.text ALIGN(4K) : AT(ADDR(.text) - 0xFFFF800000000000)
	{
		*(.text.unlikely .text.*_unlikely .text.unlikely.*)
		*(.text.exit .text.exit.*)
		*(.text.startup .text.startup.*)
		*(.text.hot .text.hot.*)
		*(.text .stub .text.* .gnu.linkonce.t.*)
		*(.gnu.warning)
	}
	
	.fini ALIGN(4K) : AT(ADDR(.fini) - 0xFFFF800000000000)
	{
		KEEP(*(SORT_NONE(.fini)))
	}
	
	.ctors ALIGN(4K) : AT(ADDR(.ctors) - 0xFFFF800000000000)
	{
		KEEP(*crtbegin.o(.ctors))
		KEEP(*crtbegin?.o(.ctors))
		KEEP(*(EXCLUDE_FILE(*crtend.o *crtend?.o) .ctors))
		KEEP(*(SORT(.ctors.*)))
		KEEP(*(.ctors))
	}
	
	.dtors ALIGN(4K) : AT(ADDR(.dtors) - 0xFFFF800000000000)
	{
		KEEP(*crtend.o(.dtors))
		KEEP(*crtend?.o(.dtors))
		KEEP(*(EXCLUDE_FILE(*crtend.o *crtend?.o) .ctors))
		KEEP(*(SORT(.dtors.*)))
		KEEP(*(.dtors))
	}
	
	.jcr ALIGN(4K) : AT(ADDR(.jcr) - 0xFFFF800000000000)
	{
		KEEP(*(.jcr))
	}
	
	.rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xFFFF800000000000)
	{
		*(.rodata .rodata.* .gnu.linkonce.r.*)
		*(.tm_clone_table .eh_frame)
	}
	
	.data ALIGN(4K) : AT(ADDR(.data) - 0xFFFF800000000000)
	{
		*(.data .data.* .gnu.linkonce.d.*)
		SORT(CONSTRUCTORS)
	}
	
	.bss ALIGN(4K) : AT(ADDR(.bss) - 0xFFFF800000000000)
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
