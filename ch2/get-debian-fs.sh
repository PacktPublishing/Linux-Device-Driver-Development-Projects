#!/usr/bin/env bash
set -euo pipefail

ARCH="arm64"
RELEASE="13"
OUTDIR="."
TMPFILE=""

die() {
    echo "Error: $*" >&2
    exit 1
}

print_help() {
    cat <<EOF
Usage: $0 [options]

Download a Debian nocloud QCOW2 image.

Options:
  -a, --arch <arch>       Architecture (default: $ARCH)
  -r, --release <rel>     Release number or codename (default: $RELEASE)
  -o, --output <dir>      Output directory (default: $OUTDIR)
  -h, --help              Show this help and exit
EOF
}

cleanup() {
    if [[ -n "$TMPFILE" ]]; then
        rm -f -- "$TMPFILE"
    fi
}

trap cleanup EXIT

while [[ "$#" -gt 0 ]]; do
    case "$1" in
        -a|--arch)
            [[ "$#" -ge 2 ]] || die "missing argument for $1"
            ARCH="$2"
            shift 2
            ;;
        -r|--release)
            [[ "$#" -ge 2 ]] || die "missing argument for $1"
            RELEASE="$2"
            shift 2
            ;;
        -o|--output)
            [[ "$#" -ge 2 ]] || die "missing argument for $1"
            OUTDIR="$2"
            shift 2
            ;;
        -h|--help)
            print_help
            exit 0
            ;;
        --)
            shift
            [[ "$#" -eq 0 ]] || die "unexpected argument: $1"
            ;;
        *)
            die "unknown option: $1"
            ;;
    esac
done

command -v wget >/dev/null 2>&1 || die "required command not found: wget"

case "$ARCH" in
    arm64|riscv64)
        ;;
    *)
        die "unsupported architecture: $ARCH (supported: arm64, riscv64)"
        ;;
esac

VERSION=""
CODENAME=""

case "$RELEASE" in
    12)
        VERSION="12"
        CODENAME="bookworm"
        ;;
    13)
        VERSION="13"
        CODENAME="trixie"
        ;;
    bookworm)
        VERSION="12"
        CODENAME="bookworm"
        ;;
    trixie)
        VERSION="13"
        CODENAME="trixie"
        ;;
    *)
        die "unsupported release: $RELEASE (supported: 12, 13, bookworm, trixie)"
        ;;
esac

URL="https://cloud.debian.org/images/cloud/${CODENAME}/latest/debian-${VERSION}-nocloud-${ARCH}.qcow2"
OUTFILE="${OUTDIR}/debian-${VERSION}-nocloud-${ARCH}.qcow2"

mkdir -p -- "$OUTDIR"
TMPFILE="$(mktemp "${OUTFILE}.XXXXXX")"
wget -O "$TMPFILE" "$URL"
mv -- "$TMPFILE" "$OUTFILE"
TMPFILE=""

echo "Downloaded $OUTFILE"
