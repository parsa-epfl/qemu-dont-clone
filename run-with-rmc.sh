#!/bin/sh
#sudo aarch64-softmmu/qemu-system-aarch64 -M virt -m 1G -cpu cortex-a57 -nographic \
sudo aarch64-softmmu/qemu-system-aarch64 -enable-kvm -M virt -m 16G -cpu host -smp 4 -nographic \
    -global virtio-blk-device.scsi=off -device virtio-scsi-device,id=scsi -rtc driftfix=slew \
    -drive if=none,file=/proj/cloudsuite3-PG0/images/ubuntu-kvm-sonuma.qcow2,id=hd0 \
    -device scsi-hd,drive=hd0 -device virtio-scsi-device \
    -pflash $HOME/images/flash0.img \
    -pflash $HOME/images/flash1.img \
    -netdev user,id=net1,hostfwd=tcp::5555-:22 -device virtio-net-device,mac=52:54:00:00:02:12,netdev=net1 \
    -device rmc,nid=2,cid=0

#-hda /home/msutherl/qflex/images/ubuntu18-x86_64-sonuma/ubuntu18-sonuma-x86_64.qcow2 -nographic -serial pty -monitor stdio -device e1000,netdev=net0 -netdev user,id=net0,hostfwd=tcp::5555-:22 -device rmc,nid=2,cid=0 -net nic,model=ne2k_pci -net tap,ifname=tap1L,script=no,downscript=no

# Run to multiplex QEMU monitor and serial port to stdio (uses the same host terminal)
#sudo x86_64-softmmu/qemu-system-x86_64 -m 16G -smp 2 -hda /home/msutherl/qflex/images/ubuntu18-x86_64-sonuma/ubuntu18-sonuma-x86_64.qcow2 -nographic -serial mon:stdio -device e1000,netdev=net0 -netdev user,id=net0,hostfwd=tcp::5555-:22 -device rmc,nid=2,cid=0 -net nic,model=ne2k_pci -net tap,ifname=tap1L,script=no,downscript=no
