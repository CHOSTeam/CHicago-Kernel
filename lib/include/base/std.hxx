/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 04 of 2021, at 17:04 BRT
 * Last edited on March 04 of 2021 at 17:07 BRT */

#pragma once

#include <base/types.hxx>

/* We use this for aligning an allocation (from ::new or ::delete), the type name NEEDS to be align_val_t btw. And yes,
 * this is hackish as hell, I know. Also, we need to put the initializer list on the std namespace.  */

namespace std {

enum class align_val_t : long unsigned int { };

template<typename T> class initializer_list {
public:
    constexpr initializer_list() : Elements(Null), Size(0) { }

    constexpr const T *begin() const { return Elements; }
    constexpr const T *end() const { return &Elements[Size]; }

    constexpr long unsigned int GetLength() const { return Size; }
private:
    /* GCC expects the main constructor to be private and constexpr. */

    constexpr initializer_list(const T *Elements, long unsigned int Size) : Elements(Elements), Size(Size) { }

    const T *Elements;
    long unsigned int Size;
};

}

using namespace std;
