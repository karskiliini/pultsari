# Pultsari

## basic/
    pulsubin.bas    the original version of Pultsari 1.0 in QuickBasic 4.5 binary format
    pultsari.bas    ASCII version of the original binary, with some refactoring done.
## include/
                    All the headers for a c++ port.
## src/
                    All the source files for a c++ port.

## Build instructions

### Windows 10
- Install [QB64](https://qb64.com/) IDE, open pultsari.bas file and press F5 to compile pultsari.exe.
- run pultsari.exe

or

- Install dosbox
- Install QB4.5 (Quick Basic 4.5)
- Open either pulsubin.bas (for the original experience) or pultsari.bas for the updated version.
- Either run directly in QB4.5 or compile pultsari.exe


### linux
The compilation requires C++11 gcc  
$ make clean  
$ make  
$ ./pultsari  
  
For advanced features try  
$ ./pultsari --help  
$ ./pultsari --all  
  
