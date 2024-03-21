all:
	@mkdir -p build/bin
	@nasm src/main.asm -o build/bin/main.o -f elf64
	@ld -o build/bin/main.elf build/bin/main.o -m elf_x86_64 -static


clean:
	@rm -rf build