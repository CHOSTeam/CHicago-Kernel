/* File author is Ítalo Lima Marconato Matias
 *
 * Created on January 31 of 2021, at 19:49 BRT
 * Last edited on February 04 of 2021 at 15:35 BRT */

struct __attribute__((packed)) BootInfo {
    unsigned int Magic;
    unsigned long long KernelStart, RegionsStart, KernelEnd,
                       EfiTempAddress;
    unsigned long long MinPhysicalAddress, MaxPhysicalAddress, PhysicalMemorySize;
    void *Directory;

    struct __attribute__((packed)) {
        unsigned long long Count;
        void *Entries;
    } MemoryMap;

    struct __attribute__((packed)) {
        unsigned long long Size, Index;
        char *Data;
    } BootImage;

    struct __attribute__((packed)) {
        unsigned long long Width, Height, Size, Address;
    } FrameBuffer;

    char KernelStack[0x10000];
};

extern "C" void _init();

extern "C" void KernelEntry(BootInfo *Info) {
    _init();

    for (unsigned long long i = 0; i < Info->FrameBuffer.Width * Info->FrameBuffer.Height; i++) {
        ((unsigned int*)Info->FrameBuffer.Address)[i] = 0xFF0000FF;
    }
    
    while (1) ;
}
