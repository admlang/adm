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

## Editor plugins

The plugins live in their own repository, [admlang/plugins](https://github.com/admlang/plugins).
Both run the installed `adm` (`adm lsp` for language features), so install ADM first.

- **IntelliJ IDEA, CLion and other JetBrains IDEs** (2025.2 and later): language features, run
  and debug, and the ADM tool window (project info, libraries, documentation, tests with
  coverage, audit, lint, live services). `adm-intellij.zip` is attached to each
  [release](https://github.com/admlang/adm/releases/latest): Settings → Plugins → ⚙ →
  *Install Plugin from Disk…*, pick the zip, restart.
- **Visual Studio Code** (1.88 and later): language features, syntax colouring, `check` suites
  in the Testing view, commands and tasks for `adm run`, `build`, `check`, `test`, `lint` and
  `fmt`. Install it from the
  [Marketplace](https://marketplace.visualstudio.com/items?itemName=admlang.adm)
  (`code --install-extension admlang.adm`), or take `adm-vscode.vsix` from a release and use
  *Extensions: Install from VSIX…*.

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
