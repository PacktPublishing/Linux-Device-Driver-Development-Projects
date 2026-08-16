# Linux Device Driver Development Projects

This repository accompanies the book *Linux Device Driver Development Projects* by Javier Carrasco, published by Packt.

It contains the source code, examples, and supporting material used throughout the book. The repository is organized by chapter, making it easy to follow along while reading and experiment with the concepts covered in each project.

## Building Linux

The scripts in `virtual-machine` (a link to `ch2`) expect the Linux source tree in the repository root, at
`linux/`. The kernel can be built outside the source tree with separate output
directories for each architecture:

```bash
make -C linux O=build-arm64 ARCH=arm64 LLVM=1
make -C linux O=build-riscv64 ARCH=riscv LLVM=1
```

The scripts use `build-arm64` and `build-riscv64` by default.

If the Linux tree is located elsewhere, set `LDDDP_LINUX_DIR` when invoking a script:

```bash
LDDDP_LINUX_DIR=/path/to/linux ./virtual-machine/qemu-ldddp.sh arm64
```

## Troubleshooting

### Out of space when installing kernel modules

If the Debian image runs out of space after installing the kernel modules, remember that the `modules_install` target installs all modules from the selected kernel build, not just the driver currently being tested. It installs them below `/lib/modules/$(KERNELRELEASE)`. A new directory, such as `7.2-g<commit>`, is created when `KERNELRELEASE` changes, commonly because `CONFIG_LOCALVERSION_AUTO` adds the current Git commit to the kernel version. Timestamps alone do not create a new directory; the same release directory is reused.

To reduce the space used by the installed modules, enable module compression in the kernel configuration:

```text
CONFIG_MODULE_COMPRESS=y
CONFIG_MODULE_COMPRESS_XZ=y
CONFIG_MODULE_COMPRESS_ALL=y
```

XZ provides the highest compression ratio of the module compression options, and depending on the modules, can reduce their size by up to around 10x. The modules are installed as `.ko.xz` files instead of `.ko` files and can still be loaded normally with `modprobe`; no additional tools are required on a standard Debian system.

The main drawbacks are that `modules_install` takes longer because every module has to be compressed, and loading a module requires it to be decompressed first, which consumes some CPU cycles. In practice, this overhead is negligible, especially in our QEMU-based development environment where reducing the size of the disk image is more important than the small additional CPU cost.

To reclaim space, remove old release directories from `/lib/modules/`, keeping the one reported by `uname -r`:

```bash
uname -r
sudo du -sh /lib/modules/*
sudo rm -rf /lib/modules/<old-kernel-release>
```

When only the driver is being changed and the running kernel release and configuration remain the same, you can copy the rebuilt module instead of running `modules_install` again:

```bash
scp linux/build-arm64/drivers/iio/chemical/ldddp_iio_co2.ko user@vm:/tmp/
sudo cp /tmp/ldddp_iio_co2.ko /lib/modules/$(uname -r)/updates/
sudo depmod -a
```

Use the corresponding `build-riscv64` path for RISC-V. If the kernel release or its ABI has changed, install the complete module set and boot the matching kernel instead.

## Notes

* The solutions provided for each project and the additional tasks are intended as guidance for the reader. They represent one possible approach, but other valid solutions may exist. As the Linux kernel continues to evolve, more efficient or elegant alternatives may also become available.

* The provided scripts automate common tasks, such as launching QEMU or downloading Debian cloud images. Feel free to adapt them to suit your own workflow and requirements.

* If you find any errors or have suggestions for improvements, contributions to this repository are always welcome.

Thank you, and I hope you enjoy the learning journey!
