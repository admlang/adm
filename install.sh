#!/bin/sh
# Installs ADM on Linux for the current user:
#
#   curl -fsSL https://raw.githubusercontent.com/admlang/adm/main/install.sh | sh
#
# Downloads the latest language package from the GitHub releases, unpacks it
# into ~/.adm (bin/, lib/, tools/) and puts ~/.adm/bin on PATH. No sudo.
#
#   ADM_VERSION=0.1      install that release instead of the latest
#   ADM_INSTALL_DIR=...  install somewhere other than ~/.adm
#   ADM_REPO=owner/name  download from another repository's releases
#   ADM_PACKAGE=...      install this package (a local .tar.gz or a URL) instead
#                        of looking up a release
#
# Everything installed is listed in <install dir>/install-manifest, which
# uninstall.sh reads to remove exactly that and nothing else.

# Everything sits in main() so a truncated download cannot run half a script.
main() {
set -eu

red="$( (tput bold || :; tput setaf 1 || :) 2>/dev/null)"
plain="$( (tput sgr0 || :) 2>/dev/null)"
status() { echo ">>> $*" >&2; }
error() { echo "${red}ERROR:${plain} $*" >&2; exit 1; }
warn() { echo "${red}WARNING:${plain} $*" >&2; }

REPO="${ADM_REPO:-admlang/adm}"
INSTALL_DIR="${ADM_INSTALL_DIR:-$HOME/.adm}"

available() { command -v "$1" >/dev/null 2>&1; }
for tool in curl tar grep sed; do
    available "$tool" || error "$tool is required but missing"
done

OS="$(uname -s)"
[ "$OS" = "Linux" ] || error "only Linux packages are published today (got $OS)"
ARCH="$(uname -m)"
case "$ARCH" in
    x86_64) ARCH=amd64 ;;
    aarch64|arm64) ARCH=arm64 ;;
    *) error "unsupported architecture: $ARCH" ;;
esac

# The package is built against glibc 2.35 (docker/package): musl systems and
# older glibc cannot run it.
if ldd --version 2>&1 | grep -qi musl || ls /lib/ld-musl-* >/dev/null 2>&1; then
    error "this system uses musl libc (Alpine?); ADM needs a glibc-based Linux (glibc 2.35 or newer)"
fi
GLIBC="$(getconf GNU_LIBC_VERSION 2>/dev/null | awk '{print $2}')"
if [ -n "$GLIBC" ]; then
    major="${GLIBC%%.*}"
    minor="${GLIBC#*.}"
    minor="${minor%%.*}"
    if [ "$major" -lt 2 ] || { [ "$major" -eq 2 ] && [ "$minor" -lt 35 ]; }; then
        error "glibc $GLIBC is too old; ADM needs glibc 2.35 or newer (Ubuntu 22.04, Debian 12, Mint 21, Fedora 36 or later)"
    fi
fi

TEMP_DIR="$(mktemp -d)"
cleanup() { rm -rf "$TEMP_DIR"; }
trap cleanup EXIT

# The release carries adm-<version>-<os>-<arch>.tar.gz; ask the API for it so
# the version number never has to be known in advance.
if [ -n "${ADM_PACKAGE:-}" ]; then
    ASSET_URL="$ADM_PACKAGE"
elif [ -n "${ADM_VERSION:-}" ]; then
    RELEASE_URL="https://api.github.com/repos/$REPO/releases/tags/v$ADM_VERSION"
else
    RELEASE_URL="https://api.github.com/repos/$REPO/releases/latest"
fi
if [ -z "${ADM_PACKAGE:-}" ]; then
status "Looking up the release..."
ASSET_URL="$(curl -fsSL "$RELEASE_URL" \
    | grep -o "\"browser_download_url\": *\"[^\"]*adm-[^\"]*-linux-$ARCH\.tar\.gz\"" \
    | head -n 1 | sed 's/.*"\(https[^"]*\)"/\1/')"
[ -n "$ASSET_URL" ] || error "no linux-$ARCH package in ${ADM_VERSION:+release v$ADM_VERSION of }$REPO"
fi
ARCHIVE="$TEMP_DIR/$(basename "$ASSET_URL")"

if [ -f "$ASSET_URL" ]; then
    cp "$ASSET_URL" "$ARCHIVE"
else
    status "Downloading $(basename "$ASSET_URL")..."
    curl --fail --show-error --location --progress-bar -o "$ARCHIVE" "$ASSET_URL"
fi

status "Unpacking..."
tar -C "$TEMP_DIR" -xzf "$ARCHIVE"
PACKAGE="$(find "$TEMP_DIR" -mindepth 1 -maxdepth 1 -type d -name 'adm-*' | head -n 1)"
[ -x "$PACKAGE/bin/adm" ] || error "the package has no bin/adm"

# A development checkout links ~/.adm/lib at the sources; never write over that.
for part in bin lib tools; do
    if [ -L "$INSTALL_DIR/$part" ]; then
        error "$INSTALL_DIR/$part is a symlink (a development setup?); remove it or set ADM_INSTALL_DIR"
    fi
done

