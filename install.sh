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

# Everything sits in main() so a truncated download cannot run half a script.
main() {
set -eu

red="$( (tput bold || :; tput setaf 1 || :) 2>/dev/null)"
plain="$( (tput sgr0 || :) 2>/dev/null)"
status() { echo ">>> $*" >&2; }
error() { echo "${red}ERROR:${plain} $*" >&2; exit 1; }

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

TEMP_DIR="$(mktemp -d)"
cleanup() { rm -rf "$TEMP_DIR"; }
trap cleanup EXIT

# The release carries adm-<version>-<os>-<arch>.tar.gz; ask the API for it so
# the version number never has to be known in advance.
if [ -n "${ADM_VERSION:-}" ]; then
    RELEASE_URL="https://api.github.com/repos/$REPO/releases/tags/v$ADM_VERSION"
else
    RELEASE_URL="https://api.github.com/repos/$REPO/releases/latest"
fi
status "Looking up the release..."
ASSET_URL="$(curl -fsSL "$RELEASE_URL" \
    | grep -o "\"browser_download_url\": *\"[^\"]*adm-[^\"]*-linux-$ARCH\.tar\.gz\"" \
    | head -n 1 | sed 's/.*"\(https[^"]*\)"/\1/')"
[ -n "$ASSET_URL" ] || error "no linux-$ARCH package in ${ADM_VERSION:+release v$ADM_VERSION of }$REPO"
ARCHIVE="$TEMP_DIR/$(basename "$ASSET_URL")"

status "Downloading $(basename "$ASSET_URL")..."
curl --fail --show-error --location --progress-bar -o "$ARCHIVE" "$ASSET_URL"

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
mkdir -p "$INSTALL_DIR"
# Only the package's own directories are replaced; cache/, pkg/ and catalog/
# belong to the user and stay.
for part in bin lib tools; do
    rm -rf "$INSTALL_DIR/$part"
    if [ -d "$PACKAGE/$part" ]; then
        cp -R "$PACKAGE/$part" "$INSTALL_DIR/$part"
    fi
done

BIN="$INSTALL_DIR/bin"
case ":$PATH:" in
    *":$BIN:"*) ;;
    *)
        LINE="export PATH=\"$BIN:\$PATH\""
        ADDED=""
        for rc in "$HOME/.bashrc" "$HOME/.zshrc" "$HOME/.profile"; do
            if [ -f "$rc" ] && ! grep -Fq "$BIN" "$rc"; then
                printf '\n# ADM\n%s\n' "$LINE" >> "$rc"
                ADDED="$ADDED $rc"
            fi
        done
        if [ -d "$HOME/.config/fish" ] && ! grep -Fqs "$BIN" "$HOME/.config/fish/config.fish"; then
            printf '\n# ADM\nfish_add_path %s\n' "$BIN" >> "$HOME/.config/fish/config.fish"
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
status "Run 'adm --help' to get started."
}

main "$@"
