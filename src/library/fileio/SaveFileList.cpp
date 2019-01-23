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

#include "SaveFileList.h"

#include "SaveFile.h"
#include "../global.h" // shared_config
#include "../GlobalState.h"
#include "../logging.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <vector>
#include <memory>
#include <cstring>
#include <unistd.h>
#include <algorithm> // remove_if

namespace libtas {

namespace SaveFileList {

static std::vector<std::unique_ptr<SaveFile>> &savefiles() {
    /* Using initialization on first use idiom because this object could be
     * used by destructors, so we must make sure it is initialized before any
     * constructors of global objects that access files complete.  This causes
     * it to be destructed after those globals are destructed because global
     * destructors are guaranteed to be called in reverse order of construction.
     */
    static std::vector<std::unique_ptr<SaveFile>> savefiles;
    return savefiles;
}

/* Check if the file open permission allows for write operation */
bool isSaveFile(const char *file, const char *modes)
{
    for (const auto& savefile : savefiles()) {
        if (savefile->isSameFile(file)) {
            return true;
        }
    }

    if (!(strstr(modes, "w") || strstr(modes, "a") || strstr(modes, "+")))
        return false;

    return isSaveFile(file);
}

bool isSaveFile(const char *file, int oflag)
{
    for (const auto& savefile : savefiles()) {
        if (savefile->isSameFile(file)) {
            return true;
        }
    }

    if ((oflag & 0x3) == O_RDONLY)
        return false;

    /*
     * This is a sort of hack to prevent considering new shared
     * memory files as a savefile, which are opened using O_CLOEXEC
     */
    if (oflag & O_CLOEXEC)
        return false;

    return isSaveFile(file);
}

/* Detect save files (excluding the writeable flag), basically if the file is regular */
bool isSaveFile(const char *file)
{
    if (!shared_config.prevent_savefiles)
        return false;

    if (!file)
        return false;

    /* Check if file is a dev file */
    GlobalNative gn;
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
    if (! S_ISREG(filestat.st_mode))
        return false;

    /* Check if the file is a message queue, semaphore or shared memory object */
    if (S_TYPEISMQ(&filestat) || S_TYPEISSEM(&filestat) || S_TYPEISSHM(&filestat))
        return false;

    /* Check if the file lies in shared memory */
    if (strstr(file, "/dev/shm"))
        return false;

    return true;
}

FILE *openSaveFile(const char *file, const char *modes)
{
    for (const auto& savefile : savefiles()) {
        if (savefile->isSameFile(file)) {
            return savefile->open(modes);
        }
    }

    savefiles().emplace_back(new SaveFile(file));
    return savefiles().back()->open(modes);
}

int openSaveFile(const char *file, int oflag)
{
    for (const auto& savefile : savefiles()) {
        if (savefile->isSameFile(file)) {
            return savefile->open(oflag);
        }
    }

    savefiles().emplace_back(new SaveFile(file));
    return savefiles().back()->open(oflag);
}

int closeSaveFile(int fd)
{
    for (const auto& savefile : savefiles()) {
        if (savefile->fd == fd) {
            return savefile->closeFile();
        }
    }

    return 1;
}

int closeSaveFile(FILE *stream)
{
    for (const auto& savefile : savefiles()) {
        if (savefile->stream == stream) {
            return savefile->closeFile();
        }
    }

    return 1;
}

int removeSaveFile(const char *file)
{
    for (const auto& savefile : savefiles()) {
        if (savefile->isSameFile(file)) {
            return savefile->remove();
        }
    }

    /* If the file is not registered, create a removed savefile */
    if (shared_config.prevent_savefiles) {
        savefiles().emplace_back(new SaveFile(file));
        savefiles().back()->remove();

        GlobalNative gn;
        return access(file, W_OK);
    }

    return 1;
}

int renameSaveFile(const char *oldfile, const char *newfile)
{
    char* canonnewfile = SaveFile::canonicalizeFile(newfile);
    if (!canonnewfile)
        return -1;
    std::string newfilestr(canonnewfile);
    free(canonnewfile);

    /* Remove the newfile if present */
    savefiles().erase( std::remove_if(savefiles().begin(), savefiles().end(),
        [newfile](const std::unique_ptr<SaveFile>& s) { return (s->isSameFile(newfile));}),
        savefiles().end());

    for (const auto& savefile : savefiles()) {
        if (savefile->isSameFile(oldfile)) {
            savefile->filename = newfilestr;
            return 0;
        }
    }

    /* If the file is not registered, create a savefile */
    if (shared_config.prevent_savefiles) {
        savefiles().emplace_back(new SaveFile(oldfile));
        savefiles().back()->open("rb");
        savefiles().back()->filename = newfilestr;

        GlobalNative gn;
        return access(oldfile, W_OK);
    }

    return 1;
}

int getSaveFileFd(const char *file)
{
    for (const auto& savefile : savefiles()) {
        if (savefile->isSameFile(file)) {
            return savefile->fd;
        }
    }

    return 0;
}

bool isSaveFileRemoved(const char *file)
{
    for (const auto& savefile : savefiles()) {
        if (savefile->isSameFile(file)) {
            return savefile->removed;
        }
    }

    return true;
}


}

}
