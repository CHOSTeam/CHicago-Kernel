/* File author is Ítalo Lima Marconato Matias
 * 
 * Created on February 06 of 2021, at 12:54 BRT
 * Last edited on February 06 of 2021, at 16:09 BRT */

ENTRY(KernelEntry)

PHDRS {
    text PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    data PT_LOAD FLAGS(6);
}

SECTIONS {
    . = ALIGN(4K);

    .init : {
        KEEP(*(SORT_NONE(.init)))
    } :text

    .text : {
        *(.text.unlikely .text.*_unlikely .text.unlikely.*)
        *(.text.exit .text.exit.*)
        *(.text.startup .text.startup.*)
        *(.text.hot .text.hot.*)
        *(.text .stub .text.* .gnu.linkonce.t.*)
        *(.gnu.warning)
    } :text

    .fini : {
        KEEP(*(SORT_NONE(.fini)))
    } :text

    . = ALIGN(4K);

    .rodata : {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    } :rodata

    .eh_frame : {
        KEEP(*(.eh_frame))
    } :rodata

    . = ALIGN(4K);

    .ctors : {
        KEEP(*crtbegin.o(.ctors))
        KEEP(*crtbegin?.o(.ctors))
        KEEP(*(EXCLUDE_FILE(*crtend.o *crtend?.o) .ctors))
        KEEP(*(SORT(.ctors.*)))
        KEEP(*(.ctors))
    } :data
    
    .dtors : {
        KEEP(*crtbegin.o(.dtors))
        KEEP(*crtbegin?.o(.dtors))
        KEEP(*(EXCLUDE_FILE(*crtend.o *crtend?.o) .dtors))
        KEEP(*(SORT(.dtors.*)))
        KEEP(*(.dtors))
    } :data

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
