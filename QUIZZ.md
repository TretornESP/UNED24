# Prueba de nivel

## Descripción

El objetivo de esta prueba es evaluar el nivel de conocimientos de los alumnos en los siguientes aspectos:

- Conocimientos de programación en C y el uso de GDB.
- Conocimientos de programación en ensamblador.
- Conocimientos de la arquitectura x86.
- Conocimientos de manejo de la línea de comandos.

Las respuestas se encuentran al final del documento. Es un ejercicio de autoevaluación, por lo que no es necesario entregarlo. Se recomienda responder de forma sincera, ya que el resultado de esta prueba te
servirá para saber si estás preparado para el curso.

Nota: En muchos casos, la respuesta correcta es: **Depende**. Sin embargo
se trata de evaluar tu conocimiento general: Si eres capaz de justificar
tu respuesta de forma correcta, consideraremos que has acertado.

## Cuestiones

### Q1. Que output produce el siguiente fragmento de código en C?

```c
void incrementa(int *a) {
    *a = *a + 1;
}

int main() {
    int a = 5;
    incrementa(&a);
    printf("%d\n", a);
    incrementa(a);
    printf("%d\n", a);
    incrementa(&a);
    printf("%d\n", a);
}
```

1. 6, 7, 8
2. 5, 5, 5
3. 6, error
4. 6, 6, 7

### Q2. Que output produce el siguiente fragmento de código en C?

```c
void incrementa(int *a) {
    *a = *a + 1;
}

int main() {
    int a[] = {5, 5, 5, 5, 5, 5, 5, 5, 5, 5};
    incrementa(&a);
    printf("%d\n", a[0]);
    incrementa(a);
    printf("%d\n", a[0]);
    incrementa(&a);
    printf("%d\n", a[0]);
}
```

1. 6, 7, 8
2. 5, 5, 5
3. 5, error, error
4. 5, dir memoria, basura

### Q3. ¿Que salida producirá probablemente el siguiente código en c?

```c

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct persona {
    char nombre[5];
    int edad;
};

int main() {
    struct persona *p = malloc(sizeof(struct persona));
    p->edad = 20;
    strcpy(p->nombre, "Maximiliano");

    printf("%s tiene %d años\n", p->nombre, p->edad);

    free(p);
}
```

1. Maximiliano tiene 20 años
2. Violación de segmento
3. Maximiliano tiene 7302753 años
4. Error de compilación

### Q4. ¿Con que secuencia de comandos GDB se puede imprimir el buffer completo?

```c
#include <stdlib.h> //Linea 1

int main() {
    int size = rand() % 1000;
    int * buffer = malloc(size);

    for (int i = 0; i < size; i++) {
        buffer[i] = rand() % 256;
    }

    //Mas código válido, linea 11

    free(buffer);
}

```

1. `b 11` `r` `p size` `x/elvalordesizexd buffer`
2. `b 11` `r` `p size` `x/elvalordesizexi buffer`
3. `p buffer`
4. `printf "%d\n", buffer`

### Q5. ¿Que hace en este caso la primera invocación de int 0x80?

```asm
section .data
    msg db "Hello, world!", 0

section .text
    global _start

_start:
    mov eax, 4
    mov ebx, 1
    mov ecx, msg
    mov edx, 13
    int 0x80

    mov eax, 1
    mov ebx, 0
    int 0x80
```

1. Escribe "Hello, world!" por la salida estándar.
2. Copia "Hello, world!" en el buffer de pantalla.
3. Llama al sistema para que escriba "Hello, world!" en la salida estándar.
4. Declara una variable de tipo entero con valor 0x80.

### Q6. ¿Que valor tiene el registro eax después de ejecutar el siguiente código en ensamblador?

```asm
section .text
    global _start

_start:
    mov eax, 4
    mov ax, 0x1234
    mov al, 0x56
```

1. 0x1234
2. 0x5600
3. 0x1256
4. 0x4

### Q7. ¿Como podemos montar una imagen de disco ext4 file.img en Linux?

1. `mount -o loop file.img /mnt -t ext4`
2. `cp file.img /mnt`
3. `fdisk -l file.img`
4. `mount file.img /mnt`

### Q8. ¿Como podemos crear de forma **más rápida** un fichero de exactamente 100 MB lleno de ceros?

1. `dd if=/dev/zero of=file.img bs=1M count=100`
2. `touch file.img`
3. `dd if=/dev/zero of=file.img bs=100M count=1`
4. `cat /dev/zero > file.img`

### Q9. ¿Que contiene el stack frame de una función en un sistema UNIX SYSTEMVR4 x64 clásico?

1. Todos los argumentos de la función, las variables locales y la dirección de retorno.
2. Algunos argumentos de la función, las variables locales y la dirección de retorno.
3. Las variables locales y la dirección de retorno.
4. Las variables locales y la dirección de retorno, pero no los argumentos.

### Q10. ¿Sabrías explicar en que consiste el Copy-on-Write?

1. Se que es, sabría explicarlo y podría implementarlo.
2. Se que es, sabría explicarlo pero no podría implementarlo.
3. Se que es pero no sabría explicarlo ni implementarlo.
4. No tengo ni idea de que es eso.

## Respuestas y tabla de puntuaciones

Una pregunta puede tener más de una respuesta correcta.

