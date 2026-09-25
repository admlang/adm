#!/bin/sh
# Removes an ADM installation made by install.sh:
#
#   ~/.adm/uninstall.sh            or
#   curl -fsSL https://raw.githubusercontent.com/admlang/adm/main/uninstall.sh | sh
#
# Removes only what install.sh recorded in <install dir>/install-manifest:
# the bin/, lib/ and tools/ directories, this script, and the "# ADM" PATH
# block it appended to shell startup files (only when that block is still
# exactly as written). System packages are never touched: install.sh never
# installs any.
#
# cache/, pkg/ and catalog/ hold the user's build cache and downloaded
# libraries; they stay unless --purge is given. Anything else in the install
# directory is left alone, and the directory itself is removed only when empty.
#
#   --purge              also delete cache/, pkg/ and catalog/
#   ADM_INSTALL_DIR=...  the installation to remove (default ~/.adm)

main() {
set -eu

red="$( (tput bold || :; tput setaf 1 || :) 2>/dev/null)"
plain="$( (tput sgr0 || :) 2>/dev/null)"
status() { echo ">>> $*" >&2; }
warn() { echo "${red}WARNING:${plain} $*" >&2; }
error() { echo "${red}ERROR:${plain} $*" >&2; exit 1; }

PURGE=0
for arg in "$@"; do
    case "$arg" in
        --purge) PURGE=1 ;;
        *) error "unknown argument: $arg (only --purge is accepted)" ;;
    esac
done

INSTALL_DIR="${ADM_INSTALL_DIR:-$HOME/.adm}"
MANIFEST="$INSTALL_DIR/install-manifest"

# A development checkout links ~/.adm/lib at the sources; never touch that.
for part in bin lib tools; do
    if [ -L "$INSTALL_DIR/$part" ]; then
        error "$INSTALL_DIR/$part is a symlink (a development setup?); nothing removed"
    fi
done

if [ -f "$MANIFEST" ]; then
    # Only names install.sh writes are accepted, whatever the file says.
    DIRS="$(sed -n 's/^dir //p' "$MANIFEST" | grep -xE 'bin|lib|tools' || :)"
    FILES="$(sed -n 's/^file //p' "$MANIFEST" | grep -xF 'uninstall.sh' || :)"
    CREATED_ROOT="$(sed -n 's/^created-root //p' "$MANIFEST")"
    RC_ENTRIES="$(grep "^rc	" "$MANIFEST" || :)"
elif [ -x "$INSTALL_DIR/bin/adm" ] && [ -d "$INSTALL_DIR/lib/std" ]; then
    # Installed before install.sh wrote a manifest: the layout and the PATH
    # line it appended are known exactly.
    status "No install-manifest (an older install); removing the standard layout"
    DIRS="bin lib tools"
    FILES="uninstall.sh"
    CREATED_ROOT=1
    LINE="export PATH=\"$INSTALL_DIR/bin:\$PATH\""
    RC_ENTRIES="$(printf 'rc\t%s\t%s\nrc\t%s\t%s\nrc\t%s\t%s\nrc\t%s\t%s' \
        "$HOME/.bashrc" "$LINE" "$HOME/.zshrc" "$LINE" "$HOME/.profile" "$LINE" \
        "$HOME/.config/fish/config.fish" "fish_add_path $INSTALL_DIR/bin")"
elif [ "$PURGE" = "1" ] && { [ -e "$INSTALL_DIR/cache" ] || [ -e "$INSTALL_DIR/pkg" ] || [ -e "$INSTALL_DIR/catalog" ]; }; then
    # Already uninstalled; --purge still clears what the first run kept.
    DIRS=""
    FILES=""
    CREATED_ROOT=1
    RC_ENTRIES=""
else
    error "no ADM installation found at $INSTALL_DIR"
fi

[ -z "$DIRS" ] || status "Removing ADM from $INSTALL_DIR..."
for part in $DIRS; do
    rm -rf "${INSTALL_DIR:?}/$part"
done
for f in $FILES; do
    rm -f "${INSTALL_DIR:?}/$f"
done
rm -f "$MANIFEST"

# Drop the block install.sh appended: a blank line, "# ADM", then the exact
# line. A block the user edited is reported and kept.
TAB="$(printf '\t')"
echo "$RC_ENTRIES" | while IFS="$TAB" read -r tag rc line; do
    [ "$tag" = "rc" ] && [ -f "$rc" ] || continue
    grep -qxF "$line" "$rc" || continue
    tmp="$rc.adm-uninstall.$$"
    if awk -v want="$line" '
        { lines[NR] = $0 }
        END {
            n = 0
            for (i = 1; i <= NR; i++) {
                if (lines[i] == "# ADM" && i < NR && lines[i+1] == want) {
                    if (n > 0 && out[n] == "") n--
                    i++
                    removed = 1
                    continue
                }
                out[++n] = lines[i]
            }
            for (i = 1; i <= n; i++) print out[i]
            exit removed ? 0 : 1
        }' "$rc" > "$tmp"; then
        cat "$tmp" > "$rc"
        status "Removed the ADM PATH entry from $rc"
    else
        warn "$rc mentions $INSTALL_DIR/bin outside the block install.sh wrote; left as is"
    fi
    rm -f "$tmp"
done

KEPT=""
for part in cache pkg catalog; do
    [ -e "$INSTALL_DIR/$part" ] || continue
    if [ "$PURGE" = "1" ]; then
        status "Deleting $INSTALL_DIR/$part ($(du -sh "$INSTALL_DIR/$part" 2>/dev/null | cut -f1))"
        rm -rf "${INSTALL_DIR:?}/$part"
    else
        KEPT="$KEPT $part($(du -sh "$INSTALL_DIR/$part" 2>/dev/null | cut -f1))"
    fi
done

if [ -d "$INSTALL_DIR" ] && rmdir "$INSTALL_DIR" 2>/dev/null; then
    status "Removed $INSTALL_DIR"
elif [ -n "$KEPT" ]; then
    status "Kept in $INSTALL_DIR:$KEPT (run with --purge to delete them)"
elif [ -d "$INSTALL_DIR" ] && [ "$CREATED_ROOT" = "1" ]; then
    status "Kept $INSTALL_DIR: it holds files ADM did not install"
fi
status "ADM uninstalled."
}

main "$@"
