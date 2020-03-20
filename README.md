QFlex - Modified QEMU Documentation
=================================

For the purposes of serving as a performance model for modern processors, 
QFlex integrates its own version of QEMU with additional features.

This additional documentation describes the process of building our
modified version of QEMU, as well as the various features we added.

## Building QEMU
We provide the `build_qemu.sh` script for convenience. It has four runtime options:
* `-install` automatically installs all of the dependencies required by QEMU. The script has been tested on two operating systems - Ubuntu 18.04 LTS and CentOS 7.
* `-emulation` builds QEMU in only processor emulation mode. For the default configuration parameters, see the file `config.emulation`.
* `-trace` and `-timing` build QEMU with support for trace- and timing-mode simulations of a modern multi-core processor, respectively. Both configurations also have their own respective `config.<>` files.

## Additional QEMU Features
The original version of QEMU is heavily optimized to emulate processors at near-native speeds.
Our goal when building QFlex was to keep this ability intact (thus enabling workloads to be ported at "real-time") while adding key features that are required for an architectural simulator.
We identified four key requirements:
1. External snapshots -> The ability to take (micro)+architectural snapshots in a programmatic way and store them in a directory structure that is _external_ to the raw image file itself. This is important for the ability to share snapshots between collaborators, and to facilitate micro-architectural snapshots of CPU-internal structures.
2. Determinism -> The emulator's state should not depend on the host's scheduler algorithm. Otherwise, this could lead to different execution paths being taken depending on other co-running processes in the host system.
3. Bounded execution skew between guest vCPUs -> When simulating the guest CPU, the cores should make progress at _roughly_ the same rate without a massive skew being created between them.
4. Timing-first execution -> To model modern speculative OoO cores and cache hierarchies, a new API must be added where QEMU only advances its state under control of the timing model.

