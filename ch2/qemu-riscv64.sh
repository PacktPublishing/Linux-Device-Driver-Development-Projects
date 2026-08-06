#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
KERNEL_DIR="${LDDDP_LINUX_DIR:-$SCRIPT_DIR/../linux}"
KERNEL="$KERNEL_DIR/build-riscv64/arch/riscv/boot/Image"
DRIVE="./debian-13-nocloud-riscv64.qcow2"
MEMORY="1024"
DTB=""
DUMP_DTB=""
HELP=0
QEMU_EXTRA=()

print_help() {
    cat <<EOF
Usage: $0 [options] [-- <extra qemu args>]

Run QEMU RISC-V64 with optional custom paths.

Options:
  -k, --kernel <path>     Path to kernel Image (default: $KERNEL)
  -d, --drive <file>      Path to qcow2 drive image (default: $DRIVE)
  -m, --memory <size>     Memory size, e.g. 1024, 1024M, 2G (default: $MEMORY)
  -D, --dtb <file>        Provide DTB file (default: none)
  --dump-dtb <file>       Dump the generated QEMU DTB and exit
  -h, --help              Show this help and exit

Any unknown options are forwarded directly to QEMU.

Examples:
  $0 -m 2G -S -s
  $0 -- -S -s -d guest_errors
EOF
}

while [[ "$#" -gt 0 ]]; do
    case "$1" in
        -k|--kernel)    KERNEL="$2"; shift 2;;
        -d|--drive)     DRIVE="$2"; shift 2;;
        -m|--memory)    MEMORY="$2"; shift 2;;
        -D|--dtb)       DTB="$2"; shift 2;;
        --dump-dtb)     DUMP_DTB="$2"; shift 2;;
        -h|--help)      HELP=1; shift;;
        --)             shift; QEMU_EXTRA+=("$@"); break;;
        *)              QEMU_EXTRA+=("$1"); shift;;
    esac
done

if [ "$HELP" -eq 1 ]; then
    print_help
    exit 0
fi

DTB_ARG=()
if [[ -n "$DTB" ]]; then
    DTB_ARG=(-dtb "$DTB")
fi

# If you have problems with the boot-efi mount, use this:
#
# APPEND="root=/dev/vda1 rw console=ttyS0 audit=0 \
#         systemd.mask=boot-efi.mount"

APPEND="root=/dev/vda1 rw console=ttyS0 audit=0"

CMD=(
    qemu-system-riscv64
    -M virt
    -m "$MEMORY"
    -kernel "$KERNEL"
    -append "$APPEND"
    -drive "file=$DRIVE,if=virtio"
    -netdev user,id=net0,hostfwd=tcp::10021-:22
    -device virtio-net-device,netdev=net0
    -nographic
)

if [[ ${#DTB_ARG[@]} -gt 0 ]]; then
    CMD+=("${DTB_ARG[@]}")
fi

if [[ -n "$DUMP_DTB" ]]; then
    CMD+=(-machine "dumpdtb=$DUMP_DTB")
fi

CMD+=("${QEMU_EXTRA[@]}")

exec "${CMD[@]}"
