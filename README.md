# Curso de programación de sistemas operativos

__Autor:__ [Xabier Iglesias](mailto://xabier.iglesias.perez@udc.es)

| Fecha | Versión | Descripción |
|-------|---------|-------------|
| 07/02/24 | 0.1 | Versión inicial |

Compilar con:

```bash
nasm -fbin boot.asm -o boot.bin
```

Ejecutar con:

```bash
qemu-system-x86_64 -fda boot.bin
```

## Descripción

Este curso pretende introducir al alumno en el desarrollo de sistemas operativos. Se tratarán los conceptos básicos de los sistemas operativos, así como las técnicas de programación necesarias para su desarrollo.

## Objetivos

- Conocer los conceptos básicos de los sistemas operativos.
- Conocer las técnicas de programación necesarias para el desarrollo de sistemas operativos.
- Conocer las particularidades de la arquitectura x86.

## Contenidos

1. Introducción a los sistemas operativos
2. El proceso de arranque
3. Interrupciones y excepciones
4. Memoria física
5. Memoria virtual
6. El heap
7. Teclado
8. Drivers
9. Disco
10. Sistema de ficheros ext2
11. Sistema de ficheros virtual
12. Procesos
13. El formato ELF
14. Multitarea y planificación
15. Llamadas al sistema
16. Conceptos avanzados

## Qué necesitas

- Un ordenador con Windows + WSL o Linux. Con 8GB de RAM, 20GB de espacio libre y un procesador de 64 bits (x86_64) con soporte para virtualización y al menos 4 núcleos.
- Recomendado 16GB de RAM y un procesador con 8 núcleos.

## Conocimientos previos

- Conocimientos de programación en C.
- Conocimientos de arquitectura de computadores.
- Solvencia en el uso de la línea de comandos de Linux.

## Metodología

- Primero haz el QUIZ de nivel!
- Después abre el primer tema desde la terminal y lee el README.md
- Clona el tag correspondiente al tema y comprueba que compila y ejecuta.
- Trata de comprender el código asociado al tema con la ayuda de la explicación del README.md
- Realiza los ejercicios propuestos.
- Modifica el código de la unidad a tu gusto y comprueba que funciona.
- Pregrúntame cualquier duda que tengas.
- Continúa con el siguiente tema y repite el proceso.

Nota: Las sesiones grabadas y las clases no se corresponden directamente con los temas. Las clases son una guía para el estudio de los temas.



## RISC-V
### Requirements Installed
Toolchain para baremetal: sudo apt install gcc-riscv64-unknown-elf
QEMU RISC-V: sudo apt install qemu-system-misc
### Especificaciones
https://wiki.riscv.org/display/HOME/RISC-V+Technical+Specifications
### Virt
Direcciones de memoria de los periféricos: https://gitlab.com/qemu-project/qemu/-/blob/master/hw/riscv/virt.c#L70
Ejemplo usando virt: https://github.com/DonaldKellett/marvelos/tree/main