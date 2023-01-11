![Symmetry in Chaos - Emperor's Cloak](docs/emperors_cloak.jpg)

# Symmetry in Chaos

This repository contains code and parameters for rendering
the images published in the book "Symmetry in Chaos" (1st edition)
by Michael Field and Martin Golubitsky.

An webpage utilizing WebAssembly for rendering in the browser can be found on the github
page <https://sophonet.github.io/symmetryinchaos/>.

### Theory

In their book, Field and Golubitsky summarize mathematical properties of symmetry
as well as fundamentals of the chaotic behavior of non-linear systems like strange
attractors. They then introduce functions that by design falls into the category of
logistic maps while on the other hand has built-in properties of symmetry. This
function yield symmetric chaotic pointsets that are colored by their number of hits
during iteration. Their book includes more details about both symmetry as well as
chaos that can be found in nature.

### Prerequisites

The code uses the following external software packages:

* SDL for fast rendering <https://www.libsdl.org/>
* nlohmann/json for json parsing <https://github.com/nlohmann/json>
* jarro2783/cxxopts for command line parameters when building the native version <https://github.com/jarro2783/cxxopts>

#### Installing prerequisites for the native application

* On MacOS, the three packages can be installed with [Homebrew](https://brew.sh):

```
brew install sdl12-compat
brew install nlohmann-json
brew install cxxopts
```

* On linux, these packages should be available in your distribution's package manager.

* All packages are also available in the [Conan center](https://conan.io/center).

#### Installing prerequisites for WebAssembly.

The installation description below uses [Emscripten](https://emscripten.org/) as a docker image, which already includes SDL. Furthermore, cxxopts for command line parameters are not needed for a browser application. The easiest way here to install nlohmann-json is to use git:

```
git clone --depth 1 https://github.com/nlohmann/json.git
```

### Installation

The single cpp source file can easily be compiled to a runtime executable, which is why build systems like CMake are not necessary, if the prerequisites are installed at default locations:

```
g++ -o symmetryinchaos -std=c++14 -lSDL symmetryinchaos.cpp
```

The WebAssembly for running the code in a website can be built using the [Emscripten](https://emscripten.org/) compiler, e.g. using docker, after cloning nlohmann/json with

```
docker run \
  --rm \
  -v $(pwd):/src \
  -u $(id -u):$(id -g) \
  emscripten/emsdk \
  emcc symmetryinchaos.cpp -I json/include -O2 \
  -sEXPORTED_FUNCTIONS=_main,_launch \
  -sEXPORTED_RUNTIME_METHODS=ccall -o index.js
```

### Dataset file

The [datasets.json](datasets.json) file contains parameters for all datasets that are described in the first edition of the book. Color maps have been inspired by another software package <https://symmetrichaos.sourceforge.net/> which is now unmaintained.

### Possible future extensions

While the software is intended to have minimal dependencies and code size, the index.html file could be extended such that individual parameters for the images can be adjusted "live" with range controls, allowing interactive discovery of new patterns or patterns that are similar to the datasets of the book.
