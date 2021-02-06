/* File author is Ítalo Lima Marconato Matias
 * 
 * Created on February 06 of 2021, at 12:54 BRT
 * Last edited on February 06 of 2021, at 12:58 BRT */

ENTRY(KernelEntry)

SECTIONS
{
    .init :
    {
        KEEP(*(SORT_NONE(.init)))
    }

    .text :
    {
        *(.text.unlikely .text.*_unlikely .text.unlikely.*)
        *(.text.exit .text.exit.*)
        *(.text.startup .text.startup.*)
        *(.text.hot .text.hot.*)
        *(.text .stub .text.* .gnu.linkonce.t.*)
        *(.gnu.warning)
    }

    .ctors :
    {
        KEEP(*crtbegin.o(.ctors))
        KEEP(*crtbegin?.o(.ctors))
        KEEP(*(EXCLUDE_FILE(*crtend.o *crtend?.o) .ctors))
        KEEP(*(SORT(.ctors.*)))
        KEEP(*(.ctors))
    }
    
    .dtors :
    {
        KEEP(*crtbegin.o(.dtors))
        KEEP(*crtbegin?.o(.dtors))
        KEEP(*(EXCLUDE_FILE(*crtend.o *crtend?.o) .dtors))
        KEEP(*(SORT(.dtors.*)))
        KEEP(*(.dtors))
    }

    .tm_clone_table :
    {
        KEEP(*(.tm_clone_table))
    }
    
    .jcr :
    {
        KEEP(*(.jcr))
    }

    .rodata :
    {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    }

    .data :
    {
        *(.data .data.* .gnu.linkonce.d.*)
        SORT(CONSTRUCTORS)
    }

    .bss :
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
