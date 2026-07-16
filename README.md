# Simple Calculator GUI (SDL)

[![CI](https://github.com/dmccoystephenson/Simple-Calculator-GUI-Using-SDL/actions/workflows/ci.yml/badge.svg)](https://github.com/dmccoystephenson/Simple-Calculator-GUI-Using-SDL/actions/workflows/ci.yml)

A small calculator in C++. The calculation logic lives in a UI-agnostic core
(`CalculatorEngine`) that every frontend shares, so the same engine drives both
a graphical [SDL2](https://www.libsdl.org/) interface and a text/CLI interface.

## Architecture

The code is split into a shared engine and thin frontends:

| File | Role |
| --- | --- |
| `CalculatorEngine.h` / `.cpp` | UI-agnostic core — input state, the 7-slot display model, and evaluation via the shunting-yard parser. Knows nothing about SDL or a terminal. |
| `simpleCalculator.cpp` | SDL/GUI frontend — renders the engine's display as textures and turns mouse clicks into engine input. |
| `textCalculator.cpp` | Text/CLI frontend — renders the display as text and turns typed characters into engine input. |
| `testingParsing.cpp` | Assert-based test suite for the engine + parser (no SDL). |

A frontend contains no calculation logic of its own; it only translates its
input events into `CalculatorEngine` calls and renders the display the engine
exposes. New frontends (e.g. a pygame binding or a WebAssembly build) can be
added by reusing the same core.

## Dependencies

- A C++17 compiler (e.g. `g++`)
- [SDL2](https://www.libsdl.org/)
- [SDL2_image](https://github.com/libsdl-org/SDL_image)

On Debian/Ubuntu:

```sh
sudo apt install g++ libsdl2-dev libsdl2-image-dev
```

## Building

With the provided `Makefile`:

```sh
make            # builds simpleCalculator, textCalculator, and testingParsing
make clean      # removes built binaries
```

Or compile directly with `g++` (each frontend links the shared engine):

```sh
# GUI calculator (needs SDL2 + SDL2_image)
g++ -std=c++17 simpleCalculator.cpp CalculatorEngine.cpp -o simpleCalculator $(pkg-config --cflags --libs sdl2 SDL2_image)

# Text/CLI calculator (no SDL dependency)
g++ -std=c++17 textCalculator.cpp CalculatorEngine.cpp -o textCalculator

# Engine + parser test suite (no SDL dependency)
g++ -std=c++17 testingParsing.cpp CalculatorEngine.cpp -o testingParsing
```

### Windows (one-shot)

On Windows, run the bundled script — it builds `simpleCalculator.exe` and
launches it in one step:

```bat
build-and-run.bat
```

Double-click it in Explorer, or run it from a Command Prompt. It checks for
`g++`, `pkg-config`, and SDL2/SDL2_image and prints the exact install command
if anything is missing, then `cd`s to its own folder so the PNG assets load.

It targets the MinGW-w64 toolchain (the same `g++` + `pkg-config` build as the
Makefile). The easiest way to get that is [MSYS2](https://www.msys2.org/) —
install the prerequisites once in an MSYS2 MinGW64 shell:

```sh
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 \
          mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-pkg-config
```

then add `C:\msys64\mingw64\bin` to your PATH (so both the compiler and the
SDL runtime DLLs are found).

## Running

```sh
./simpleCalculator
```

The button textures are PNG files (`one.png`, `plus.png`, `equals.png`, …) loaded
with relative paths via `IMG_Load`, so **run the program from the repository
directory** where those assets live. Running it from elsewhere will fail to load
the textures.

The text/CLI frontend runs in a terminal — type digits and `+ - *`, then `=` to
evaluate, `c` to clear, `q` to quit:

```sh
./textCalculator
```

The test suite runs the engine + parser assertions and prints a summary:

```sh
./testingParsing
```

## Supported operations

Integer `+`, `-`, and `*` with standard precedence and left-associativity.
Division and decimals are not yet supported (tracked in the issue tracker).

See the [issue tracker](https://github.com/dmccoystephenson/Simple-Calculator-GUI-Using-SDL/issues)
for known limitations and planned work.
