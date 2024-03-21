# El proceso de arranque del sistema

Clona ahora el tag s0.boot.0.1 y comprueba que compila y ejecuta.

```bash
git clone
```
## Introducción, antes de arrancar

Un sistema x86, arranca cuando presionamos el botón de encendido. En ese momento, el procesador comienza a ejecutar instrucciones almacenadas en la memoria ROM de la placa base. Estas instrucciones primero
se aseguran de que el hardware está en un estado conocido y apto para el arranque, a este proceso se le
conoce como POST (Power On Self Test). Una vez que el hardware está listo, la BIOS comienza a ejecutarse
en una dirección fija conocida como vector de reinicio. La BIOS leerá su configuración de arranque y buscará, en orden, un dispositivo válido desde el que iniciar el boot. Lo cargará en memoria y le cederá el control.

## El sector de arranque del disco duro

Un disco duro historicamente se dividía en cilindros, cabezales (header) y sectores ( direccionamiento CHS). La bios habla este lenguaje y es capaz de leer y escribir en el disco duro utilizando estos valores. En el mundo moderno, los discos duros se dividen únicamente en sectores y se accede a ellos mediante el direccionamiento LBA (Logical Block Addressing). Para pasar de CHS a LBA y viceversa basta con saber el número de sectores por pista, el número de pistas por cilindro y el número de cilindros. La bios es capaz de hacer esta conversión.

Un disco duro tiene un sector especial llamado sector de arranque. Este sector es el primero del disco y contiene el código de arranque del sistema operativo. El sector de arranque tiene 512 bytes de longitud y se encuentra en el cilindro 0, cabeza 0, sector 1. 

## Formato de particiones

Existen dos formatos de particiones ampliamente conocidos: MBR y GPT. El primero es el más antiguo y el segundo es el más moderno.

### MBR

MBR permite hasta 4 particiones primarias, definidas en entradas de 16 bytes cada una. Si se necesitan más particiones, se pueden definir particiones extendidas, que contienen particiones lógicas.

Esta es la estructura de un sector MBR:

```
0x0000 446 bytes Código de arranque
0x01BE 64 bytes Entradas de partición, 4x16 bytes
0x01FE 2 bytes Firma MBR 0x55AA
```

El código de arranque es código binario ejecutable que se encargará de cargar el sistema operativo. Es 
lo que se conoce como la primera etapa del cargador de arranque.

Cada entrada de partición tiene la siguiente estructura:

```
0x00 1 byte Estado de la partición
0x01 3 bytes CHS del primer sector de la partición
0x04 1 byte Tipo de partición
0x05 3 bytes CHS del ultimo sector de la partición
0x08 4 bytes LBA del primer sector de la partición
0x0C 4 bytes Tamaño de la partición en sectores
```

El estado de la partición puede ser 0x00 (inactiva) o 0x80 (activa). La partición activa es la que se arranca. Aunque muchas veces se ignora el estado o solamente se comprueba que no sea 0x00.

El tipo de partición se corresponde con una tabla de tipos estandar: https://es.wikipedia.org/wiki/C%C3%B3digo_de_tipo_de_partici%C3%B3n#C%C3%B3digos_para_particiones_en_MBR

El MBR termina con la firma 0x55AA. Estos dos bytes son la única forma de saber si un sector es un MBR válido y es lo que buscará la BIOS para decidir si arrancar o pasar al siguiente dispositivo.

### GPT

GPT es el formato de particiones más moderno. Permite hasta 128 particiones primarias y no tiene las limitaciones de MBR. Además, GPT es más seguro que MBR, ya que tiene redundancia en la tabla de particiones y contiene un CRC32.

GPT es el formato de particiones que utilizan los discos duros modernos y los discos duros de más de 2TB,
es el único formato de particiones que soporta UEFI.

La estructura de un disco con particiones GPT es la siguiente:

```
0x0000 512 bytes MBR clasico para retrocompatibilidad
0x0200 512 bytes Cabecera GPT
0x0400 128 bytes Entradas de partición GPT
0xM        PARTICIONES
0xN    128 bytes Copia de las entradas de partición GPT
0xL    512 bytes Copia de la cabecera GPT
```
M,N,L son valores arbitrarios, L es el último sector del disco, N va desde el final de M hasta L.

