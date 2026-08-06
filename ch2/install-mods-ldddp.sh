#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
KERNEL_DIR="${LDDDP_LINUX_DIR:-$SCRIPT_DIR/../linux}"
MNT="/mnt/vm"
MOUNTED=0

print_help() {
    cat <<EOF
Usage: $0 <arm64|riscv64>

Install the modules from an out-of-tree kernel build into the
corresponding Debian QEMU image.
EOF
}

ARCH="${1:-}"
case "$ARCH" in
    arm64)
        BUILD_DIR="$KERNEL_DIR/build-arm64"
        IMG="$SCRIPT_DIR/debian-13-nocloud-arm64.qcow2"
        MAKE_ARCH="arm64"
        ;;
    riscv64)
        BUILD_DIR="$KERNEL_DIR/build-riscv64"
        IMG="$SCRIPT_DIR/debian-13-nocloud-riscv64.qcow2"
        MAKE_ARCH="riscv"
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

die() {
    echo "Error: $*" >&2
    exit 1
}

cleanup() {
    if [[ "$MOUNTED" -eq 1 ]]; then
        sudo guestunmount "$MNT" 2>/dev/null || true
    fi
}

trap cleanup EXIT

for command in sudo guestmount guestunmount make mountpoint; do
    command -v "$command" >/dev/null 2>&1 ||
        die "required command not found: $command"
done

[[ -f "$IMG" ]] || die "disk image not found: $IMG"
[[ -f "$BUILD_DIR/.config" ]] ||
    die "kernel configuration not found: $BUILD_DIR/.config"
[[ -f "$BUILD_DIR/modules.order" ]] ||
    die "kernel modules have not been built in $BUILD_DIR"

if mountpoint -q "$MNT"; then
    die "mount point is already in use: $MNT"
fi

sudo mkdir -p "$MNT"
sudo guestmount -a "$IMG" -i --rw "$MNT"
MOUNTED=1

sudo make -C "$KERNEL_DIR" \
    O="${BUILD_DIR#$KERNEL_DIR/}" \
    ARCH="$MAKE_ARCH" \
    LLVM=1 \
    INSTALL_MOD_PATH="$MNT" \
    modules_install
