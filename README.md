# ADM

ADM is a statically typed, multi-paradigm programming language. This repository holds the
released language packages and the sources of the prelude and standard library every program
is checked against.

## Install

**Automatic** (Linux, current user, no sudo):

```sh
curl -fsSL https://raw.githubusercontent.com/admlang/adm/main/install.sh | sh
```

The script downloads the latest release into `~/.adm` and adds `~/.adm/bin` to your `PATH`.
`ADM_VERSION=0.1` installs a specific release; `ADM_INSTALL_DIR` changes the location.

**Manual**: download `adm-<version>-linux-amd64.tar.gz` from the
[releases](https://github.com/admlang/adm/releases), unpack it anywhere, and put its `bin/`
directory on `PATH`. The package is self-contained: the compiler finds `lib/` (prelude and
standard library) and `tools/` (bundled clang and lld) relative to itself.

```sh
tar -xzf adm-<version>-linux-amd64.tar.gz
export PATH="$PWD/adm-<version>-linux-amd64/bin:$PATH"
adm version
```

Later, `adm update` installs the newest release over the current one, and `adm version` says
when there is one.

## IntelliJ plugin

`adm-intellij-<version>.zip`: syntax highlighting, code completion,
navigation and diagnostics through the language server, and run, test and build actions in
the editor gutter for applications, `check` suites and plugins. Install it from the file:
Settings → Plugins → ⚙ → *Install Plugin from Disk…*, pick the zip, restart. The plugin
finds `adm` on `PATH`.

## First program

```adm
application Hello {
	def new(args string[]) int {
		println("hello", 6 * 7)
		return 0
	}
}
```

```sh
adm run hello.adm
```

## Layout

- `lib/prelude` — the builtin module: strings, arrays, maps, numbers, channels, annotations
- `lib/std` — the standard library
- `install.sh` — the installer above; `adm update` does the same for an existing install
- `VERSION` — the released compiler version

Documentation: https://adm-lang.dev. Libraries are published through the
[registry](https://github.com/admlang/registry).
