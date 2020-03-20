//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
// 
// ### Software developed externally (not by the QFlex group)
// 
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
// 
// ### Software developed internally (by the QFlex group)
// **QFlex License**
// 
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
/*
 * memfd.c
 *
 * Copyright (c) 2015 Red Hat, Inc.
 *
 * QEMU library functions on POSIX which are shared between QEMU and
 * the QEMU tools.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"

#include <glib/gprintf.h>

#include "qemu/memfd.h"

#if defined CONFIG_LINUX && !defined CONFIG_MEMFD
#include <sys/syscall.h>
#include <asm/unistd.h>

static int memfd_create(const char *name, unsigned int flags)
{
#ifdef __NR_memfd_create
    return syscall(__NR_memfd_create, name, flags);
#else
    return -1;
#endif
}
#endif

#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC 0x0001U
#endif

#ifndef MFD_ALLOW_SEALING
#define MFD_ALLOW_SEALING 0x0002U
#endif

/*
 * This is a best-effort helper for shared memory allocation, with
 * optional sealing. The helper will do his best to allocate using
 * memfd with sealing, but may fallback on other methods without
 * sealing.
 */
void *qemu_memfd_alloc(const char *name, size_t size, unsigned int seals,
                       int *fd)
{
    void *ptr;
    int mfd = -1;

    *fd = -1;

#ifdef CONFIG_LINUX
    if (seals) {
        mfd = memfd_create(name, MFD_ALLOW_SEALING | MFD_CLOEXEC);
    }

    if (mfd == -1) {
        /* some systems have memfd without sealing */
        mfd = memfd_create(name, MFD_CLOEXEC);
        seals = 0;
    }
#endif

    if (mfd != -1) {
        if (ftruncate(mfd, size) == -1) {
            perror("ftruncate");
            close(mfd);
            return NULL;
        }

        if (seals && fcntl(mfd, F_ADD_SEALS, seals) == -1) {
            perror("fcntl");
            close(mfd);
            return NULL;
        }
    } else {
        const char *tmpdir = g_get_tmp_dir();
        gchar *fname;

        fname = g_strdup_printf("%s/memfd-XXXXXX", tmpdir);
        mfd = mkstemp(fname);
        unlink(fname);
        g_free(fname);

        if (mfd == -1) {
            perror("mkstemp");
            return NULL;
        }

        if (ftruncate(mfd, size) == -1) {
            perror("ftruncate");
            close(mfd);
            return NULL;
        }
    }

    ptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, mfd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        close(mfd);
        return NULL;
    }

    *fd = mfd;
    return ptr;
}

void qemu_memfd_free(void *ptr, size_t size, int fd)
{
    if (ptr) {
        munmap(ptr, size);
    }

    if (fd != -1) {
        close(fd);
    }
}

enum {
    MEMFD_KO,
    MEMFD_OK,
    MEMFD_TODO
};

bool qemu_memfd_check(void)
{
    static int memfd_check = MEMFD_TODO;

    if (memfd_check == MEMFD_TODO) {
        int fd;
        void *ptr;

        ptr = qemu_memfd_alloc("test", 4096, 0, &fd);
        memfd_check = ptr ? MEMFD_OK : MEMFD_KO;
        qemu_memfd_free(ptr, 4096, fd);
    }

    return memfd_check == MEMFD_OK;
}
