#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/brownieos.kernel isodir/boot/brownieos.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "brownieos" {
	multiboot /boot/brownieos.kernel
}
EOF
grub-mkrescue -o brownieos.iso isodir
