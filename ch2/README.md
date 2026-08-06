# Virtual machine scripts

These scripts manage the Debian QCOW2 images used to run the book's kernel
examples with QEMU. The kernel builds are expected in `../linux`, using
`build-arm64` for ARM64 and `build-riscv64` for RISCV64.

The examples below assume they are run from this directory.

The scripts that mount an image require `guestmount`, `guestunmount`, `sudo`,
and `make`. The image download script requires `wget`, and the QEMU scripts
require the corresponding `qemu-system-*` program.

## Get a Debian image

`get-debian-fs.sh` downloads a Debian nocloud QCOW2 image. By default it
downloads the Debian 13 ARM64 image into the current directory.

```bash
./get-debian-fs.sh
```

To download the RISCV64 image instead:

```bash
./get-debian-fs.sh --arch riscv64
```

## Run QEMU

The architecture-specific scripts boot the matching kernel build and Debian
image. They use 1024 MiB of memory by default and expose the guest console in
the terminal. SSH connections are forwarded from host port `10021` to guest
port `22`.

`qemu-arm64.sh` boots the ARM64 build. With no parameters it uses
`build-arm64/arch/arm64/boot/Image` and `./debian-13-nocloud-arm64.qcow2`.

```bash
./qemu-arm64.sh
```

`qemu-riscv64.sh` boots the RISCV64 build. With no parameters it uses
`build-riscv64/arch/riscv/boot/Image` and
`./debian-13-nocloud-riscv64.qcow2`.

```bash
./qemu-riscv64.sh
```

`qemu-ldddp.sh` is the architecture-independent wrapper. It requires the
architecture as its first argument; with no arguments it prints its help.

```bash
./qemu-ldddp.sh arm64
```

All QEMU scripts accept the same options for overriding the kernel, drive,
memory, or DTB. Extra QEMU arguments can be passed after `--`, for example:

```bash
./qemu-ldddp.sh riscv64 -- -S -s
```

## Install kernel modules

The installation scripts mount the corresponding Debian image and run
`modules_install` using the selected kernel build. The image is
unmounted automatically when the command finishes.

`install-mods-arm64.sh` uses `build-arm64` and the ARM64 image:

```bash
./install-mods-arm64.sh
```

`install-mods-riscv64.sh` uses `build-riscv64` and the RISCV64 image:

```bash
./install-mods-riscv64.sh
```

`install-mods-ldddp.sh` is the generic version. It requires the architecture
as its first argument; with no arguments it prints its help.

```bash
./install-mods-ldddp.sh arm64
```

The module scripts use `/mnt/vm` as their temporary mount point and expect it
not to be in use before they start.
