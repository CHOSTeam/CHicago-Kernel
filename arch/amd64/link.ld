/* File author is Ítalo Lima Marconato Matias
 * 
 * Created on May 11 of 2018, at 13:21 BRT
 * Last edited on November 28 of 2020, at 14:33 BRT */

ENTRY(KernelEntry)

PHDRS
{
	text PT_LOAD;
	data PT_LOAD;
}

SECTIONS
{
	. = 0xFFFFFFFF80000000;
	
	.init ALIGN(4K) : AT(ADDR(.init) - 0xFFFFFFFF7FFFF000)
	{
		KEEP(*(SORT_NONE(.init)))
	} :text
	
	.text ALIGN(4K) : AT(ADDR(.text) - 0xFFFFFFFF7FFFF000)
	{
		*(.text.unlikely .text.*_unlikely .text.unlikely.*)
		*(.text.exit .text.exit.*)
		*(.text.startup .text.startup.*)
		*(.text.hot .text.hot.*)
		*(.text .stub .text.* .gnu.linkonce.t.*)
		*(.gnu.warning)
	} :text
	
	.fini ALIGN(4K) : AT(ADDR(.fini) - 0xFFFFFFFF7FFFF000)
	{
		KEEP(*(SORT_NONE(.fini)))
	} :text
	
	.ctors ALIGN(4K) : AT(ADDR(.ctors) - 0xFFFFFFFF7FFFF000)
	{
		KEEP(*crtbegin.o(.ctors))
		KEEP(*crtbegin?.o(.ctors))
		KEEP(*(EXCLUDE_FILE(*crtend.o *crtend?.o) .ctors))
		KEEP(*(SORT(.ctors.*)))
		KEEP(*(.ctors))
	} :data
	
	.dtors ALIGN(4K) : AT(ADDR(.dtors) - 0xFFFFFFFF7FFFF000)
	{
		KEEP(*crtbegin.o(.dtors))
		KEEP(*crtbegin?.o(.dtors))
		KEEP(*(EXCLUDE_FILE(*crtend.o *crtend?.o) .dtors))
		KEEP(*(SORT(.dtors.*)))
		KEEP(*(.dtors))
	} :data
	
	.rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xFFFFFFFF7FFFF000)
	{
		*(.rodata .rodata.* .gnu.linkonce.r.*)
	} :data
	
	.data ALIGN(4K) : AT(ADDR(.data) - 0xFFFFFFFF7FFFF000)
	{
		*(.data .data.* .gnu.linkonce.d.*)
		SORT(CONSTRUCTORS)
	} :data
	
	.bss ALIGN(4K) : AT(ADDR(.bss) - 0xFFFFFFFF7FFFF000)
	{
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(COMMON)
	} :data
	
	/DISCARD/ :
	{
		*(.note.GNU-stack)
		*(.gnu_debuglink)
		*(.gnu.lto_*)
		*(.comment)
		*(.note.gnu.build-id)
	}
}
