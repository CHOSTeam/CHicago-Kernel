/* File author is Ítalo Lima Marconato Matias
 * 
 * Created on January 31 of 2020, at 19:31 BRT
 * Last edited on February 02 of 2020, at 10:40 BRT */

ENTRY(KernelEntry)

SECTIONS
{
    . = 0xFFFFFF8000000000;

    .init ALIGN(4K) : AT(ADDR(.init) - 0xFFFFFF8000000000)
    {
        KEEP(*(SORT_NONE(.init)))
    }

    .text ALIGN(4K) : AT(ADDR(.text) - 0xFFFFFF8000000000)
    {
        *(.text.unlikely .text.*_unlikely .text.unlikely.*)
        *(.text.exit .text.exit.*)
        *(.text.startup .text.startup.*)
        *(.text.hot .text.hot.*)
        *(.text .stub .text.* .gnu.linkonce.t.*)
        *(.gnu.warning)
    }

    .ctors ALIGN(4K) : AT(ADDR(.ctors) - 0xFFFFFF8000000000)
    {
        KEEP(*crtbegin.o(.ctors))
        KEEP(*crtbegin?.o(.ctors))
        KEEP(*(EXCLUDE_FILE(*crtend.o *crtend?.o) .ctors))
        KEEP(*(SORT(.ctors.*)))
        KEEP(*(.ctors))
    }
    
    .dtors ALIGN(4K) : AT(ADDR(.dtors) - 0xFFFFFF8000000000)
    {
        KEEP(*crtbegin.o(.dtors))
        KEEP(*crtbegin?.o(.dtors))
        KEEP(*(EXCLUDE_FILE(*crtend.o *crtend?.o) .dtors))
        KEEP(*(SORT(.dtors.*)))
        KEEP(*(.dtors))
    }

    .tm_clone_table : AT(ADDR(.tm_clone_table) - 0xFFFFFF8000000000)
    {
        KEEP(*(.tm_clone_table))
    }
    
    .jcr ALIGN(4K) : AT(ADDR(.jcr) - 0xFFFFFF8000000000)
    {
        KEEP(*(.jcr))
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xFFFFFF8000000000)
    {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    }

    .data ALIGN(4K) : AT(ADDR(.data) - 0xFFFFFF8000000000)
    {
        *(.data .data.* .gnu.linkonce.d.*)
        SORT(CONSTRUCTORS)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - 0xFFFFFF8000000000)
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
