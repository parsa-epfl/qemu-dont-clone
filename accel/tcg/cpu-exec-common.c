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
 *  emulator main execution loop
 *
 *  Copyright (c) 2003-2005 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "cpu.h"
#include "sysemu/cpus.h"
#include "exec/exec-all.h"
#include "exec/memory-internal.h"

bool tcg_allowed;

/* exit the current TB, but without causing any exception to be raised */
void cpu_loop_exit_noexc(CPUState *cpu)
{
    /* XXX: restore cpu registers saved in host registers */

    cpu->exception_index = -1;
    siglongjmp(cpu->jmp_env, 1);
}

#if defined(CONFIG_SOFTMMU)
void cpu_reloading_memory_map(void)
{
    PTH_UPDATE_CONTEXT
    if (qemu_in_vcpu_thread() && PTH(current_cpu)->running) {
        /* The guest can in theory prolong the RCU critical section as long
         * as it feels like. The major problem with this is that because it
         * can do multiple reconfigurations of the memory map within the
         * critical section, we could potentially accumulate an unbounded
         * collection of memory data structures awaiting reclamation.
         *
         * Because the only thing we're currently protecting with RCU is the
         * memory data structures, it's sufficient to break the critical section
         * in this callback, which we know will get called every time the
         * memory map is rearranged.
         *
         * (If we add anything else in the system that uses RCU to protect
         * its data structures, we will need to implement some other mechanism
         * to force TCG CPUs to exit the critical section, at which point this
         * part of this callback might become unnecessary.)
         *
         * This pair matches cpu_exec's rcu_read_lock()/rcu_read_unlock(), which
         * only protects cpu->as->dispatch. Since we know our caller is about
         * to reload it, it's safe to split the critical section.
         */
        rcu_read_unlock();
        rcu_read_lock();
    }
}
#endif

void cpu_loop_exit(CPUState *cpu)
{
    siglongjmp(cpu->jmp_env, 1);
}

void cpu_loop_exit_restore(CPUState *cpu, uintptr_t pc)
{
    if (pc) {
        cpu_restore_state(cpu, pc);
    }
    siglongjmp(cpu->jmp_env, 1);
}

void cpu_loop_exit_atomic(CPUState *cpu, uintptr_t pc)
{
    cpu->exception_index = EXCP_ATOMIC;
    cpu_loop_exit_restore(cpu, pc);
}
