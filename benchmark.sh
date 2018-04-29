#!/bin/bash
BIN_PATH=/mnt/qemu/aarch64-softmmu/qemu-system-aarch64
SRC_PATH=/mnt/qemu
IMAGE_PATH=/mnt/qemu/images

$BIN_PATH -machine virt -cpu cortex-a57 -smp 4 -m 128G -kernel $IMAGE_PATH/vmlinuz-4.4.0-83-generic \
    -append 'console=ttyAMA0 root=/dev/sda2' -initrd $IMAGE_PATH/initrd.img-4.4.0-83-generic \
    -nographic -rtc driftfix=slew -exton -netdev user,id=net1,hostfwd=tcp::2220-:22 \
    -device virtio-net-device,mac=52:54:00:00:00:00,netdev=net1 -global virtio-blk-device.scsi=off \
    -device virtio-scsi-device,id=scsi -drive file=$IMAGE_PATH/ubuntu-16.04-lts-blank.qcow2,id=rootimg,cache=unsafe,if=none \
    -device scsi-hd,drive=rootimg -qmp unix:/tmp/qmp-sock,server,nowait > qemu-out.log &

sleep 200

sshpass -p "cloudsuite" scp -P 2220 ./stress-ng cloudsuite@localhost:~

sshpass -p "cloudsuite" ssh -p 2220 cloudsuite@localhost "nohup ~/stress-ng --vm 1 --vm-bytes 512M --vm-method inc-nybble -t 1000m > /dev/null 2>&1 &"

i=0

while [ $i -lt 500 ]
do
    echo "savevm-ext nocomp$i" | $SRC_PATH/scripts/qmp/qmp-shell -H /tmp/qmp-sock
    sleep 7
    i=$[$i+1]
done

killall qemu-system-aarch64
