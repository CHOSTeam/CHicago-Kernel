LANG=en_us_88591

echo -e "// File author is Ítalo Lima Marconato Matias"
echo -e "//"
echo -e "// Created on November 16 of 2018, at 01:04 BRT"
echo -e "// Last edited on $(date +%B) $(date +%d) of $(date +%Y), at $(LANG=pt_BR.UTF-8; date +%H:%M) BRT\n"
echo -e "#ifndef __CHICAGO_SC_H__"
echo -e "#define __CHICAGO_SC_H__\n"
echo -e "#include <chicago/ipc.h>\n"
echo -e "#define HANDLE_TYPE_FILE 0x00"
echo -e "#define HANDLE_TYPE_THREAD 0x01"
echo -e "#define HANDLE_TYPE_PROCESS 0x02"
echo -e "#define HANDLE_TYPE_IPC_RESPONSE_PORT 0x04\n"
echo -e "typedef struct {"
echo -e "\tPUInt32 major;"
echo -e "\tPUInt32 minor;"
echo -e "\tPUInt32 build;"
echo -e "\tPWChar codename;"
echo -e "\tPWChar arch;"
echo -e "} SystemVersion, *PSystemVersion;\n"
echo -e "typedef struct {"
echo -e "\tIntPtr num;"
echo -e "\tUInt8 type;"
echo -e "\tPVoid data;"
echo -e "\tBoolean used;"
echo -e "} Handle, *PHandle;\n"
echo -e "#ifndef __SCTABLE__"
echo -e "extern PUIntPtr ScTable;"
echo -e "#endif\n"
echo -e "IntPtr ScAppendHandle(UInt8 type, PVoid data);"
echo -e "Void ScFreeHandle(UInt8 type, PVoid data);\n"

cat $1 | sed '/^\/\// d' | (
	while read ret name ; do
		if [ "$ret" != "" ]; then
			echo -e "$ret Sc$name();"
		fi
	done
)

echo -e "\n#endif\t\t// __CHICAGO_SC_H__";
