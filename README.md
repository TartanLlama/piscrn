# piscrn

A screenshot server, library, and command-line utility for the Raspberry Pi

## Components
- `libpiscrn` is a library which provides a function for taking a PNG screenshot of the Raspberry Pi video output and writing it to a file, stdout, or memory.
- `piscrn` is a command-line interface to `libpiscrn`.
- `piscrnd` is an HTTP daemon which replies to `GET /screenshot` requests with a screenshot.

## Building

`piscrn` uses CMake as its build system. You can compile it for the Raspberry Pi with a [CMake Toolchain File](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html) like so:

```bash
mkdir build
cd build
cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=my-raspberry-pi-toolchain.cmake ..
ninja
ninja install
```

## Usage
### `piscrnd`
To serve requests to `GET /screenshot` at port 3001:
```
./piscrnd
```

#### Full options

```
Usage: piscrnd [--port <port>] [--quiet]

    --port,-p - port to listen on(default is 3001)
    --quiet,-q - quiet mode(default is off)
    --help,-h - print this usage information
```

### `piscrn`
To take a screenshot and save it to "snapshot.png"
```
./piscrn
```

#### Full options
```
Usage: piscrn [--pngname name] [--width <width>] [--height <height>] 
              [--compression <level>] [--delay <delay>] [--display <number>] [--stdout] [--help]

    --pngname,-p - name of png file to create (default is snapshot.png)
    --height,-h - image height (default is screen height)
    --width,-w - image width (default is screen width)
    --compression,-c - PNG compression level (0 - 9)
    --delay,-d - delay in seconds (default 0)
    --display,-D - Raspberry Pi display number (default 0)
    --stdout,-s - write file to stdout
    --help,-H - print this usage information
```

### `libpiscrn`

`libpiscrn` provides a single entry point:
```cpp
piscrn_error_code piscrn_take_screenshot(piscrn_screenshot_params const *params);
```

#### Examples
##### Output to Memory
```cpp
  piscrn_screenshot_params params = piscrn_default_params;
  params.output.choice = PISCRN_OUTPUT_MEMORY;
  const char *pngBuffer;
  size_t pngSize;
  params.output.memoryOut = &pngBuffer;
  params.output.sizeOut = &pngSize;
  piscrn_take_screenshot(&params);
```

##### Output to "my_output.png" File
```cpp
  piscrn_screenshot_params params = piscrn_default_params;
  params.output.choice = PISCRN_OUTPUT_FILE;
  params.output.fileName = "my_output.png";
  piscrn_take_screenshot(&params);
```

#### `piscrn_screenshot_params` struct members
- `piscrn_output_descriptor output`: description of where to output the png (default: "snapshot.png")
- `uint32_t displayNumber`: which RPi display to use (default: 0)
- `int compression`: zlib compression level (default: -1)
- `int32_t requestedHeight`: requested height of png (default: display height)
- `int32_t requestedWidth`: requested height of png (default: display width)
- `int delay`: delay before taking screenshot in ms (default: 0ms)

#### `piscrn_output_descriptor` struct members
- `piscrn_output_choice choice` - which output method to use
- `const char** memoryOut` - where to store the resulting data if outputting to memory (NOTE: BE SURE TO `free` THIS)
- `size_t* sizeOut` - where to store the size of the resulting data if outputting to memory
- `const char* fileName` - filename to use if outputting to a file
   
#### `prscrn_output_choice` enum members
- `PISCRN_OUTPUT_STDOUT`: output png to stdout
- `PISCRN_OUTPUT_FILE`: output png to a file
- `PISCRN_OUTPUT_MEMORY`: output png to memory

#### `prscrn_default_params`

An instance of `piscrn_screenshot_params` with defaults filled in.

## Dependencies

Depends upon [libpng](https://github.com/glennrp/libpng). This can be installed using [vcpkg](https://github.com/microsoft/vcpkg/tree/master/ports/libpng).

## Licences

Uses [jeremycw/httpserver.h](https://github.com/jeremycw/httpserver.h), which uses an MIT license.

Screenshot code is adapted from [AndrewFromMelbourne/raspi2png](https://github.com/AndrewFromMelbourne/raspi2png), which uses as MIT license.
