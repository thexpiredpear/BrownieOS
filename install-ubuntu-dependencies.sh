# SCRIPT TO EASY INSTALL i686-elf CROSS COMPILER
# ONLY TESTED ON UBUNTU 22.04.3 LTS
# GCC AND BINUTILS VERSIONS CONFIRMED COMPATIBLE, UPDATE IF YOU LIKE
# gcc dependencies
sudo apt install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo libisl-dev qemu-system-i386 xorriso grub-pc-bin
# create dir to build cross compiler
cd ~
mkdir build-cross
cd build-cross
# downlooad binutils and gcc src - confirmed to work, use updated versions if you like
wget https://ftp.gnu.org/gnu/binutils/binutils-2.41.tar.xz
wget https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.xz
# unzip src
tar -xvf binutils-2.41.tar.xz
tar -xvf gcc-31.2.0.tar.xz
# env variables for convenience
# install binutils and gcc into ~/opt/cross
export PREFIX="$HOME/opt/cross"
# target triplet
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"
# create dir for building
mkdir build-binutils
cd build-binutils
# create makefile
../binutils-2.41/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
# install
make
make install
cd ..
# create dir for building
mkdir build-gcc
cd build-gcc
# create makefile
../gcc-13.2.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
# install
make all-gcc
make all-target 
make install-gcc
make install-target-libgcc
# add to path
export PATH="$HOME/opt/cross/bin:$PATH"
echo 'export PATH="$HOME/opt/cross/bin:$PATH"' >> ~/.bashrc
echo 'export PATH="$HOME/opt/cross/bin:$PATH"' >> ~/.profile
