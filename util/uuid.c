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
 *  QEMU UUID functions
 *
 *  Copyright 2016 Red Hat, Inc.
 *
 *  Authors:
 *   Fam Zheng <famz@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qemu/uuid.h"
#include "qemu/bswap.h"

void qemu_uuid_generate(QemuUUID *uuid)
{
    int i;
    uint32_t tmp[4];

    QEMU_BUILD_BUG_ON(sizeof(QemuUUID) != 16);

    for (i = 0; i < 4; ++i) {
        tmp[i] = g_random_int();
    }
    memcpy(uuid, tmp, sizeof(tmp));
    /* Set the two most significant bits (bits 6 and 7) of the
      clock_seq_hi_and_reserved to zero and one, respectively. */
    uuid->data[8] = (uuid->data[8] & 0x3f) | 0x80;
    /* Set the four most significant bits (bits 12 through 15) of the
      time_hi_and_version field to the 4-bit version number.
      */
    uuid->data[6] = (uuid->data[6] & 0xf) | 0x40;
}

int qemu_uuid_is_null(const QemuUUID *uu)
{
    static QemuUUID null_uuid;
    return memcmp(uu, &null_uuid, sizeof(QemuUUID)) == 0;
}

void qemu_uuid_unparse(const QemuUUID *uuid, char *out)
{
    const unsigned char *uu = &uuid->data[0];
    snprintf(out, UUID_FMT_LEN + 1, UUID_FMT,
             uu[0], uu[1], uu[2], uu[3], uu[4], uu[5], uu[6], uu[7],
             uu[8], uu[9], uu[10], uu[11], uu[12], uu[13], uu[14], uu[15]);
}

char *qemu_uuid_unparse_strdup(const QemuUUID *uuid)
{
    const unsigned char *uu = &uuid->data[0];
    return g_strdup_printf(UUID_FMT,
                           uu[0], uu[1], uu[2], uu[3], uu[4], uu[5], uu[6],
                           uu[7], uu[8], uu[9], uu[10], uu[11], uu[12],
                           uu[13], uu[14], uu[15]);
}

static bool qemu_uuid_is_valid(const char *str)
{
    int i;

    for (i = 0; i < strlen(str); i++) {
        const char c = str[i];
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (str[i] != '-') {
                return false;
            }
        } else {
            if ((c >= '0' && c <= '9') ||
                (c >= 'A' && c <= 'F') ||
                (c >= 'a' && c <= 'f')) {
                continue;
            }
            return false;
        }
    }
    return i == 36;
}

int qemu_uuid_parse(const char *str, QemuUUID *uuid)
{
    unsigned char *uu = &uuid->data[0];
    int ret;

    if (!qemu_uuid_is_valid(str)) {
        return -1;
    }

    ret = sscanf(str, UUID_FMT, &uu[0], &uu[1], &uu[2], &uu[3],
                 &uu[4], &uu[5], &uu[6], &uu[7], &uu[8], &uu[9],
                 &uu[10], &uu[11], &uu[12], &uu[13], &uu[14],
                 &uu[15]);

    if (ret != 16) {
        return -1;
    }
    return 0;
}

/* Swap from UUID format endian (BE) to the opposite or vice versa.
 */
QemuUUID qemu_uuid_bswap(QemuUUID uuid)
{
    bswap32s(&uuid.fields.time_low);
    bswap16s(&uuid.fields.time_mid);
    bswap16s(&uuid.fields.time_high_and_version);
    return uuid;
}
