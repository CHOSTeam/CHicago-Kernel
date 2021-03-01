/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 01 of 2021, at 11:59 BRT
 * Last edited on March 01 of 2021, at 15:48 BRT */

#pragma once

#include <types.hxx>

namespace CHicago {

template<class T> struct RemoveReference { typedef T Value; };
template<class T> struct RemoveReference<T&> { typedef T Value; };
template<class T> struct RemoveReference<T&&> { typedef T Value; };

/* The reverse iterator/wrapper structs (and begin/end functions) are just so we can easily iterate using rbegin/rend. */

template<typename T> class ReverseIterator {
public:
    ReverseIterator(T *Value) : Value(Value) { }

    T &operator *(Void) const { return *Value; }
    ReverseIterator &operator ++(Void) { return Value--, *this; }
    Boolean operator ==(const ReverseIterator &Value) const { return this->Value == Value.Value; }
private:
    T *Value;
};

template<typename T> class ConstReverseIterator {
public:
    ConstReverseIterator(const T *Value) : Value(Value) { }

    const T &operator *(Void) const { return *Value; }
    ConstReverseIterator &operator ++(Void) { return Value--, *this; }
    Boolean operator ==(const ConstReverseIterator &Value) const { return this->Value == Value.Value; }
private:
    const T *Value;
};

template<typename T> struct ReverseWrapper {
    ReverseWrapper(T &Value) : Value(Value) { }
    T &Value;
};

template<typename T> static inline auto begin(ReverseWrapper<T> Value) { return Value.Value.rbegin(); }
template<typename T> static inline auto end(ReverseWrapper<T> Value) { return Value.Value.rend(); }

/* Forward, Move and Exchange are compiler black magic used for making our life a bit easier when, for example, we want
 * to implement move operators/constructors, or force a value to be moved instead of copied. */

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
