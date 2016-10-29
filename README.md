# About

This is a FreeRTOS port on IA32 PC, which is modified from the [FreeRTOS port on Intel Galileo](http://www.freertos.org/RTOS_Intel_Quark_Galileo_GCC.html).

# Directory Structure

See [FreeRTOS source code directory structure](http://www.freertos.org/a00017.html). This PC port contains only the files from the Galileo port. The directory Demo/IA32_flat_GCC_Galileo_Gen_2 is renamed to Demo/IA32_flat_GCC_PC.

In Demo/IA32_flat_GCC_PC, Support_Files contains the board supporting package, and the remaining directories are FreeRTOS applications.

# Cross Compiler

Download the [pre-built i686-elf-gcc](http://wiki.osdev.org/GCC_Cross-Compiler#Prebuilt_Toolchains) for Linux x86_64 or Windows. Extract the archive and add the directory where i686-elf-gcc exists into the environment variable PATH.

For Windows users, it is also required to install [MinGW](https://sourceforge.net/projects/mingw/files/latest/download), and add MinGW\bin into PATH to run i686-elf-gcc.

# Builder

For Linux users, just install make. For Windows users, install [msys2](https://msys2.github.io), add msys64\usr\bin into PATH, and run "pacman -S make" in cmd.

# Build Commands

```
git clone https://github.com/kugwa/freertos_x86_gcc_pc.git

cd freertos_x86_gcc_pc/Demo/IA32_flat_GCC_PC/Blinky_Demo

make # generate build/Blinky_Demo.elf
```

# Boot Loader

Use a bootloader supporting Multiboot Specification to boot Blinky_Demo.elf

For example, if you use GRUB 2 as the bootloader and put Blinky_Demo.elf in /boot, you need a menuentry in grub.cfg:

```
menuentry 'FreeRTOS Blinky Demonstration' {
    multiboot /boot/Blinky_Demo.elf
}
```
