/* File author is Ítalo Lima Marconato Matias
 * 
 * Created on February 04 of 2021, at 18:32 BRT
 * Last edited on February 04 of 2021, at 18:26 BRT */

ENTRY(KernelEntry)

SECTIONS
{
    . = 0xC0000000;

    .init ALIGN(4K) :
    {
        KEEP(*(SORT_NONE(.init)))
    }

    .text ALIGN(4K) :
    {
        *(.text.unlikely .text.*_unlikely .text.unlikely.*)
        *(.text.exit .text.exit.*)
        *(.text.startup .text.startup.*)
        *(.text.hot .text.hot.*)
        *(.text .stub .text.* .gnu.linkonce.t.*)
        *(.gnu.warning)
    }

    .ctors ALIGN(4K) :
    {
        KEEP(*crtbegin.o(.ctors))
        KEEP(*crtbegin?.o(.ctors))
        KEEP(*(EXCLUDE_FILE(*crtend.o *crtend?.o) .ctors))
        KEEP(*(SORT(.ctors.*)))
        KEEP(*(.ctors))
    }
    
    .dtors ALIGN(4K) :
    {
        KEEP(*crtbegin.o(.dtors))
        KEEP(*crtbegin?.o(.dtors))
        KEEP(*(EXCLUDE_FILE(*crtend.o *crtend?.o) .dtors))
        KEEP(*(SORT(.dtors.*)))
        KEEP(*(.dtors))
    }

    .tm_clone_table ALIGN(4K) :
    {
        KEEP(*(.tm_clone_table))
    }
    
    .jcr ALIGN(4K) :
    {
        KEEP(*(.jcr))
    }

    .rodata ALIGN(4K) :
    {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    }

    .data ALIGN(4K) :
    {
        *(.data .data.* .gnu.linkonce.d.*)
        SORT(CONSTRUCTORS)
    }

    .bss ALIGN(4K) :
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