| Pregunta | Respuesta | Puntuación |
|----------|-----------|------------|
| Q1       | 3         | 1          |
| Q1       | 4         | 0.5        |
| Q2       | 1         | 1          |
| Q3       | 3         | 1          |
| Q3       | 2         | 0.5        |
| Q4       | 1         | 1          |
| Q5       | 1         | 0.5        |
| Q5       | 3         | 1          |
| Q6       | 3         | 1          |
| Q7       | 1         | 1          |
| Q7       | 4         | 0.5        |
| Q8       | 1         | 1          |
| Q9       | 2         | 1          |
| Q10      | 1         | 1          |
| Q10      | 2         | 0.5        |
| Q10      | 3         | 0.25       |

Como referencia:

7 o más: Tienes un nivel muy alto, estás bien preparado para el curso.
5 o 6: Tienes un buen nivel, encontrarás
algo de dificultad en el curso.
3 o 4: Tienes un nivel medio, deberías repasar algunos conceptos.
1 o 2: Tienes un nivel bajo, deberías repasar muchos conceptos antes de empezar el curso.

Si has obtenido una baja cualificación (0,1,2)
y aún así deseas realizar el curso, ponte en
contacto conmigo cuanto antes y haremos una
sesión de repaso previa.

### Explicación Q1:

Este ejercicio pretende evaluar el conocimiento básico
de punteros en C. La primera llamada a `incrementa` modifica
el valor de `a` en `main`. La segunda llamada a `incrementa`
trata de modificar el valor de la dirección de memoria que le 
hemos pasado, esto es el valor de `a` en `main`. Con lo que
el programa tratará de modificar la dirección de memoria `6`.
Por esto el programa fallará. Si 0x6 estuviese mapeado, entonces
imprimiría 6, 6, 7 ya que el segundo incremento no tendría efecto para nosotros.

### Explicación Q2:

En este caso se pretende obsevar una curiosidad que produce
muchos quebraderos de cabeza a los estudiantes de C. En el caso
de una lista a de enteros. `a` es un puntero a la primera posición, con lo que el compilador ignorará el operador de indirección `&`. Comprobadlo añadiendo esta linea al final
del código `printf("%p vs %p\n", &a, a);`

### Explicación Q3:

Este ejercicio evalua el conocimiento del desbordamiento de buffers. En este caso, el buffer `nombre` tiene un tamaño de 5 bytes. Sin embargo, estamos copiando 11 bytes en él. Debido a 
como organiza el compilador las estructuras, el buffer `edad` se encuentra justo después del buffer `nombre`. Con lo que estamos escribiendo en el y editando de forma impredecible el valor de `edad`. También es posible que sigamos escribiendo en memoria hasta provocar un error de segmentación.

### Explicación Q4:

Este ejercicio evalua el conocimiento de GDB. La secuencia de comandos es la siguiente:

1. `b 11` Establece un breakpoint en la linea 11.
2. `r` Ejecuta el programa.
3. `p size` Imprime el valor de la variable `size`.
4. `x/elvalordesizexd buffer` Imprime elvalordesize elementos
de tamaño d (double word) en hexadecimal de `buffer`.

### Explicación Q5:

Este ejercicio busca la profundidad en el conocimiento de los 
programas. Es cierto que este programa escribe "Hello, world!" en la salida estándar. Sin embargo, la instrucción `int 0x80` no escribe nada por si sola. Es el sistema operativo el que se encarga de interpretarla y realizar la llamada al sistema adecuada. En este caso, la llamada al sistema adecuada es la 4, que es la llamada al sistema `sys_write`. La llamada al sistema `sys_exit` es la 1.

En resumen, int 0x80 es una forma de llamar al sistema en Linux.

### Explicación Q6:

Este ejercicio evalua el conocimiento de la arquitectura x86. El registro `eax` es el registro de 32 bits. Los registros `ax` y `al` son los registros de 16 y 8 bits respectivamente. La instrucción `mov eax, 4` escribe el valor 4 en el registro `eax`. La instrucción `mov ax, 0x1234` escribe el valor 0x1234 en el registro `ax`, que es el registro de 16 bits menos significativos de `eax`. Esto reescribe el 4 que habíamos escrito antes. La instrucción `mov al, 0x56` escribe el valor 0x56 en el registro `al`, que es el registro de 8 bits menos significativos de `ax`. Esto reescribe el 0x34 que habíamos escrito antes. Siendo el resultado final 0x1256.

### Explicación Q7:

Esta pregunta pretende evaluar el conocimiento de la línea de comandos. La respuesta correcta es la 1. La opción `-o loop` es necesaria para montar un fichero como si fuese un dispositivo. La opción `-t ext4` es necesaria para indicar que el fichero es de tipo ext4.

### Explicación Q8:

En este caso tratamos de evaluar el conocimiento del usuario en lo que a dispositivos se refiere. Existen dos tipos de dispositivos habituales: Dispositivos de caracter y de bloque. En el caso de los segundos, podemos transferir datos en bloques de tamaño fijo. La opción `bs` indica el tamaño de bloque y la opción `count` el número de bloques. La respuesta correcta será la 3 en el caso de que nuestro disco soporte bloques de 100M. La respuesta correcta será la 1 en el caso de que nuestro disco no soporte bloques de 100M.

### Explicación Q9:

Este ejercicio plantea el concepto de marco de función. El marco de función es una estructura de datos que se encuentra en la pila y que contiene los argumentos de la función, las variables locales y la dirección de retorno. Es importante entender que esto queda supeditado al Application Binary Interface (ABI) que se esté utilizando. En el caso del ABI de SystemV R4, algunos parámetros se pasan por registros y otros por la pila. Por eso
la respuesta correcta es la 2.

### Explicación Q10:

Este ejercicio plantea al alumno una pregunta
simple pero con una respuesta compleja. El Copy-on-Write. Si se domina este concepto, se
asume un dominio de la memoria virtual.