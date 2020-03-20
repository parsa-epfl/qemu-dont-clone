# KnockoutKraken QEMU

This is the instrumented version of QEMU used by the the KnockoutKraken simulator. KnockoutKraken brings FPGA-accelerated simulation to the QFlex family.

KnockoutKraken is composed of three main components: a modified version of QEMU, an instrumented ARM softcore (ARMFlex), and a driver that handles the communication between QEMU and ARMFlex. The vast majority of developers will work on QEMU and/or ARMFlex. QEMU is written in C and can be developed in most Linux machines. ARMFlex is written in Chisel, and while basic testing can be done in most Linux machines, fully simulating and synthesizing the softcore requires an extensive toolchain.

In the following section, we will describe the process to build QEMU for KnockoutKraken

# Building QEMU

To build QEMU, first download all the dependencies:
```
$ sudo apt-get update -qq
$ sudo apt-get install -y build-essential checkinstall wget sudo \
                          python-dev software-properties-common \
                          pkg-config zip zlib1g-dev unzip libbz2-dev \
                          libtool python-software-properties git-core

$ sudo apt-get --no-install-recommends -y build-dep qemu
```

We use git-lfs to store and share QEMU images. Please refer to this page to install git-lfs.

Now, configure and build QEMU:
```
$ cd ../qemu
$ export CFLAGS="-fPIC"
$ ./configure --target-list=aarch64-softmmu --python=/usr/bin/python2 --enable-extsnap \
     --enable-fa-qflex --extra-cflags=-std=gnu99 --enable-fpga
$ make -j4
```

After the build process is complete, you are ready to experiment with QEMU. Download an image [here](https://github.com/parsa-epfl/images/tree/matmul-knockoutkraken) and give it a test!
