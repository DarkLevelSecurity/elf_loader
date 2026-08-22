# ELF loader

This is a PoC to a technique I found on X. This technique creates an anonymous process in the linux memory in run time and inject a payload then execute it.

> *Note: This code reads the targeted payload from the device disk with no decryption or any kind of obsfuscation methods. A more practical version will include obfuscation, and the should be payload fetched from the internet or embedded in the loader code*

## Requirements
- Linux system
- GCC compiler installed

## How to try

Download the code: `git clone [REPO_URL]`

Compile the loader: `make loader`

Compile the example: `make example`

then run the loader: `./elf_loader.out`

You will notice a `hacked_test.txt` file is being created which was the job of the `payload_example.elf` file that the loader loaded to the memory.