To address each of these issues, we add the following functionality to QEMU. Each feature has its own configuration flag which can enable/disable it at compile time (and some can also be controlled at runtime).
1. Extsnap
  * This feature is enabled with the compile time flag `--enable-extsnap`.
  * We use the overlay and VMState migration features already present in QEMU to stream the current machine state to a named directory present in the same place as the root disk image.
  * To use this feature, enter the QEMU monitor however you choose and enter the command `savevm-ext <snapshot name>`. This creates a directory named `<snapshot name>` in the directory /path/to/image.
  * Our set of Captain scripts provide a parameter to specify the snapshot name to load (or, it can be done at the regular QEMU command prompt using the flag `-loadext=<snapshot name>`.
2. Using GNU PTH as a backend for QEMU threads
  * Instead of using Pthreads and the regular host scheduler to multiplex between QEMU's many threads, which is nondeterministic, we integrate the GNU PTH user-level threading library allowing each QEMU thread to be scheduled in deterministic fashion.
  * This feature is enabled by using `--enable-pth` and specifying the path to its libraries with `--pth-path=/path/to/pth`. 
  * The `build_qemu.sh` script automatically builds and installs pth in your `$HOME` directory.
3. Quantum -> When simulating multiple VCPUs, we provide a feature to force QEMU to swap the active CPU every N guest instructions.
  * This feature is enabled by passing `--enable-quantum`.
  * NOTE: This feature is incompatible with QEMU MTTCG mode.
4. Libqflex -> We provide an API in the "libqflex" submodule which gives the functionality for any of the underlying "Kraken" family of simulators to advance QEMU one instruction at a time.
  * The APIs are compiled in by using the `--enable-flexus` flag.

## Features still under development
QFlex is still actively being developed. In particular, we are working on the extsnap and libqflex features to enable the guest to be restored at the exact program counter value it left off at when the snapshot was taken - currently, CPUs often restart in kernel mode, execute the bottom half of an IRQ handler, and then return to the program counter at snapshot time.

## Contributing and Code Maintainer(s)
We encourage you to submit your contributions and issues here on GitHub using pull requests and issues.

The current code maintainer for QFlex's modified QEMU is:
Mark Sutherland, PhD Candidate, EPFL, Switzerland

If you have questions about the functionality or architecture of this system, feel free to contact him at <firstname.lastname@epfl.ch>.

## Licences
If you modify any files in this directory, you must place the unmodified QFlex licence (available in the parent QFlex directory) on the top of the modified files.
This is required to comply with the terms of the GPL which covers the original QEMU licence.
         
         
Original QEMU Documentation
===========================
         
         QEMU README
         ===========

QEMU is a generic and open source machine & userspace emulator and
virtualizer.

QEMU is capable of emulating a complete machine in software without any
need for hardware virtualization support. By using dynamic translation,
it achieves very good performance. QEMU can also integrate with the Xen
and KVM hypervisors to provide emulated hardware while allowing the
hypervisor to manage the CPU. With hypervisor support, QEMU can achieve
near native performance for CPUs. When QEMU emulates CPUs directly it is
capable of running operating systems made for one machine (e.g. an ARMv7
board) on a different machine (e.g. an x86_64 PC board).

QEMU is also capable of providing userspace API virtualization for Linux
and BSD kernel interfaces. This allows binaries compiled against one
architecture ABI (e.g. the Linux PPC64 ABI) to be run on a host using a
different architecture ABI (e.g. the Linux x86_64 ABI). This does not
involve any hardware emulation, simply CPU and syscall emulation.

QEMU aims to fit into a variety of use cases. It can be invoked directly
by users wishing to have full control over its behaviour and settings.
It also aims to facilitate integration into higher level management
layers, by providing a stable command line interface and monitor API.
It is commonly invoked indirectly via the libvirt library when using
open source applications such as oVirt, OpenStack and virt-manager.

QEMU as a whole is released under the GNU General Public License,
version 2. For full licensing details, consult the LICENSE file.


Building
========

QEMU is multi-platform software intended to be buildable on all modern
Linux platforms, OS-X, Win32 (via the Mingw64 toolchain) and a variety
of other UNIX targets. The simple steps to build QEMU are:

  mkdir build
  cd build
  ../configure
  make

Additional information can also be found online via the QEMU website:

  http://qemu-project.org/Hosts/Linux
  http://qemu-project.org/Hosts/Mac
  http://qemu-project.org/Hosts/W32


Submitting patches
==================

The QEMU source code is maintained under the GIT version control system.

   git clone git://git.qemu-project.org/qemu.git

When submitting patches, the preferred approach is to use 'git
format-patch' and/or 'git send-email' to format & send the mail to the
qemu-devel@nongnu.org mailing list. All patches submitted must contain
a 'Signed-off-by' line from the author. Patches should follow the
guidelines set out in the HACKING and CODING_STYLE files.

Additional information on submitting patches can be found online via
the QEMU website

  http://qemu-project.org/Contribute/SubmitAPatch
  http://qemu-project.org/Contribute/TrivialPatches


Bug reporting
=============

The QEMU project uses Launchpad as its primary upstream bug tracker. Bugs
found when running code built from QEMU git or upstream released sources
should be reported via:

  https://bugs.launchpad.net/qemu/

If using QEMU via an operating system vendor pre-built binary package, it
is preferable to report bugs to the vendor's own bug tracker first. If
the bug is also known to affect latest upstream code, it can also be
reported via launchpad.

For additional information on bug reporting consult:

  http://qemu-project.org/Contribute/ReportABug


Contact
=======

The QEMU community can be contacted in a number of ways, with the two
main methods being email and IRC

 - qemu-devel@nongnu.org
   http://lists.nongnu.org/mailman/listinfo/qemu-devel
 - #qemu on irc.oftc.net

Information on additional methods of contacting the community can be
found online via the QEMU website:

  http://qemu-project.org/Contribute/StartHere

-- End
