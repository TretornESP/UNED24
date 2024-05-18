# Comandos para bootshell x86
Compilar: nasm -fbin boot.asm
Cargar a QEMU: qemu-system-x86_64 -fda boot -s -S 

# Comandos bootshell RISC-V
Compilar: riscv64-unknown-elf-as -o boot_riscv boot_riscv.s
Dump de la compilación: riscv64-unknown-elf-objdump -d boot_riscv

Máquina Virt: https://www.qemu.org/docs/master/system/riscv/virt.html
Máquinas disponibles RISC-V: https://www.qemu.org/docs/master/system/target-riscv.html