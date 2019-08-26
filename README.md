# CHicago Kernel

## Building it

For building it from scratch, you need:

	An Unix-like environment
	CHicago Toolchain in ../toolchain (instructions below)
	Make

To build it, go to the root directory and type:

	make

You can append ARCH=\<ARCH\> and SUBARCH=\<SUBARCH\> to change the system architecture, VERBOSE=true to enable verbose build (good for finding compilation errors) and DEBUG=yes to disable optimizations and make a debug build.
The output will be inside of the build folder (chkrnl-\<ARCH\>_\<SUBARCH\>)

## Toolchain

The CHicago Toolchain is inside of the main repository (https://github.com/CHOSTeam/CHicago), clone it, go inside of the toolchain folder, and build it for the desired architecture. After building just copy/move the whole toolchain folder to ../toolchain (relative to the kernel folder).
