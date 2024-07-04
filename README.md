# QEMU for emulating BeagleBoard

This is a fork of https://git.linaro.org/qemu/qemu-linaro.git, which is an old fork of QEMU that can emulate an OMAP3530 BeagleBoard. It includes a cherry-picked compilation fix for modern glibc versions, a fix for compatibility with U-Boot SPL, and an updated URL for the `dtc` submodule.

## Compilation instructions, tested on Ubuntu 22.04:

- `git clone https://github.com/dougg3/qemu-linaro-beagleboard.git`
- `cd qemu-linaro-beagleboard`
- `git submodule update --init dtc`
- `./configure --python="$(which python2)" --target-list=arm-softmmu --disable-werror --prefix=$PWD/_install`
- `make -j $(nproc)`
- `make install`

Now you have an installation of this QEMU fork inside of the `_install` directory. Obviously you can also install it wherever else you want. I just picked `_install` for convenience because I wanted to tinker with it but not actually install it anywhere on my system.

## Run instructions:

Assuming you have a file called sdcard.img that is a raw SD card image suitable for booting a BeagleBoard, use the following command:

`./_install/bin/qemu-system-arm -M beagle -drive if=sd,cache=writeback,file=sdcard.img -m 256 -serial stdio`

## Notes:

- You may need to pad the sdcard.img file with a bunch of zeros at the end to make it longer if you get any kernel warnings about the size of the device being too small. I ran into this issue with a buildroot-generated sdcard.img.
