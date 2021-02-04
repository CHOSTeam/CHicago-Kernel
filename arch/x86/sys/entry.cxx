/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 04 of 2021, at 18:30 BRT
 * Last edited on February 04 of 2021 at 18:30 BRT */

extern "C" void KernelEntry(void*) {
    while (1) {
        asm volatile("hlt");
    }
}
