/* File author is Ítalo Lima Marconato Matias
 * 
 * Created on February 06 of 2021, at 12:54 BRT
 * Last edited on March 06 of 2021, at 14:35 BRT */

ENTRY(KernelEntry)

PHDRS {
    text PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    data PT_LOAD FLAGS(6);
}

SECTIONS {
    . = ALIGN(4K);

    .init : {
        KEEP(*(.init))
    } :text

    .fini : {
        KEEP(*(.fini))
    } :text

    .init_array : {
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
    } :text

    .fini_array : {
        __fini_array_start = .;
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        __fini_array_end = .;
    } :text

    .ctors : {
        KEEP(*crtbegin.o(.ctors))
        KEEP(*crtbegin?.o(.ctors))
        KEEP(*(EXCLUDE_FILE(*crtend.o *crtend?.o) .ctors))
        KEEP(*(SORT(.ctors.*)))
        KEEP(*(.ctors))
    } :text

    .dtors : {
        KEEP(*crtbegin.o(.dtors))
        KEEP(*crtbegin?.o(.dtors))
        KEEP(*(EXCLUDE_FILE(*crtend.o *crtend?.o) .dtors))
        KEEP(*(SORT(.dtors.*)))
        KEEP(*(.dtors))
    } :text

    .text : {
        *(.text.unlikely .text.*_unlikely .text.unlikely.*)
        *(.text.exit .text.exit.*)
        *(.text.startup .text.startup.*)
        *(.text.hot .text.hot.*)
        *(.text .stub .text.* .gnu.linkonce.t.*)
        *(.gnu.warning)
    } :text

    . = ALIGN(4K);

    /* The boot loader expects the very first bytes of the read only section to be our stack guard magic number (that it
     * will set). We go 8-bytes forward just to make sure (as we can't, afaik, get if the machine is 32 or 64-bits
     * here). */

    .rodata : {
        __stack_chk_guard = .;
        . += 8;
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    } :rodata

    .eh_frame : {
        KEEP(*(.eh_frame))
    } :rodata

    . = ALIGN(4K);

    .tm_clone_table : {
        KEEP(*(.tm_clone_table))
    } :data
    
    .jcr : {
        KEEP(*(.jcr))
    } :data

    .data : {
        *(.data .data.* .gnu.linkonce.d.*)
        SORT(CONSTRUCTORS)
    } :data

    .bss : {
        *(.bss .bss.* .gnu.linkonce.b.*)
        *(COMMON)
    } :data

    /DISCARD/ : {
        *(.comment)
        *(.gnu_debuglink .gnu.lto_*)
        *(.note.gnu.build-id .note.GNU-stack)
    }
}
