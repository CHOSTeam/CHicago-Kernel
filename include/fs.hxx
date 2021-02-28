/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 28 of 2021, at 14:01 BRT
 * Last edited on February 28 of 2021 at 14:02 BRT */

#pragma once

#include <list.hxx>
#include <string.hxx>

namespace CHicago {

class FileSys {
public:
    static List<String> TokenizePath(const String&);
    static String CanonicalizePath(const String&, const String& = "");
};

}
