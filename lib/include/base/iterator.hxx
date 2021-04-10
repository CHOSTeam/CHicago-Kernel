/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 15 of 2021, at 18:21 BRT
 * Last edited on March 15 of 2021 at 18:12 BRT */

#pragma once

#include <base/traits.hxx>

namespace CHicago {

class Iterator {
private:
    /* Basic iterator tags and the iterator Traits struct itself, we only need three different tags, and the base tag
     * (that most functions will not accept/the compiler will error out as there is no valid function prototype). */

    struct Tag {};
public:
    struct Forward : Tag {};
    struct Bidirectional : Forward {};
    struct AnyDirection : Bidirectional {};

    template<class T> struct Traits {
        using Tag = typename T::Tag;
        using Val = typename T::ValType;
        using Ptr = typename T::PtrType;
        using Ref = typename T::RefType;
    };

    template<class T> struct Traits<T*> {
        using Tag = AnyDirection;
        using Val = T;
        using Ptr = T*;
        using Ref = T&;
    };

    template<class T> struct Traits<const T*> {
        using Tag = AnyDirection;
        using Val = const T;
        using Ptr = const T*;
        using Ref = const T&;
    };

    Iterator() = delete;

    /* Getters for some iterator common properties (start iterator, end iterator, distance from the start to the
     * end). */

    template<class T> static constexpr auto GetStart(T &It) { return It.begin(); }
    template<class T> static constexpr auto GetStart(const T &It) { return It.begin(); }
    template<class T, UIntPtr N> static constexpr T *GetStart(T (&Array)[N]) { return Array; }

    template<class T> static constexpr UIntPtr GetLength(const T &It) { return It.GetLength(); }
    template<class T, UIntPtr N> static constexpr UIntPtr GetLength(T(&)[N]) { return N; }

    template<class T> static constexpr bool IsEmpty(const T &It) { return It.IsEmpty(); }
    template<class T, UIntPtr N> static constexpr bool IsEmpty(T(&)[N]) { return False; }

    template<class T> static constexpr auto GetEnd(T &It) { return It.end(); }
    template<class T> static constexpr auto GetEnd(const T &It) { return It.end(); }
    template<class T, UIntPtr N> static constexpr T *GetEnd(T (&Array)[N]) { return Array + N; }

    /* next() and prev() just call advance on a copied iterator (and return it in the end). */

    template<class T> static constexpr T Next(T It, UIntPtr Size = 1) { return Advance(It, Size), It; }

    template<class T> static constexpr EnableIf<IsBaseOfV<typename Traits<T>::Tag, Bidirectional>, T> Prev(T It, UIntPtr Size = 1) {
        return Advance(It, -Size), It;
    }

    template<class T> static constexpr void Advance(T &It, IntPtr Size) {
        /* On any_direction iterators we can use the addition operator (+=), as it is random access/a normal array, but
         * on others, we need to use ++ and -- manually (-- when it's a bidirectional iterator, and the Size is < 0). */

        if constexpr (IsBaseOfV<typename Traits<T>::Tag, AnyDirection>) It += Size;
        else if constexpr (IsBaseOfV<typename Traits<T>::Tag, Bidirectional>) {
            if (Size < 0) for (; Size++ != 0; --It) ;
            else for (; Size-- > 0; ++It) ;
        } else for (; Size-- > 0; ++It) ;
    }

    template<class T> static constexpr IntPtr Distance(T It1, const T &It2) {
        /* And on distance() is pretty much the same as advance(), but we increase/decrease a copy of 'It' (just to find
         * the distance), instead of the original 'It' itself. */

        if constexpr (IsBaseOfV<typename Traits<T>::Tag, AnyDirection>) return It2 - It1;

        IntPtr ret = 0;
        while (It1 != It2) ++It1;

        return ret;
    }

    template<class T> static constexpr auto Move(T &&It) { return Move(*It); }
    template<class T> static constexpr void Swap(T &&It1, T &&It2) { Swap(*It1, *It2); }
};

}