status "Installing to $INSTALL_DIR..."
MANIFEST="$INSTALL_DIR/install-manifest"
# A reinstall keeps what the first install recorded about the directory and
# the shell files, so uninstall still knows ~/.adm was ours.
CREATED_ROOT=0
[ -d "$INSTALL_DIR" ] || CREATED_ROOT=1
if [ -f "$MANIFEST" ] && grep -q '^created-root 1$' "$MANIFEST"; then
    CREATED_ROOT=1
fi
OLD_RC=""
[ -f "$MANIFEST" ] && OLD_RC="$(grep "^rc	" "$MANIFEST" || :)"
mkdir -p "$INSTALL_DIR"
# Only the package's own directories are replaced; cache/, pkg/ and catalog/
# belong to the user and stay.
for part in bin lib tools; do
    rm -rf "$INSTALL_DIR/$part"
    if [ -d "$PACKAGE/$part" ]; then
        cp -R "$PACKAGE/$part" "$INSTALL_DIR/$part"
    fi
done
if [ -f "$PACKAGE/uninstall.sh" ]; then
    cp "$PACKAGE/uninstall.sh" "$INSTALL_DIR/uninstall.sh"
    chmod +x "$INSTALL_DIR/uninstall.sh"
fi
{
    echo "# Written by install.sh; uninstall.sh removes exactly what is listed here."
    echo "version $(cat "$PACKAGE/VERSION" 2>/dev/null || basename "$PACKAGE" | sed 's/^adm-//; s/-[a-z]*-[a-z0-9]*$//')"
    echo "created-root $CREATED_ROOT"
    for part in bin lib tools; do
        [ ! -d "$INSTALL_DIR/$part" ] || echo "dir $part"
    done
    [ ! -f "$INSTALL_DIR/uninstall.sh" ] || echo "file uninstall.sh"
    [ -z "$OLD_RC" ] || echo "$OLD_RC"
} > "$MANIFEST"

BIN="$INSTALL_DIR/bin"
case ":$PATH:" in
    *":$BIN:"*) ;;
    *)
        LINE="export PATH=\"$BIN:\$PATH\""
        ADDED=""
        for rc in "$HOME/.bashrc" "$HOME/.zshrc" "$HOME/.profile"; do
            if [ -f "$rc" ] && ! grep -Fq "$BIN" "$rc"; then
                printf '\n# ADM\n%s\n' "$LINE" >> "$rc"
                printf 'rc\t%s\t%s\n' "$rc" "$LINE" >> "$MANIFEST"
                ADDED="$ADDED $rc"
            fi
        done
        if [ -d "$HOME/.config/fish" ] && ! grep -Fqs "$BIN" "$HOME/.config/fish/config.fish"; then
            printf '\n# ADM\nfish_add_path %s\n' "$BIN" >> "$HOME/.config/fish/config.fish"
            printf 'rc\t%s\t%s\n' "$HOME/.config/fish/config.fish" "fish_add_path $BIN" >> "$MANIFEST"
            ADDED="$ADDED $HOME/.config/fish/config.fish"
        fi
        if [ -n "$ADDED" ]; then
            status "Added $BIN to PATH in:$ADDED"
            status "Open a new shell, or run: $LINE"
        else
            status "Add $BIN to your PATH: $LINE"
        fi
        ;;
esac

status "Installed $("$BIN/adm" version 2>/dev/null | head -n 1 || echo adm)"

# Programs link against the system C library, so its headers and crt objects
# must be installed. install.sh never installs system packages; it names the
# one to install.
CLANG="$INSTALL_DIR/tools/clang"
if [ -x "$CLANG" ]; then
    if ! printf '#include <stdio.h>\n#include <time.h>\n' | "$CLANG" -x c -fsyntax-only - >/dev/null 2>&1 \
        || [ "$("$CLANG" -print-file-name=crt1.o)" = "crt1.o" ]; then
        ids="$(sed -n 's/^\(ID\|ID_LIKE\)=//p' /etc/os-release 2>/dev/null | tr -d '"' | tr '\n' ' ')"
        case " $ids " in
            *" debian "*|*" ubuntu "*|*" linuxmint "*) cmd="sudo apt install libc6-dev" ;;
            *" fedora "*|*" rhel "*|*" centos "*) cmd="sudo dnf install glibc-devel" ;;
            *" suse "*|*" opensuse "*|*" opensuse-leap "*|*" opensuse-tumbleweed "*) cmd="sudo zypper install glibc-devel" ;;
            *" arch "*|*" manjaro "*) cmd="sudo pacman -S glibc linux-api-headers" ;;
            *) cmd="" ;;
        esac
        warn "the C library's development files (headers, crt1.o) are missing; programs will not build until they are installed"
        if [ -n "$cmd" ]; then
            warn "install them with: $cmd"
        else
            warn "install your distribution's glibc development package"
        fi
        status "'adm doctor' checks the toolchain again afterwards."
    fi
fi
status "Run 'adm --help' to get started."
if [ -f "$INSTALL_DIR/uninstall.sh" ]; then
    status "To remove ADM later: $INSTALL_DIR/uninstall.sh"
fi
}

main "$@"
