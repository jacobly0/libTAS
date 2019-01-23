/*
    Copyright 2015-2018 Clément Gallet <clement.gallet@ens-lyon.org>

    This file is part of libTAS.

    libTAS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libTAS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libTAS.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dlhook.h"
#include "logging.h"
#include "hook.h"
#include <cstring>
#include <set>
#include "backtrace.h"

namespace libtas {

/* Set of libraries that are loaded by the game using dlopen */
static std::set<std::string> &libraries() {
    /* Games can sometimes call hooked functions very early in their execution,
     * possibly before all globals are constructed, so we must use a local static
     * which gets constructed on first use instead of a global.
     */
    static std::set<std::string> libraries;
    return libraries;
}

std::string find_lib(const char* library)
{
    for (auto const& itr : libraries())
        if (itr.find(library) != std::string::npos)
            return itr;

    std::string emptystring;
    return emptystring;
}

DEFINE_ORIG_POINTER(dlopen);
DEFINE_ORIG_POINTER(dlsym);

void *dlopen(const char *file, int mode) throw() {
    if (!orig::dlopen) {
        /* To access the real dlopen function, we use the fact that dlsym
         * calls internally _dl_sym.
         */
        orig::dlopen = reinterpret_cast<decltype(orig::dlopen)>(_dl_sym(RTLD_NEXT, "dlopen", reinterpret_cast<void*>(dlopen)));
    }

    if (GlobalState::isNative()) {
        return orig::dlopen(file, mode);
    }

    debuglog(LCF_HOOK, __func__, " call with file ", (file!=nullptr)?file:"<NULL>");

    void *result = orig::dlopen(file, mode);

    if (result && (file != nullptr))
        libraries().insert(std::string(file));
    return result;
}

void *dlsym(void *handle, const char *name) throw() {
    if (!orig::dlsym) {
        /* Again, we use the internal `_dl_sym` function to access to the
         * location of the real `dlsym` function. This may seems weird, and is
         * also implementation-dependant, but it is simple. `_dl_sym` does not
         * have error-checking so we only use it here.
         */
        orig::dlsym = reinterpret_cast<decltype(orig::dlsym)>(_dl_sym(RTLD_NEXT, "dlsym", reinterpret_cast<void*>(dlsym)));
    }

    if (GlobalState::isNative()) {
        return orig::dlsym(handle, name);
    }

    debuglog(LCF_HOOK, __func__, " call with function ", name);

    /* Special cases when dlsym is called with dl* functions (yes, it happens...). */
    if (strcmp(name, "dlopen") == 0)
        return reinterpret_cast<void*>(dlopen);
    if (strcmp(name, "dlsym") == 0)
        return reinterpret_cast<void*>(dlsym);

    /* FIXME: This design is not good enough.
     * This idea is to link to our defined function when there is one, instead
     * of the function inside the library that the game wants to load.
     * However, there is a problem in this, encountered in Towerfall:
     * This game bundles with libpng 1.5.10, and load this library dynamically.
     * Also, the ffmpeg libraries that we use are usually compiled with png
     * support, which means that we statically link to our libpng (1.6.28 for me)
     * Thus, calling `dlsym(RTLD_DEFAULT, "png_xxx")` will return the function
     * from our installed libpng library, which has no reason to be of the same
     * version as the one bundled with the game.
     *
     * One solution would be to check if the symbol name is defined strictly
     * in our library, excluding any shared library dependency.
     * AFAIK, dlopen and dlsym always include all dependencies. Maybe by doing
     * some arithmetic pointer shenanigan?
     */

    /* Correct way of using dlsym is with dlerror */
    dlerror(); // Reseting the internal buffer

    /* Try to link to an already defined function first */
    void *addr = orig::dlsym(RTLD_DEFAULT, name);
    char* error = dlerror();
    if (error == nullptr) {
        /* We found a matching symbol. Now checking that this symbol comes
         * from our library and not another linked library.
         */
        Dl_info info;
        int res = dladdr(addr, &info);
        if (res != 0) {
            std::string libpath = info.dli_fname;
            std::string libtasstr = "libtas.so"; // bad!
            if (libpath.length() >= libtasstr.length() &&
                libpath.compare(libpath.length()-libtasstr.length(), libtasstr.length(), libtasstr) == 0) {
                debuglog(LCF_HOOK, "   function ", name, " is overriden!");
                return addr;
            }
        }
    }

    addr = orig::dlsym(handle, name);
    return addr;
}

}
