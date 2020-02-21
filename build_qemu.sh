#!/bin/bash
# QFlex consists of several software components that are governed by various
# licensing terms, in addition to software that was developed internally.
# Anyone interested in using QFlex needs to fully understand and abide by the
# licenses governing all the software components.

# ### Software developed externally (not by the QFlex group)

#     * [NS-3](https://www.gnu.org/copyleft/gpl.html)
#     * [QEMU](http://wiki.qemu.org/License)
#     * [SimFlex](http://parsa.epfl.ch/simflex/)

# ### Software developed internally (by the QFlex group)
# **QFlex License**

# QFlex
# Copyright (c) 2016, Parallel Systems Architecture Lab, EPFL
# All rights reserved.

# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:

#     * Redistributions of source code must retain the above copyright notice,
#       this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright notice,
#       this list of conditions and the following disclaimer in the documentation
#       and/or other materials provided with the distribution.
#     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
#       nor the names of its contributors may be used to endorse or promote
#       products derived from this software without specific prior written
#       permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
# EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# This function is called on in ERROR state
usage() {
    echo -e "\nUsage: $0 "
    echo -e "use -gdb to run the exact same command with debugging."
    echo -e "use -valgrind to run the exact same command with memcheck enabled."
    echo -e "use -ldow to run the exact same command, overwriting LD_LIBRARY_PATH for debugging purposes."
    echo -e "use -mult (--multiple) option to set up multiple instances (default: single instance)"
    echo -e "use -lo=snapshot, where snapshot is name of snapshot (default: boot)"
    echo -e "To use with Flexus trace add the -tr option"
    echo -e "To use with Flexus timing add the -timing option"
    echo -e "To run with icount in the guest, add the --enable_icount option"
    echo -e "To use single instance without user network use --no_net option"
    echo -e "To specify the user port for single-instance, use --unet-port"
    echo -e "To run multiple instances without NS3 use --no_ns3 opton"
    echo -e "To kill the qemu instances after the automated run add the --kill option"
    echo -e "You can use the -sf option to manually set the SIMULATE_TIME for Flexus (default $SIMULATE_TIME)"
    echo -e "Use the -uf option to specify the user file. e.g. -uf=user1.cfg to use user1.cfg (default: user.cfg)"
    echo -e "To name your log directory use the -exp=\"name\" or --experiment=\"name\". (default name: run)"
    echo -e "Use the -ow option to overwrite in an existing log directory"
    echo -e "Use the -sn option to take a snapshot with specified name"
    echo -e "Use the -set_quantum option to set a limit for the number of instructions executed per turn by each cpu."
    echo -e "Use the -pflash option to tell the script to add two pflash devices to the command line. Assumes they are defined in the user config file as PF0 and PF1."
    echo -e "Use the -rmc=\"NODE_ID\" option to tell the script to add an RMC component with node number <NODE_ID>"
    echo -e "Use the -kern option to use an extracted kernel and initrd image, which must be defined in the user.cfg file"
    echo -e "Use --extra=\"<>\" to add arguments to QEMU invocation"
    echo -e "Use -dbg=\"LVL\" to set the flexus debug level"
    echo -e "Use the -h option for help"
}

# Parse the dynamic options
for i in "$@"
do
    case $i in
        -timing)
        BUILD_TIMING="TRUE"
        shift
        ;;
        -emulation)
        BUILD_EMULATION="TRUE"
        shift
        ;;
        -install)
        INSTALL_DEPS="TRUE"
        shift
        ;;
        -h|--help)
        usage
        exit
        shift
        ;;
        *)
        echo "$0 : what do you mean by $i ?"
        usage
        exit
        ;;
    esac
done

set -x
set -e

# Check for test
export IS_EXTSNAP=`grep enable-extsnap configure`
export IS_QUANTUM=`grep enable-quantum configure`
export IS_PTH=`grep enable-pth configure`

if [[ "$IS_EXTSNAP" == "" ]] && [[ "$TEST_EXTSNAP" == "yes" ]]; then
    exit 0
fi

if [[ "$IS_QUANTUM" == "" ]] && [[ "$TEST_QUANTUM" == "yes" ]]; then
    exit 0
fi

if [[ "$IS_PTH" == "" ]] && [[ "$TEST_PTH" == "yes" ]]; then
    exit 0
fi

## 
if [ "${INSTALL_DEPS}" = "TRUE" ]; then
    sudo apt-get update -qq
    sudo apt-get install -y build-essential checkinstall wget python-dev \
        software-properties-common pkg-config zip zlib1g-dev unzip curl
    # Install dependencies
    sudo apt-get update
    sudo apt-get install -y build-essential checkinstall git-core libbz2-dev libtool expect bridge-utils uml-utilities
    sudo apt-get --no-install-recommends -y build-dep qemu
    # Install a compatible version of GCC
    sudo apt-get install python-software-properties
    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
    sudo apt-get update
    sudo apt-get -y install gcc-${GCC_VERSION}
    sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-${GCC_VERSION} 20
    # Install pth
    wget ftp://ftp.gnu.org/gnu/pth/pth-2.0.7.tar.gz
    tar -xvf pth-2.0.7.tar.gz
    cd pth-2.0.7
    sed -i 's#$(LOBJS): Makefile#$(LOBJS): pth_p.h Makefile#' Makefile.in
    sudo ./configure --with-pic --prefix=/usr --mandir=/usr/share/man
    sudo make
    sudo make test
    sudo make install
    cd ..
    git submodule update --init dtc
fi

# Build Qemu for emulation, or timing
if [ "${BUILD_EMULATION}" = "TRUE" ]; then
    export CFLAGS="-fPIC"
    ./config.emulation
    make clean && make -j8
elif [ "${BUILD_TIMING}" = "TRUE" ]; then
    export CFLAGS="-fPIC"
    ./config.timing
    make clean && make -j8
fi
