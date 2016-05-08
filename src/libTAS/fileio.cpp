/*
    Copyright 2015-2016 Clément Gallet <clement.gallet@ens-lyon.org>

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

#include "fileio.h"

#ifdef LIBTAS_ENABLE_FILEIO_HOOKING

#include "logging.h"
#include "hook.h"
#include "../shared/Config.h"
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <fcntl.h>
#include <map>
#include <string>
#include <sys/stat.h>
#include <errno.h>

/*** SDL file IO ***/

namespace orig {
    static SDL_RWops *(*SDL_RWFromFile)(const char *file, const char *mode);
    static SDL_RWops *(*SDL_RWFromFP)(FILE * fp, SDL_bool autoclose);
}

size_t dummyWrite (struct SDL_RWops * context, const void *ptr,
                              size_t size, size_t num);
size_t dummyWrite (struct SDL_RWops * context, const void *ptr,
                              size_t size, size_t num)
{
    debuglog(LCF_FILEIO, "Preventing writing ", num, " objects of size ", size);
    return num;
}

SDL_RWops *SDL_RWFromFile(const char *file, const char *mode)
{
    debuglog(LCF_FILEIO, __func__, " call with file ", file, " with mode ", mode);
    SDL_RWops* handle = orig::SDL_RWFromFile(file, mode);

    if (!config.prevent_savefiles) {
    /* We replace the write callback with our own function */
        if (handle)
            handle->write = dummyWrite;
    }
    return handle;
}

SDL_RWops *SDL_RWFromFP(FILE * fp, SDL_bool autoclose)
{
    debuglog(LCF_FILEIO, __func__, " call");
    SDL_RWops* handle = orig::SDL_RWFromFP(fp, autoclose);

    if (!config.prevent_savefiles) {
        /* We replace the write callback with our own function */
        if (handle)
            handle->write = dummyWrite;
    }
    return handle;
}

void link_sdlfileio(void)
{
    LINK_NAMESPACE_SDL2(SDL_RWFromFile);
    LINK_NAMESPACE_SDL2(SDL_RWFromFP);
}

/*** stdio file IO ***/

namespace orig {
    static FILE *(*fopen) (const char *filename, const char *modes) = nullptr;
    static FILE *(*fopen64) (const char *filename, const char *modes) = nullptr;
    static int (*fprintf) (FILE *stream, const char *format, ...) = nullptr;
    static int (*vfprintf) (FILE *s, const char *format, va_list arg) = nullptr;
    static int (*fputc) (int c, FILE *stream) = nullptr;
    static int (*putc) (int c, FILE *stream) = nullptr;
    static size_t (*fwrite) (const void *ptr, size_t size,
            size_t n, FILE *s) = nullptr;
}

FILE *fopen (const char *filename, const char *modes)
{
    if (!orig::fopen) {
        link_stdiofileio();
        if (!orig::fopen) {
            printf("Failed to link fopen\n");
            return NULL;
        }
    }

    /* iostream functions are banned inside this function, I'm not sure why.
     * This is the case for every open function.
     * Generating debug message using stdio.
     */
    if (filename)
        debuglogstdio(LCF_FILEIO, "%s call with filename %s and mode %s", __func__, filename, modes);
    else
        debuglogstdio(LCF_FILEIO, "%s call with null filename", __func__);
    return orig::fopen(filename, modes);
}

FILE *fopen64 (const char *filename, const char *modes)
{
    if (!orig::fopen64) {
        link_stdiofileio();
        if (!orig::fopen64) {
            printf("Failed to link fopen64\n");
            return NULL;
        }
    }

    if (filename)
        debuglogstdio(LCF_FILEIO, "%s call with filename %s and mode %s", __func__, filename, modes);
    else
        debuglogstdio(LCF_FILEIO, "%s call with null filename", __func__);

    return orig::fopen64(filename, modes);
}

int fprintf (FILE *stream, const char *format, ...)
{
    DEBUGLOGCALL(LCF_FILEIO);

    /* We cannot pass the arguments to fprintf_real. However, we
     * can build a va_list and pass it to vfprintf
     */
    va_list args;
    va_start(args, format);
    int ret = orig::vfprintf(stream, format, args);
    va_end(args);
    return ret;
}

int vfprintf (FILE *s, const char *format, va_list arg)
{
    DEBUGLOGCALL(LCF_FILEIO);
    return orig::vfprintf(s, format, arg);
}

int fputc (int c, FILE *stream)
{
    DEBUGLOGCALL(LCF_FILEIO);
    return orig::fputc(c, stream);
}

int putc (int c, FILE *stream)
{
    DEBUGLOGCALL(LCF_FILEIO);
    return orig::putc(c, stream);
}

