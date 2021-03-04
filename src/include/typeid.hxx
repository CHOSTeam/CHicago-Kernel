/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 04 of 2021, at 11:25 BRT
 * Last edited on March 04 of 2021, at 11:52 BRT */

#pragma once

#include <string.hxx>

namespace CHicago {

class Type {
public:
    template<typename T> static inline Type From(T = {}) {
        static UIntPtr id = GetNextID();
        return Type(id, __PRETTY_FUNCTION__);
    }

    inline Boolean operator ==(const Type &Value) const { return GetID() == Value.GetID(); }
    inline Boolean operator !=(const Type &Value) const { return GetID() != Value.GetID(); }

    inline UIntPtr GetID() const { return ID; }
    inline const String &GetName() const { return Name; }
private:
    Type(UIntPtr ID, const Char *Name) : ID(ID), Name(Name + 54) { this->Name.SetView(0, this->Name.GetViewEnd() - 1); }

    static UIntPtr GetNextID() { static UIntPtr last = 0; return last++; }

    UIntPtr ID;
    String Name;
};

}
