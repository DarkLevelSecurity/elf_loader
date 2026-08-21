loader:
	gcc src/elf_loader.c -o elf_loader.out -g
example:
	gcc src/payload_example.c -o payload_example.elf

clean:
	rm -v *.elf *.out *.txt