size_t fwrite (const void *ptr, size_t size, size_t n, FILE *s)
{
    if (!orig::fwrite) {
        link_stdiofileio();
        if (!orig::fwrite) {
            printf("Failed to link fwrite\n");
            return 0;
        }
    }
    //DEBUGLOGCALL(LCF_FILEIO);
    //debuglogstdio(LCF_FILEIO, "%s call", __func__);
    return orig::fwrite(ptr, size, n, s);
}

void link_stdiofileio(void)
{
    LINK_NAMESPACE(fopen, nullptr);
    LINK_NAMESPACE(fopen64, nullptr);
    LINK_NAMESPACE(fprintf, nullptr);
    LINK_NAMESPACE(vfprintf, nullptr);
    LINK_NAMESPACE(fputc, nullptr);
    LINK_NAMESPACE(putc, nullptr);
    LINK_NAMESPACE(fwrite, nullptr);
}

namespace orig {
    static int (*open) (const char *file, int oflag, ...);
    static int (*open64) (const char *file, int oflag, ...);
    static int (*openat) (int fd, const char *file, int oflag, ...);
    static int (*openat64) (int fd, const char *file, int oflag, ...);
    static int (*creat) (const char *file, mode_t mode);
    static int (*creat64) (const char *file, mode_t mode);
    static int (*close) (int fd);
    static ssize_t (*write) (int fd, const void *buf, size_t n);
    static ssize_t (*pwrite) (int fd, const void *buf, size_t n, __off_t offset);
    static ssize_t (*pwrite64) (int fd, const void *buf, size_t n, __off64_t offset);
}

static std::map<int, std::string> posix_savefiles;

static bool isSaveFile(const char *file, int oflag)
{
    if (!config.prevent_savefiles)
        return false;

    if (!file)
        return false;

    static bool inited = 0;
    if (!inited) {
        posix_savefiles.clear();
        inited = 1;
    }

    /* Check if file is writeable */
    if ((oflag & 0x3) == O_RDONLY)
        return false;

    /* Check if file is a dev file */
    struct stat filestat;
    int rv = stat(file, &filestat);

    if (rv == -1) {
        /*
         * If the file does not exists,
         * we consider it as a savefile
         */
        if (errno == ENOENT)
            return true;

        /* For any other error, let's say no */
        return false;
    }

    /* Check if the file is a regular file */
    if (S_ISREG(filestat.st_mode))
        return true;

    return false;

}

int open (const char *file, int oflag, ...)
{
    if (!orig::open) {
        link_posixfileio();
        if (!orig::open) {
            printf("Failed to link open\n");
            return -1;
        }
    }

    if (file)
        debuglogstdio(LCF_FILEIO, "%s call with filename %s and flag %X", __func__, file, oflag);
    else
        debuglogstdio(LCF_FILEIO, "%s call with null filename and flag %X", __func__, oflag);

    int fd;
    if ((oflag & O_CREAT) || (oflag & O_TMPFILE))
    {
        va_list arg_list;
        mode_t mode;

        va_start(arg_list, oflag);
        mode = va_arg(arg_list, mode_t);
        va_end(arg_list);

        fd = orig::open(file, oflag, mode);
    }
    else
    {
        fd = orig::open(file, oflag);
    }

    if (isSaveFile(file, oflag))
        posix_savefiles[fd] = std::string(file); // non NULL file is tested in isSaveFile()

    return fd;
}

int open64 (const char *file, int oflag, ...)
{
    if (!orig::open64) {
        link_posixfileio();
        if (!orig::open64) {
            printf("Failed to link open64\n");
            return -1;
        }
    }

    if (file)
        debuglogstdio(LCF_FILEIO, "%s call with filename %s and flag %X", __func__, file, oflag);
    else
        debuglogstdio(LCF_FILEIO, "%s call with null filename and flag %X", __func__, oflag);

    int fd;
    if ((oflag & O_CREAT) || (oflag & O_TMPFILE))
    {
        va_list arg_list;
        mode_t mode;

        va_start(arg_list, oflag);
        mode = va_arg(arg_list, mode_t);
        va_end(arg_list);

        fd = orig::open64(file, oflag, mode);
    }
    else
    {
        fd = orig::open64(file, oflag);
    }

    if (isSaveFile(file, oflag))
        posix_savefiles[fd] = std::string(file); // non NULL file is tested in isSaveFile()

    return fd;
}

int openat (int fd, const char *file, int oflag, ...)
{
    if (!orig::openat) {
        link_posixfileio();
        if (!orig::openat) {
            printf("Failed to link openat\n");
            return -1;
        }
    }

    if (file)
        debuglogstdio(LCF_FILEIO, "%s call with filename %s and flag %X", __func__, file, oflag);
    else
        debuglogstdio(LCF_FILEIO, "%s call with null filename and flag %X", __func__, oflag);

    int newfd;
    if ((oflag & O_CREAT) || (oflag & O_TMPFILE))
    {
        va_list arg_list;
        mode_t mode;

        va_start(arg_list, oflag);
        mode = va_arg(arg_list, mode_t);
        va_end(arg_list);

        newfd = orig::openat(fd, file, oflag, mode);
    }
    else
    {
        newfd = orig::openat(fd, file, oflag);
    }

    if (isSaveFile(file, oflag))
        posix_savefiles[newfd] = std::string(file); // non NULL file is tested in isSaveFile()

    return newfd;
}

