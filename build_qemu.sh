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
# Get images
mkdir images
cd images
mkdir ubuntu-16.04-blank
cd ubuntu-16.04-blank
wget https://github.com/parsa-epfl/images/blob/stripped/ubuntu-16.04-blank/ubuntu-stripped-comp3.qcow2?raw=true -O ubuntu-stripped-comp3.qcow2
wget https://github.com/parsa-epfl/images/blob/stripped/ubuntu-16.04-blank/initrd.img-4.4.0-83-generic?raw=true -O initrd.img-4.4.0-83-generic
wget https://github.com/parsa-epfl/images/blob/stripped/ubuntu-16.04-blank/vmlinuz-4.4.0-83-generic?raw=true -O vmlinuz-4.4.0-83-generic
cd ../..
git submodule update --init dtc
# Build Qemu
export CFLAGS="-fPIC"
./configure --target-list=$TARGET_LIST $CONFIG --disable-werror --disable-tpm
make -j4
