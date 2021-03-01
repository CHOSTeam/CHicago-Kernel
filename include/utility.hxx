/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 01 of 2021, at 11:59 BRT
 * Last edited on March 01 of 2021, at 12:13 BRT */

#pragma once

#include <types.hxx>

namespace CHicago {

template<class T> struct RemoveReference { typedef T Value; };
template<class T> struct RemoveReference<T&> { typedef T Value; };
template<class T> struct RemoveReference<T&&> { typedef T Value; };

template<class T> static inline constexpr T &&Forward(typename RemoveReference<T>::Value &Value) {
    return static_cast<T&&>(Value);
}

template<class T> static inline constexpr T &&Forward(typename RemoveReference<T>::Value &&Value) {
    return static_cast<T&&>(Value);
}

template<class T> static inline constexpr typename RemoveReference<T>::Value &&Move(T &&Value) {
    return static_cast<typename RemoveReference<T>::Value&&>(Value);
}

template<class T, class U = T> static inline constexpr T Exchange(T &Old, U &&New) {
    T old = Move(Old);
    return Old = Forward<U>(New), old;
}

template<class T> static inline constexpr Void Swap(T &Left, T &Right) {
    T tmp = Move(Left);
    Left = Move(Right), Right = Move(tmp);
}

}
