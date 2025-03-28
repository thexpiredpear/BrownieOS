#!/bin/sh
set -e
. ./clean.sh
. ./iso.sh
if [ $1 -eq 1 ]
then
 qemu-system-$(./target-triplet-to-arch.sh $HOST) -smp 2 -s -S -cdrom brownieos.iso &
 gdb -ex "target remote localhost:1234" -ex "symbol-file sysroot/boot/brownieos.kernel" -ex "layout src"
elif [ $1 -eq 2 ]
then
 qemu-system-$(./target-triplet-to-arch.sh $HOST) -smp 2 -cdrom brownieos.iso -monitor stdio
else
 qemu-system-$(./target-triplet-to-arch.sh $HOST) -smp 2 -cdrom brownieos.iso
fi