Cada entrada de partición ocupa 128 bytes y tiene la siguiente estructura:

```
0x00 16 bytes Tipo de partición
0x10 16 bytes Identificador de partición único (GUID)
0x20 8 bytes Primer LBA de la partición
0x28 8 bytes Ultimo LBA de la partición
0x30 8 bytes Atributos de la partición
0x38 72 bytes Nombre de la partición
```

## El cargador de arranque de primer nivel

Asumamos ahora el caso de que el disco duro tiene un sector de arranque MBR. Este sector contiene un cargador de arranque de primer nivel. El cargador de arranque de primer nivel es un pequeño programa que se encarga de cargar el cargador de arranque de segundo nivel. Esto lo hace llamando a la BIOS para que
cargue N sectores del disco duro en memoria y le ceda el control mediante un salto. 

Como hemos observado durante el estudio de MBR, el cargador de arranque solo dispone de 446 bytes para su código. Este espacio es muy limitado y no permite hacer muchas cosas. Como curiosidad existen competiciones de programación en las que se intenta hacer programas útiles que quepan en este espacio.

## El cargador de arranque de segundo nivel es parte del sistema operativo

Aquí decide el programador que hacer. Podemos empezar cargando una imagen ejecutable (elf), o podemos
ejecutar código residente en memoria. Independientemente del orden, es importante pasar por unos pasos
determinados para que nuestro entorno de ejecución sea el adecuado. 

Asumiendo que estamos en una arquitectura x86-64, los pasos a seguir son los siguientes:

1. Desactivar interrupciones, vaciar registros y poner la pila en un lugar conocido.
2. Activar la línea A20, cargar la tabla de descriptores globales (GDT) y la tabla de descriptores de interrupciones (IDT).
3. Configurar la tabla de páginas, entrar en modo de segmentación plana y entrar en modo protegido.
4. Habilitar PAE (Physical Address Extension) y cargar la tabla de páginas de nivel 4.
5. Cambiar a modo largo (64 bits) y cargar la tabla de páginas de nivel 5 si la hubiera.

Una vez que hemos llegado a este punto, podemos cargar el kernel del sistema operativo y cederle el control. El kernel se encargará de inicializar el hardware, cargar los drivers y el resto del sistema operativo.

### Línea A20

La línea A20 es un hack con el que hemos de lidiar. Antiguamente los procesadores podían direccionar 2^19
bytes de memoria, es decir, 512KB. El espacio de direcciones, en caso de emitir una que superase este rango comenzaba a trabajar en módulo 19. Es decir, si queríamos acceder al KB 513, estaríamos accediendo
en realidad al KB 1. Este comportamiento lo aprovecharon algunos programadores para ahorrarse el cambio de segmentos y acceder a la memoria de forma más rápida. Sin embargo, cuando se empezaron a fabricar CPUs con más líneas en el bus de direcciones, este hack de módulo 19 dejó de funcionar. Para evitar que todos los programas que lo aprovechaban quedasen inservibles, se decidió que las CPUs empezarían a trabajar con 19 líneas de dirección y que mediante una señal, se podría activar la línea 20. Aún hoy día debemos lidiar con este hack. (Salvo que usemos UEFI, que no tiene este problema).

### Tabla de descriptores globales (GDT)

La GDT es una tabla que contiene descriptores de segmento. Cada descriptor de segmento contiene información sobre un segmento de memoria. La GDT es necesaria para entrar en modo protegido. La GDT se carga en el registro GDTR. En realidad esta tabla es bastante inútil en el mundo de los 64 bits, con definir los siguientes segmentos es suficiente:

- El segmento de código para el kernel.
- El segmento de datos para el kernel.
- El segmento de código para el usuario.
- El segmento de datos para el usuario.

### Tabla de descriptores de interrupciones (IDT)

La IDT es una tabla que contiene descriptores de interrupción. Cada descriptor de interrupción contiene información sobre una interrupción. La IDT es necesaria para manejar las interrupciones. La IDT se carga en el registro IDTR. Veremos más adelante como se configura.