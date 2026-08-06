#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
KERNEL_DIR="${LDDDP_LINUX_DIR:-$SCRIPT_DIR/../linux}"
BUILD_DIR="$KERNEL_DIR/build-riscv64"
IMG="$SCRIPT_DIR/debian-13-nocloud-riscv64.qcow2"
MNT="/mnt/vm"
MOUNTED=0

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
    O=build-riscv64 \
    ARCH=riscv \
    LLVM=1 \
    INSTALL_MOD_PATH="$MNT" \
    modules_install
