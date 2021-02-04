/* File author is Ítalo Lima Marconato Matias
 *
 * Created on January 31 of 2021, at 19:49 BRT
 * Last edited on February 04 of 2021 at 18:29 BRT */

extern "C" void KernelEntry(void*) {
    while (1) {
        asm volatile("wfi");
    }
}
