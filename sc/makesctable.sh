LANG=en_us_88591

(
	echo -e "// File author is Ítalo Lima Marconato Matias"
	echo -e "//"
	echo -e "// Created on January 18 of 2020, at 13:42 BRT"
	echo -e "// Last edited on $(date +%B) $(date +%d) of $(date +%Y), at $(LANG=pt_BR.UTF-8; date +%H:%M) BRT\n"
	echo -e "#define __SCTABLE__"
	echo -e "#include <chicago/sc.h>\n"
	
	echo -e "UIntPtr ScTable[] = {"
	
	cat $1 | sed '/^\/\// d' | (
		while read ret name ; do
			if [ "$ret" != "" ]; then
				echo -e "\t(UIntPtr)Sc$name,"
			fi
		done
	)
) | sed '$ s/,$//'

echo -e "};"
