# Build Requirements

## Unix-like Toolchain (Recommended)

To build the project on Windows using a Unix-like environment, you can use:
[**W64devkit**](https://github.com/skeeto/w64devkit)

* Download and extract W64devkit
* Add its `bin` directory to your `PATH`
* Includes GCC, GDB, and Make

---

## Clang (Optional)
**Version used: LLVM 21.1.8**

### Clang with MSVC (Recommended)
Requires Microsoft toolchain.

* Install **Visual Studio** with C++ support: https://visualstudio.microsoft.com/
* Install **(LLVM-21.1.8-win64.exe)**: https://github.com/llvm/llvm-project/releases/tag/llvmorg-21.1.8

---

### LLVM MinGW
Preconfigured Clang toolchain for Windows with MinGW runtime:
* Install **(llvm-mingw-20251216-msvcrt-x86_64.zip)**: https://github.com/mstorsjo/llvm-mingw/releases/tag/20251216
* Includes MinGW-w64 runtime
* No Visual Studio required

---

# Notes
* **MSVC toolchain** depends on Visual Studio and uses Microsoft’s linker/runtime
* **MinGW / W64devkit** provides a self-contained Unix-like environment with GCC or Clang
* The project is primarily tested with MinGW-based toolchains
