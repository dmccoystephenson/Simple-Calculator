# Simple Calculator GUI (SDL)

[![CI](https://github.com/dmccoystephenson/Simple-Calculator/actions/workflows/ci.yml/badge.svg)](https://github.com/dmccoystephenson/Simple-Calculator/actions/workflows/ci.yml)

A small calculator in C++. The calculation logic lives in a UI-agnostic core
(`CalculatorEngine`) that every frontend shares, so the same engine drives both
a graphical [SDL2](https://www.libsdl.org/) interface and a text/CLI interface.

## Architecture

The code is split into a shared engine and thin frontends:

| File | Role |
| --- | --- |
| `CalculatorEngine.h` / `.cpp` | UI-agnostic core — input state, the 7-slot display model, and evaluation via the shunting-yard parser. Knows nothing about SDL or a terminal. |
| `simpleCalculator.cpp` | SDL/GUI frontend — renders the engine's display as textures and turns mouse clicks and key presses into engine input. |
| `textCalculator.cpp` | Text/CLI frontend — renders the display as text and turns typed characters into engine input. |
| `testingParsing.cpp` | Assert-based test suite for the engine + parser (no SDL). |
| `testingFrontend.cpp` | Assert-based test suite for the SDL frontend's event translation and display rendering. Runs headlessly (SDL's dummy video driver), so it needs SDL2 but no display. |

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
make            # builds simpleCalculator, textCalculator, testingParsing, and testingFrontend
make test       # builds (if needed) and runs both test suites
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

# SDL frontend test suite (needs SDL2 + SDL2_image, but no display)
g++ -std=c++17 testingFrontend.cpp CalculatorEngine.cpp -o testingFrontend $(pkg-config --cflags --libs sdl2 SDL2_image)
```

`testingFrontend.cpp` includes `simpleCalculator.cpp` directly rather than
linking it, because the GUI frontend is a single translation unit with no header
of its own; the suite renames its `main` out of the way and calls the frontend's
functions in place. That is why `simpleCalculator.cpp` is absent from the
compile line above.

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

The button textures are PNG files (`one.png`, `plus.png`, `divide.png`, …) loaded
with relative paths via `IMG_Load`, so **run the program from the repository
directory** where those assets live. Running it from elsewhere will fail to load
the textures.

The on-screen buttons can be left-clicked (a right- or middle-click is ignored),
or the keyboard can be used instead — both go through the same dispatch, so they
are interchangeable:

| Key | Effect |
| --- | --- |
| `0`–`9` (number row or keypad) | enter a digit |
| `+`, `-`, `*`, `/` (number row or keypad) | enter an operator |
| `.` | enter a decimal point |
| `=`, `Enter` (Return or keypad) | evaluate |
| `c`, `Escape`, `Delete` | clear |

Any other key is ignored. There is no key that deletes a single character —
`c`/`Escape`/`Delete` clear the whole equation, matching the on-screen `clear`
button, which is the only erase the engine offers.

The text/CLI frontend runs in a terminal — type digits, `.`, and `+ - * /`, then
`=` to evaluate, `c` to clear, `q` to quit:

```sh
./textCalculator
```

## Tests

There are two assert-based suites, each printing a summary on success. A failing
assertion aborts with a non-zero status, so both work as checks in scripts and
CI, and `make test` runs them in order:

```sh
make test       # rebuilds first if sources changed
./testingParsing    # the shared engine: parser, input state, display formatting
./testingFrontend   # the SDL frontend: event translation and display rendering
```

`testingFrontend` covers the part of the GUI that is logic rather than pixels —
which SDL event maps to which button id, which id maps to which engine call,
where a click counts as landing on a button, and which texture ends up in each
display slot. It selects SDL's dummy video driver and software renderer itself,
so it needs SDL2 but never a window, a GPU, or a display server. It does load
the real PNG assets, which is what makes a missing or renamed one a test
failure, so — like the GUI itself — **run it from the repository directory**.

Because every check in both suites is a bare `assert`, neither can be built with
`NDEBUG` defined — `assert` would compile to nothing and the binary would report
success without exercising anything. Both files guard against this with an
`#error`, so a `CXXFLAGS` override that includes `-DNDEBUG` (such as a
release-style `-O2 -DNDEBUG`) fails the build instead of passing silently. Build
the other targets with those flags if needed, and leave the suites' assertions
enabled.

## Supported operations

`+`, `-`, `*`, and `/` with standard precedence and left-associativity, on
floating-point (decimal) values.

A number literal may include at most one decimal point — `2.5`, `.5`, and `2.`
are all valid ways to enter a value, while `2.3.4` (a second `.` in the same
number) is rejected as malformed. Division is true division, not truncated —
`7/2` is `3.5`. Dividing by zero is rejected the same way any other malformed
equation is: the GUI leaves the display unchanged, and the text frontend
prints `(invalid equation)`.

A `-` typed where a value is expected is a sign rather than a subtraction, so a
negative number can be entered directly (`-` then `7` evaluates to `-7`) and a
calculation can continue from a negative result — after `3 - 10` shows `-7`,
pressing `+ 3 =` gives `-4`. The sign belongs to the number it precedes, not to
the rest of the equation, so `2*-3` is `-6` and `5--3` is `8`. Only one sign is
allowed per position: pressing `-` a third time in a row (`5---3`) is rejected
as malformed, the same way any other doubled operator (`5+*3`) is, and a leading
`+` is likewise still rejected.

Values are `double`s. An equation whose literals or intermediate/final results
overflow to infinity is rejected as malformed rather than silently producing
`inf`.

The display has 7 fixed slots (the GUI and text frontend both render from the
same underlying state, so they always agree on what a result looks like). They
show a window onto the *end* of the equation being built: an equation longer
than 7 characters scrolls, so the characters most recently entered are always
the ones on screen and what is displayed is always a truthful suffix of what
will be evaluated. A result is rounded to fit: the integer part is always shown
in full, and the result is rejected outright if that part alone does not fit —
wider than 7 characters (e.g. `50000000+49999999`), or wider than 6 when a
leading `-` takes one of the slots (e.g. `0-9999999`, where `-999999` is the
widest negative result that still fits).
Whatever slots remain are used for a decimal point and as many fractional
digits as fit — so `1/4` displays as `0.25`, but a result with a longer,
non-terminating fraction is rounded, not shown truncated after however many
digits happen to fit. A result that is a whole number once rounded (e.g. `4/2`
or a fraction that rounds away entirely) is shown with no decimal point, the
same way integer division always displayed results. A result with no magnitude
left to sign displays as a plain `0` rather than a misleading `-0` — that covers
both a negative value too small to survive the rounding and a negative zero
entered or produced directly (`-0`, `0*-3`, `0/-5`).

See the [issue tracker](../../issues)
for known limitations and planned work.