int openat64 (int fd, const char *file, int oflag, ...)
{
    if (!orig::openat64) {
        link_posixfileio();
        if (!orig::openat64) {
            printf("Failed to link openat64\n");
            return -1;
        }
    }
    
    if (file)
        debuglogstdio(LCF_FILEIO, "%s call with filename %s and flag %X", __func__, file, oflag);
    else
        debuglogstdio(LCF_FILEIO, "%s call with null filename and flag %X", __func__, oflag);

    int newfd;
    if ((oflag & O_CREAT) || (oflag & O_TMPFILE))
    {
        va_list arg_list;
        mode_t mode;

        va_start(arg_list, oflag);
        mode = va_arg(arg_list, mode_t);
        va_end(arg_list);

        newfd = orig::openat64(fd, file, oflag, mode);
    }
    else
    {
        newfd = orig::openat64(fd, file, oflag);
    }

    if (isSaveFile(file, oflag))
        posix_savefiles[newfd] = std::string(file); // non NULL file is tested in isSaveFile()

    return newfd;
}

int creat (const char *file, mode_t mode)
{
    if (!orig::creat) {
        link_posixfileio();
        if (!orig::creat) {
            printf("Failed to link creat\n");
            return -1;
        }
    }

    debuglog(LCF_FILEIO, __func__, " call with file ", file);

    int fd = orig::creat(file, mode);

    if (file)
        posix_savefiles[fd] = std::string(file);

    return fd;
}

int creat64 (const char *file, mode_t mode)
{
    if (!orig::creat64) {
        link_posixfileio();
        if (!orig::creat64) {
            printf("Failed to link creat64\n");
            return -1;
        }
    }

    debuglog(LCF_FILEIO, __func__, " call with file ", file);

    int fd = orig::creat64(file, mode);

    if (file)
        posix_savefiles[fd] = std::string(file);

    return fd;
}

int close (int fd)
{
    if (!orig::close) {
        link_posixfileio();
        if (!orig::close) {
            printf("Failed to link close\n");
            return -1;
        }
    }

    DEBUGLOGCALL(LCF_FILEIO);

    int rv = orig::close(fd);

    if (posix_savefiles.find(fd) != posix_savefiles.end()) {
        debuglog(LCF_FILEIO, "  close savefile ", posix_savefiles[fd]);
        posix_savefiles.erase(fd);
    }

    return rv;
}

ssize_t write (int fd, const void *buf, size_t n)
{
    if (!orig::write) {
        link_posixfileio();
        if (!orig::write) {
            printf("Failed to link write\n");
            return -1;
        }
    }
    DEBUGLOGCALL(LCF_FILEIO);

    if (config.prevent_savefiles) {
        if (posix_savefiles.find(fd) != posix_savefiles.end()) {
            debuglog(LCF_FILEIO, "  prevent write to ", posix_savefiles[fd]);
            return n;
        }
    }
    return orig::write(fd, buf, n);
}

ssize_t pwrite (int fd, const void *buf, size_t n, __off_t offset)
{
    if (!orig::pwrite) return n;
    DEBUGLOGCALL(LCF_FILEIO);

    if (config.prevent_savefiles) {
        if (posix_savefiles.find(fd) != posix_savefiles.end()) {
            debuglog(LCF_FILEIO, "  prevent write to ", posix_savefiles[fd]);
            return n;
        }
    }
    return orig::pwrite(fd, buf, n, offset);
}

ssize_t pwrite64 (int fd, const void *buf, size_t n, __off64_t offset)
{
    if (!orig::pwrite64) return n;
    DEBUGLOGCALL(LCF_FILEIO);

    if (config.prevent_savefiles) {
        if (posix_savefiles.find(fd) != posix_savefiles.end()) {
            debuglog(LCF_FILEIO, "  prevent write to ", posix_savefiles[fd]);
            return n;
        }
    }
    return orig::pwrite64(fd, buf, n, offset);
}

void link_posixfileio(void)
{
    LINK_NAMESPACE(open, nullptr);
    LINK_NAMESPACE(open64, nullptr);
    LINK_NAMESPACE(openat, nullptr);
    LINK_NAMESPACE(openat64, nullptr);
    LINK_NAMESPACE(creat, nullptr);
    LINK_NAMESPACE(creat64, nullptr);
    LINK_NAMESPACE(close, nullptr);
    LINK_NAMESPACE(write, nullptr);
    LINK_NAMESPACE(pwrite, nullptr);
    LINK_NAMESPACE(pwrite64, nullptr);
}

#else

void link_posixfileio(void) {}
void link_stdiofileio(void) {}
void link_sdlfileio(void) {}

#endif
