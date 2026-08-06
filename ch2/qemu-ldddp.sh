#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
KERNEL_DIR="${LDDDP_LINUX_DIR:-$SCRIPT_DIR/../linux}"
MEMORY="1024"
DTB=""
DUMP_DTB=""
QEMU_EXTRA=()

print_help() {
    cat <<EOF
Usage: $0 <arm64|riscv64> [options]

Run QEMU for the LDDDP book examples on ARM64 or RISC-V64.

Options:
  -k, --kernel <path>     Path to kernel Image (default: per-arch)
  -d, --drive <file>      Path to qcow2 drive image (default: per-arch)
  -m, --memory <size>     Memory size, e.g. 1024, 1024M, 2G (default: $MEMORY)
  -D, --dtb <file>        Provide DTB file (default: none)
  --dump-dtb <file>       Dump the generated QEMU DTB and exit
  -h, --help              Show this help and exit

Any unknown options are forwarded directly to QEMU.

Examples:
  $0 arm64 -m 2G
  $0 riscv64 -- -S -s
EOF
}

ARCH="${1:-}"
if [[ -n "$ARCH" ]]; then
    shift
fi

case "$ARCH" in
    arm64)
        KERNEL="$KERNEL_DIR/build-arm64/arch/arm64/boot/Image"
        DRIVE="./debian-13-nocloud-arm64.qcow2"
        QEMU="qemu-system-aarch64"
        APPEND="root=/dev/vda1 console=ttyAMA0"
        CPU=( -cpu cortex-a72 )
        ;;
    riscv64)
        KERNEL="$KERNEL_DIR/build-riscv64/arch/riscv/boot/Image"
        DRIVE="./debian-13-nocloud-riscv64.qcow2"
        QEMU="qemu-system-riscv64"
        APPEND="root=/dev/vda1 rw console=ttyS0 audit=0"
        CPU=()
        ;;
    -h|--help|"")
        print_help
        exit 0
        ;;
    *)
        echo "Unknown architecture \"$ARCH\". Supported: arm64, riscv64" >&2
        print_help >&2
        exit 1
        ;;
esac

HELP=0
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

if [[ "$HELP" -eq 1 ]]; then
    print_help
    exit 0
fi

CMD=(
    "$QEMU"
    -M virt
    "${CPU[@]}"
    -m "$MEMORY"
    -kernel "$KERNEL"
    -append "$APPEND"
    -drive "file=$DRIVE,if=virtio"
    -netdev user,id=net0,hostfwd=tcp::10021-:22
    -device virtio-net-device,netdev=net0
    -nographic
)

if [[ -n "$DTB" ]]; then
    CMD+=( -dtb "$DTB" )
fi

if [[ -n "$DUMP_DTB" ]]; then
    CMD+=( -machine "dumpdtb=$DUMP_DTB" )
fi

CMD+=("${QEMU_EXTRA[@]}")

exec "${CMD[@]}"
