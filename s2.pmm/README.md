# Manejo de memoria física

El manejo de memoria física es una de las tareas más importantes de un sistema operativo. El sistema operativo debe ser capaz de gestionar la memoria física del sistema, asignando y liberando memoria a los procesos, y protegiendo la memoria del sistema de accesos no autorizados.

## La memoria física

La memoria física es la RAM que todos conocéis, sin mayor complicación. El único detalle a tener en cuenta es que cuando empezamos a trabajar, es muy probable que ya haya partes de la memoria ocupadas por el firmware de la placa base, por ejemplo, o por otros dispositivos. Por tanto, no podemos asumir que la memoria empieza en la dirección 0x0.
La BIOS (o uefi) nos permite obtener un mapa de la memoria física del sistema. Debemos trabajar con él para obtener
una lista de direcciones que podemos asignar a los procesos y al propio sistema operativo.

## Cómo trabajar con la memoria física

Los pasos son relativa sencillos:

1. Obtener el mapa de memoria de la BIOS o UEFI.
2. Limpiar la memoria que está en uso pero que no necesitamos.
3. Crear una estructura que mapee la memoria física y nos permita llevar la cuenta de qué direcciones están libres y cuáles no.
4. Crear un sistema de asignación y liberación de memoria.

### Obtener el mapa de memoria

En BIOS, podemos obtener el mapa de memoria (EXTENDED BIOS DATA AREA) mediante la interrupción 15h, EAX=E820h. El
uso de esta función, como todo lo relacionado con la BIOS, es bastante arbitrario:

1. Ponemos en EAX el valor 0xE820 (Con leading 0).
2. Ponemos a cero EBX.
3. Ponemos en ECX el tamaño de la estructura que queremos que nos devuelva. En nuestro caso, 24 bytes.
4. Ponemos en EDX el valor 0x534D4150 (SMAP en ASCII).
5. En ES:DI ponemos la dirección de memoria donde queremos que la BIOS nos devuelva el mapa de memoria.
6. Ejecutamos la interrupción 15h.

En UEFI, este proceso es mucho más sencillo. La UEFI nos proporciona una estructura llamada EFI_MEMORY_DESCRIPTOR que
contiene el mapa de memoria. La estructura es la siguiente:

```c
typedef struct {
  UINT32 Type;
  EFI_PHYSICAL_ADDRESS PhysicalStart;
  EFI_VIRTUAL_ADDRESS VirtualStart;
  UINT64 NumberOfPages;
  UINT64 Attribute;
} EFI_MEMORY_DESCRIPTOR;
```

Para obtener el mapa de memoria, simplemente llamamos a la función GetMemoryMap de la UEFI. Esta función nos devolverá
un puntero a la estructura EFI_MEMORY_DESCRIPTOR.

En Limine, que es el cargador de arranque que vamos a utilizar, podemos obtener el mapa de memoria mediante la directiva `LIMINE_MEMMAP_REQUEST`. Esta directiva nos devolverá un puntero a una estructura que contiene el mapa de memoria.

El formato de la respuesta es el siguiente:

```c
struct limine_memmap_response {
    uint64_t revision;
    uint64_t entry_count;
    struct limine_memmap_entry **entries;
};
```

Cada entrada de la estructura es un puntero a una estructura de tipo `limine_memmap_entry`. Esta estructura es la siguiente:

```c
// Constants for `type`
#define LIMINE_MEMMAP_USABLE                 0
#define LIMINE_MEMMAP_RESERVED               1
#define LIMINE_MEMMAP_ACPI_RECLAIMABLE       2
#define LIMINE_MEMMAP_ACPI_NVS               3
#define LIMINE_MEMMAP_BAD_MEMORY             4
#define LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE 5
#define LIMINE_MEMMAP_KERNEL_AND_MODULES     6
#define LIMINE_MEMMAP_FRAMEBUFFER            7

struct limine_memmap_entry {
    uint64_t base;
    uint64_t length;
    uint64_t type;
};
```

De momento, solo nos vamos a centrar en LIMINE_MEMMAP_USABLE, aunque podríamos emplear el resto de tipos para
optimizar el uso de la memoria.

### Gestionar la memoria disponible

Una vez tengamos una lista de bloques utilizables, podemos dividirlos en bloques más pequeños y gestionarlos mediante
una estructura de datos que nos permita saber qué bloques están libres y cuáles no.

La estructura de datos que vamos a utilizar es un bitmap. Un bitmap es una estructura de datos que nos permite representar un conjunto de bits. En nuestro caso, cada bit representará un bloque de memoria. Si el bit está a 1, el bloque está ocupado. Si el bit está a 0, el bloque está libre. El tamaño de bloque lo decidimos nosotros, pero es habitual que sea de 4KB para que coincida con el tamaño de página habitual.

### Asignar y liberar memoria

En este esquema, la asignación y liberación de memoria es trivial. Para asignar memoria, simplemente buscamos un bloque libre en el bitmap y lo marcamos como ocupado. Para liberar memoria, simplemente buscamos el bloque ocupado en el bitmap y lo marcamos como libre.

Las primities de asignación y liberación de memoria que emplearemos son las siguientes:

```c
void *request_page(); // Asigna una página de memoria
void free_page(void *page); // Libera una página de memoria
void *request_contiguous_pages(size_t count); // Asigna un número de páginas de memoria contiguas
void free_contiguous_pages(void *page, size_t count); // Libera un número de páginas de memoria contiguas
```