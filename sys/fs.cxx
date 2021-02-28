/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 28 of 2021, at 14:02 BRT
 * Last edited on February 28 of 2021 at 14:12 BRT */

#include <fs.hxx>

using namespace CHicago;

List<String> FileSys::TokenizePath(const String &Path) {
    /* The String class already have a function that tokenizes the string, and return a List<> of tokens, but we have
     * some tokens that we need to remove... */

    List<String> ret = Path.Tokenize("/");

    for (UIntPtr i = 0; i < ret.GetLength();) {
        const String &val = ret[i];

        if (val.Compare(".")) {
            /* '.' means the current directory, we can just remove the token without doing anything else. */
            ret.Remove(i);
        } else if (val.Compare("..")) {
            /* '..' means the parent directory, we need to remove ourselves and the token before us (or,
             * in case there is no entry before us, only remove ourselves. */

            if (i > 0) {
                ret.Remove(--i);
            }

            ret.Remove(i);
        } else {
            i++;
        }
    }

    return Move(ret);
}

String FileSys::CanonicalizePath(const String &Path, const String &Increment) {
    /* This time, we need to tokenize TWO strings, append the contents of the second list to the first, and do the same
     * process we did in the TokenizePath function. */

    List<String> lpath = Path.Tokenize("/"), lincr = Increment.Tokenize("/");

    if (lincr.GetLength() && lpath.Add(lincr) != Status::Success) {
        return {};
    }

    /* Now, we can do the same process to remove the '.' and the '..'s. */

    for (UIntPtr i = 0; i < lpath.GetLength();) {
        const String &val = lpath[i];

        if (val.Compare(".")) {
            /* '.' means the current directory, we can just remove the token without doing anything else. */
            lpath.Remove(i);
        } else if (val.Compare("..")) {
            /* '..' means the parent directory, we need to remove ourselves and the token before us (or,
             * in case there is no entry before us, only remove ourselves. */

            if (i > 0) {
                lpath.Remove(--i);
            }

            lpath.Remove(i);
        } else {
            i++;
        }
    }

    /* Finally, let's try to alloc the return string, let's just hope that the allocations are not going to fail here,
     * at the very end. */

    if (lpath.GetLength() == 0) {
        return "/";
    }

    String ret;

    for (const String &part : lpath) {
        ret.Append("/{}", part);
    }

    return Move(ret);
}
