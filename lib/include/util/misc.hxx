/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 01 of 2021, at 11:59 BRT
 * Last edited on March 15 of 2021, at 18:06 BRT */

#pragma once

#include <base/traits.hxx>

namespace CHicago {

/* Forward, Move and Exchange are compiler black magic used for making our life a bit easier when, for example, we want
 * to implement move operators/constructors, or force a value to be moved instead of copied. */

template<class T> static inline constexpr T &&Forward(RemoveRefT<T> &Value) {
    return static_cast<T&&>(Value);
}

template<class T> static inline constexpr T &&Forward(RemoveRefT<T> &&Value) {
    return static_cast<T&&>(Value);
}

template<class T> static inline constexpr auto Move(T &&Value) {
    return static_cast<RemoveRefT<T>&&>(Value);
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

template<class T, class U> class ReverseIterator {
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

template<class T, class U> class ConstReverseIterator {
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
