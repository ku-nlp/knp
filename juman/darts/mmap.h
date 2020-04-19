#ifndef MMAP_H
#define MMAP_H

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#else
#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if !defined(_WIN32) || defined(__CYGWIN__)
namespace {
int c_open(const char* name, int flag) { return open(name, flag); }
int c_close(int fd) { return close(fd); }
}
#endif

class Mmap {
    char *mem; // pointer to memory
    size_t length; // length of the file (mapped memory)
    int fd; // file descriptor
#if defined(_WIN32) && !defined(__CYGWIN__)
    HANDLE hFile, hMapping;
#endif
  public:

    Mmap(): mem(NULL), fd(-1) {}
    ~Mmap() {
        close();
    }

    bool open(const char *filename) {
        struct stat st;

        // open file
        fd = c_open(filename, O_RDONLY);
        // get file size
        if (fstat(fd, &st) < 0)
            return false;
        length = st.st_size;

#if defined(_WIN32) && !defined(__CYGWIN__)
        hFile = (HANDLE)_get_osfhandle(fd);
        if (hFile == (HANDLE) -1)
            return false;
        hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        if (!hMapping)
            return false;
        mem = (unsigned char *)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
        CloseHandle(hMapping);
        if (!mem)
            return false;
#else
#ifdef HAVE_MMAP
        mem = reinterpret_cast<char *>(mmap(NULL, length, PROT_READ, MAP_SHARED, fd, 0));
        if (mem == MAP_FAILED)
            return false;
#else
        std::cerr << "mmap is not supported." << std::endl;
        return false;
#endif
#endif
        c_close(fd);
        fd = -1;
        return true;
    }

    void close() {
        if (fd >= 0) {
            c_close(fd);
            fd = -1;
        }

        if (mem) {
#if defined(_WIN32) && !defined(__CYGWIN__)
            UnmapViewOfFile((void*)mem);
#else
#ifdef HAVE_MMAP
            munmap(mem, length);
            mem = NULL;
#endif
#endif
        }
    }

    char *begin() {
        return mem;
    }
};

#endif
