/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 01 of 2021, at 11:59 BRT
 * Last edited on March 04 of 2021, at 17:28 BRT */

#pragma once

#include <base/types.hxx>

namespace CHicago {

template<class T> struct RemoveReference { using Type = T; };
template<class T> struct RemoveReference<T&> { using Type = T; };
template<class T> struct RemoveReference<T&&> { using Type = T; };

/* Forward, Move and Exchange are compiler black magic used for making our life a bit easier when, for example, we want
 * to implement move operators/constructors, or force a value to be moved instead of copied. */

template<class T> static inline constexpr T &&Forward(typename RemoveReference<T>::Type &Value) {
    return static_cast<T&&>(Value);
}

template<class T> static inline constexpr T &&Forward(typename RemoveReference<T>::Type &&Value) {
    return static_cast<T&&>(Value);
}

template<class T> static inline constexpr typename RemoveReference<T>::Type &&Move(T &&Value) {
    return static_cast<typename RemoveReference<T>::Type&&>(Value);
}

template<class T, class U = T> static inline constexpr T Exchange(T &Old, U &&New) {
    T old = Move(Old);
    return Old = Forward<U>(New), old;
}

template<class T> static inline constexpr Void Swap(T &Left, T &Right) {
    T tmp = Move(Left);
    Left = Move(Right), Right = Move(tmp);
}

/* The reverse iterator/wrapper struct are just so we can easily iterate on reverse (duh). */

template<typename T, typename U> class ReverseIterator {
public:
    class Iterator {
    public:
        Iterator(U Value) : Value(Value) { }

        U &GetIterator() { return Value; }

        T &operator *() { return *Value; }
        Iterator &operator ++() { return Value--, *this; }
        Boolean operator ==(const Iterator &Value) const { return this->Value == Value.Value; }
    private:
        U Value;
    };

    ReverseIterator(U Start, U End) : Start(Start), End(End) { this->Start--, this->End--; }

    inline Iterator begin() { return End; }
    inline Iterator end() { return Start; }
private:
    U Start, End;
};

template<typename T, typename U> class ConstReverseIterator {
public:
    class Iterator {
    public:
        Iterator(U Value) : Value(Value) { }

        U &GetIterator() { return Value; }

        T operator *() const { return *Value; }
        Iterator &operator ++() { return Value--, *this; }
        Boolean operator ==(const Iterator &Value) const { return this->Value == Value.Value; }
    private:
        U Value;
    };

    ConstReverseIterator(U Start, U End) : Start(Start), End(End) { this->Start--, this->End--; }

    inline Iterator begin() const { return End; }
    inline Iterator end() const { return Start; }
private:
    U Start, End;
};

}
