# Entrada/salida

En un sistema, la entrada y salida es vital para permitir la comunicación con el mundo exterior. Existen 2 mecanismos clásicos
para la entrada/salida: 

## Entrada/salida basada en puertos

En este mecanismo, el procesador emplea instrucciones especiales para comunicarse con los dispositivos de entrada/salida, estas
instrucciones sirven para enviar y recibir datos de los dispositivos. Los dispositivos de entrada/salida tienen un número de
puerto asociado. Cada dispositivo puede tener N puertos asociados. Lo más normal es que un dispositivo tenga un puerto para
leer el estado y otro para enviar/recibir datos.

Un ejemplo de dispositivo de entrada/salida basado en puertos es el teclado PS/2. El teclado PS/2 tiene 2 puertos asociados, uno
para leer el estado y otro para enviar/recibir datos:


El puerto 0x60 es el puerto de datos del teclado PS/2. El puerto 0x64 es el puerto de estado del teclado PS/2.

Este código ensamblador comprueba si hay datos en el buffer del teclado y si los hay, los lee. Si no hay datos, el código se
bloquea hasta que haya datos en el buffer.

```nasm
read_keyboard:
    in al, 0x64
    test al, 0x01
    jz read_keyboard
    in al, 0x60
    ret
```

De cara a facilitar la comunicación con los dispositivos, podemos crear nuestra pequeña librería C para invocar estas
instrucciones de entrada salida.

## Entrada/salida mapeada en memoria

Otra opción es mapear los dispositivos de entrada/salida en memoria. En este caso, los dispositivos de entrada/salida se
visualizan como un rango de direcciones dentro del espacio de del proceso. Pudiendo leer y escribir en estas direcciones como si
fuesen memoria normal. Un ejemplo de dispositivo de entrada/salida mapeada en memoria es el framebuffer de la tarjeta gráfica.

Este código C mapea el framebuffer de la tarjeta gráfica en memoria y escribe un pixel rojo en la posición 0,0.

```c
#include <stdint.h>

#define FRAMEBUFFER 0xB8000 // Dirección del framebuffer

void main() {
    uint16_t *fb = (uint16_t *) FRAMEBUFFER;
    fb[0] = 0x04;
}
```

