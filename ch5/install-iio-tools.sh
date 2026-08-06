#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
KERNEL_DIR="${LDDDP_LINUX_DIR:-$SCRIPT_DIR/../linux}"

usage()
{
    echo "Usage: $0 [-i] <arm64|riscv64>"
    echo
    echo "Compile IIO tools for the selected architecture."
    echo "The Linux source tree defaults to: $SCRIPT_DIR/../linux"
    echo "Set LDDDP_LINUX_DIR to use a different Linux source tree."
    echo
    echo "Options:"
    echo "  -i    Install the generated executables on qemu-vm"
    echo "  -h    Show this help"
}

die()
{
    echo "Error: $*" >&2
    exit 1
}

INSTALL=0

if [[ "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

while getopts "ih" opt; do
    case "$opt" in
        i)
            INSTALL=1
            ;;
        h)
            usage
            exit 0
            ;;
        *)
            usage >&2
            exit 1
            ;;
    esac
done

shift $((OPTIND - 1))

if [ "$#" -ne 1 ]; then
    usage >&2
    exit 1
fi

ARCH="$1"

case "$ARCH" in
    arm64)
        TARGET="aarch64-linux-gnu"
        BUILD_DIR="$KERNEL_DIR/build-arm64"
        ;;
    riscv64)
        TARGET="riscv64-linux-gnu"
        BUILD_DIR="$KERNEL_DIR/build-riscv64"
        ;;
    *)
        echo "Error: unsupported architecture: $ARCH" >&2
        echo "Supported architectures: arm64, riscv64" >&2
        exit 1
        ;;
esac

for command in make clang; do
    command -v "$command" >/dev/null 2>&1 ||
        die "required command not found: $command"
done

if [ "$INSTALL" -eq 1 ]; then
    for command in scp ssh; do
        command -v "$command" >/dev/null 2>&1 ||
            die "required command not found: $command"
    done
fi

[[ -f "$KERNEL_DIR/tools/iio/Makefile" ]] ||
    die "Linux source tree not found: $KERNEL_DIR"
[[ -d "$BUILD_DIR" ]] ||
    die "kernel build directory not found: $BUILD_DIR"

make -C "$KERNEL_DIR/tools/iio" \
    O="$BUILD_DIR" \
    ARCH="$ARCH" \
    LLVM=1 \
    CC="clang --target=$TARGET"

if [ "$INSTALL" -eq 1 ]; then
    for program in iio_event_monitor iio_generic_buffer lsiio; do
        [[ -x "$BUILD_DIR/$program" ]] ||
            die "built executable not found: $BUILD_DIR/$program"
        scp "$BUILD_DIR/$program" qemu-vm:/tmp/
    done

    ssh qemu-vm \
        sudo install -m 0755 \
        /tmp/iio_event_monitor /tmp/iio_generic_buffer /tmp/lsiio \
        /usr/local/bin/
fi

echo
echo "Successfully completed."

if [ "$INSTALL" -eq 1 ]; then
    echo "IIO tools installed in qemu-vm:/usr/local/bin/"
fi
