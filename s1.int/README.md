# Interrupciones

## Introducción

Una interrupción es una señal que indica que un evento ha ocurrido. Las interrupciones pueden servir para indicar que un dispositivo ha terminado una operación, que ha ocurrido un error, que se ha pulsado una tecla, que ha terminado un temporizador, etc.

El sistema operativo no puede ejecutar interrupciones directamente (Esto de hecho, negaría el concepto de interrupción). En cambio, es el hardware el que genera la interrupción y el sistema operativo define que
código que se ejecutará cuando se produzca la interrupción.

## Tipos de interrupciones

### Interrupciones de hardware

Las interrupciones de hardware son generadas por el hardware del sistema. Por ejemplo, cuando se pulsa una tecla. A nivel físico estas están gestionadas mediante un chip dedicado llamado PIC (Programmable Interrupt Controller). El PIC gestiona las interrupciones y las prioridades de las mismas. Enviando al
procesador la interrupción más prioritaria.

El chip que históricamente se ha utilizado para gestionar las interrupciones es el 8259A. Este chip es capaz de gestionar 8 interrupciones. Para gestionar más de 8 interrupciones se utilizan varios 8259A en cascada. En la actualidad y a raíz de la aparición de los procesadores multicore, se ha desarrollado el APIC (Advanced Programmable Interrupt Controller) que es capaz de gestionar hasta 224 interrupciones (O más si se emplea MSI, MSI-X). En este curso trabajaremos principalmente con el 8259A, tratando APIC en el último tema.

### Interrupciones de software

Las interrupciones de software son generadas por el software del sistema. Por ejemplo, cuando se ejecuta una instrucción INT en ensamblador.

### Excepciones

Las excepciones son un tipo especial de interrupción que se generan cuando ocurre un error. Por ejemplo, cuando se intenta dividir por cero, se intenta acceder a una dirección de memoria no válida, etc.

## Configuración de las interrupciones

Las interrupciones se configuran mediante una tabla llamada IDT (Interrupt Descriptor Table). La IDT es una tabla de 256 entradas, cada una de las cuales contiene la dirección de memoria de la rutina de atención a la interrupción correspondiente. La IDT se configura mediante la instrucción LIDT.

### IDT

Cada entrada de la IDT tiene un tamaño fijo de 16 bytes y contiene los siguientes campos:

| Campo | Tamaño | Descripción |
|-------|--------|-------------|
| Offset 15:0 | 16 bits | Parte baja de la dirección de memoria de la rutina de atención a la interrupción |
| Selector | 16 bits | Selector de segmento de código (Apunta a una entrada valida de la GDT) |
| IST | 3 bits | Número de la pila (Entrada del TSS) a utilizar (Se puede ignorar poniendolo a 0) |
| Reservado | 5 bits | Reservado |
| Tipo | 4 bits | Tipo de puerta (Puerta de interrupción 0xE, puerta de trampa 0xF) |
| Zero | 1 bit | Siempre vale 0 en 64 bits |
| DPL | 2 bits | Nivel de privilegio (0, 1, 2, 3) que debe ejecutar el código que invoque esta interrupción por sw |
| P | 1 bit | Presente (1 si la entrada es válida, 0 si no) |
| Offset 31:16 | 16 bits | Parte media de la dirección de memoria de la rutina de atención a la interrupción |
| Offset 63:32 | 32 bits | Parte alta de la dirección de memoria de la rutina de atención a la interrupción |
| Reservado | 32 bits | Reservado |

Para calcular la dirección de la entrada i de la IDT se utiliza la siguiente fórmula:

```
IDT[i] = IDT_BASE + i * 16
```

Para extraer la dirección de memoria de la rutina de atención a la interrupción se utiliza la siguiente fórmula:

```
Offset = offset_15_0 + (offset_31_16 << 16) + (offset_63_32 << 32)
```

### Interrupción de tipo Trap

Una trap es una excepción que se debe manejar. Por ejemplo, una división por cero. En caso de no haber definido
una rutina de atención a la interrupción para una trap, el sistema operativo ejecutará una nueva excepción llamada
doble falta. Si no se ha definido una rutina de atención a la doble falta, el sistema operativo ejecutará una triple
falta que causará un reset del sistema.

Una trap guardará en el estado de la CPU la dirección de la instrucción que la causó. Esto es útil para depuración.

### Interrupción de tipo Interrupt

Una interrupción es un evento que se debe manejar. Por ejemplo, una tecla pulsada. La lógica para doble y triple falta
aplica igual que en el caso de las traps. Sin embargo, las interrupciones guardan la dirección de la instrucción que
debe ejecutarse a continuación en el estado de la CPU (Vs la dirección que causó la interrupción en el caso de la trap).

Otra diferencia vital, en el caso del PIC, es que las interrupciones pueden ser enmascaradas. Es decir, si se está
atendiendo a una interrupción de teclado, se puede enmascarar la interrupción de ratón. Esto no es posible con las traps.

Ambas deben finalizar con la instrucción IRET.

### IA-32 TSS

El TSS (Task State Segment) es una estructura de datos que se utiliza para almacenar el estado de un proceso. El TSS
se usaba en el pasado para cambiar de tarea en sistemas operativos. En la actualidad, el TSS ha quedado mayoritariamente en desuso ya que la multitarea por software es más eficiente.