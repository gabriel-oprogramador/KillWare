# Build Requirements
- GCC or Clang (C compiler)
- GDB (Debugger)

## Links
Manual installation **(LLVM-21.1.8-Linux-X64.tar.xz)**: https://github.com/llvm/llvm-project/releases/tag/llvmorg-21.1.8  

## Debian / Ubuntu
### GCC
```bash
sudo apt install build-essential gdb
```
### Clang (optional)
```bash
sudo apt install clang gdb
```

## Arch
### GCC
```bash
sudo pacman -S base-devel gdb
```
### Clang (optional)
```bash
sudo pacman -S clang gdb
```

## Fedora
### GCC
```bash
sudo dnf install gcc gcc-c++ make gdb
```
### Clang (optional)
```bash
sudo dnf install clang gdb
```

# Dependencies

## Debian / Ubuntu
```bash
sudo apt install libx11-dev libxi-dev libxrender-dev libgl1-mesa-dev
```
## Arc
```bash
sudo pacman -S libx11 libxi libxrender mesa
```
## Fedora
```bash
sudo dnf install libX11-devel libXi-devel libXrender-devel mesa-libGL-devel
```