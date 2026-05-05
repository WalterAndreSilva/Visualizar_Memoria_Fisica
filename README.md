# Visualizar memoria RAM fisica en tiempo real
#### (En proceso)

## Descripción General

Este proyecto consiste en una herramienta diseñada para leer las páginas físicas pertenecientes a la memoria RAM directamente desde el **Kernel Space**, transferir los datos de manera eficiente al **User Space** y, posteriormente, procesarlos para su visualización gráfica interactiva. El desarrollo se llevó a cabo utilizando el kernel de **Linux 6.14** con 16 GB de memoria RAM. Para cambiar esta cantidad de memoria o ajustar otras características de la visualización, se puede modificar el archivo `conf.h`.

El núcleo del proyecto radica en un módulo cargable llamado `mmap_kernel`, responsable de clasificar las páginas y mantener la información actualizada. Al inicializarse, el módulo recorre todos los Page Frame Numbers (PFN) y almacena aquellos marcados como memoria RAM. Cabe destacar que esta cantidad detectada es menor que la capacidad teórica de la RAM, ya que la BIOS reserva regiones de memoria durante la creación del mapa de memoria (BIOS-e820). Este mapa de memoria se puede observar en el archivo /proc/iomem. En la representación gráfica, la memoria reservada por la BIOS se muestra en color rojo.

Una vez que cada página es clasificada, la información se envía al espacio de usuario. Para esto, se estableció un archivo llamado `ku_mmap` como método de comunicación, aprovechando las ventajas de rendimiento que ofrece mmap. Finalmente, la aplicación de usuario `mmap_user` se encarga de recibir estos datos y renderizar los gráficos utilizando OpenGL, incorporando además la capacidad de realizar zoom y desplazarse libremente sobre la visualización.

### Teclas

- **F**: Activar o desactivar la pantalla completa.
- **R**: Resetear posicion y zoom.
- **I**: Mostrar la información de las páginas.
- **0-8**: Mostrar u ocultar la categoría de la página.
- **A**: Seleccionar todas las páginas o ninguna.
- **X**: Invertir la selección actual.
- **Z**: Alternar entre la vistas y a que zona pertenece.
- **S**: Mostrar estado de la página.
- **W** y **E**: teclas auxiliares para hacer zoom.
- **Teclas de dirección**: Desplazarse por la textura.


### Significado de los colores de las páginas

#### Vista de usos
 
- Rojo (VOID): Representa los páginas reservadas por la BIOS.

- Azul (RESE): Páginas reservadas exclusivamente por el Kernel. Incluye la imagen binaria del kernel y sus estructuras esenciales.

- Verde azulado (SLAB): Paginas utilizadas por el gestor de memoria Slab.

- Verde oscuro (PGTB): Páginas destinadas al almacenamiento de las Page Tables (tablas de páginas), necesarias para la traducción de direcciones.

- Amarillo (COMP): Páginas compuestas (Compound Pages) utilizadas para gestionar Huge Pages. Incluye tanto Transparent Huge Pages (THP) como hugetlbfs.

- Naranja (FILE): Páginas de caché utilizadas para el mapeo de archivos desde el almacenamiento hacia la RAM.

- Violeta (ANON): Páginas de memoria anónima. Son aquellas utilizadas por procesos en User Space para almacenar datos dinámicos (como el stack y el heap).

- Marrón (USER): Páginas referenciadas por tablas de procesos en User Space que no entran en la categoría de anónimas ni están vinculadas a archivos.

- Verde claro (FREE): Páginas que no tienen referencias activas y están disponibles para ser asignadas.

- Magenta (KERN): Hay páginas que no entran en ninguna de las categorías anteriores y el kernel las utiliza para realizar otras tareas.

Al desactivar una categoría, se puede observar a qué otra categoría pertenece la página.

#### Vista de zonas

Esta vista muestra la distribución física de la RAM, por lo tanto, es estática y no varía con el tiempo.

- Rosa (Zona DMA): Históricamente, representa los primeros 16 MB de memoria física. Está reservada para hardware antiguo que utiliza un bus de direcciones de 24 bits. La tecnología DMA (Direct Memory Access) permite a estos periféricos transferir datos directamente hacia y desde la RAM sin la intervención constante de la CPU.

- Gris (Zona DMA32): Espacio que abarca desde los 16 MB y puede extenderse hasta un máximo de 4 GB. Está reservado para garantizar que los dispositivos y controladores limitados a una arquitectura de 32 bits tengan un lugar donde escribir y leer datos mediante DMA.

- Cian (Zona Normal): Abarca toda la memoria física restante a partir de los 4 GB. Es el área de trabajo principal del sistema operativo y los procesos de usuario, ya que los procesadores modernos de 64 bits pueden mapearla directamente sin restricciones. Solo cuando las páginas de esta zona se agotan, el kernel recurre a las zonas DMA y DMA32 como respaldo.

#### Vista de estados

Las páginas de usuario pueden ser modificadas y, tras un intervalo, los cambios se sincronizan en el almacenamiento permanente. Esta transición es especialmente visible durante la copia de archivos de gran tamaño.

- Celeste (WB): Páginas en transito desde la RAM hacia el disco duro.

- Lima (DIRTY): Páginas modificadas en la RAM, pendientes de ser guardarse en disco.

## Instalar librerias

Instalar las herramientas esenciales y los encabezados del kernel actual: 
```bash
$ sudo apt update && sudo apt install build-essential linux-headers-$(uname -r)
```
Instalar librerias de OpenGL para poder compilar la parte grafica. 
```bash
$ sudo apt-get install libglfw3 libglfw3-dev
```
Instalar GLEW para ejecutar shader en la tarjeta grafica.
```bash
$ sudo apt-get install libglew-dev 
```

## Compilacion 

Se incluye un Makefile que automatiza la compilación tanto del módulo del kernel como de las herramientas de usuario. Esto abarca la aplicación gráfica de visualización y un programa de prueba diseñado para llenar la memoria con datos aleatorios.

## Ejecutar el programa

### Cargar modulo en el Kernel:

```bash
$ sudo insmod mmap_kernel.ko
```
Puedes confirmar la carga del módulo filtrando la salida de **lsmod** con el comando **grep**.

```bash
$ lsmod | grep mmap_kernel
```

### Quitar modulo:

```bash 
$ sudo rmmod mmap_kernel
```
### Leer log del kernel:

```bash
$ sudo dmesg | tail
```

### Ejecutar aplicacion grafica: 

```bash
$ sudo ./mmap_user
```
### Ejecutar programa que llena la memoria: 

```bash
$ sudo ./full_mem
```

## Fuentes 

 Stack overflow
 
 How to mmap a Linux kernel buffer to user space? 
 https://stackoverflow.com/questions/10760479/how-to-mmap-a-linux-kernel-buffer-to-user-space

 Github
 
 Linux Kernel Module Cheat: 
 https://github.com/cirosantilli/linux-kernel-module-cheat/blob/2ea5e17d23553334c23934d83965de8a47df3780/kernel_modules/mmap.c
 
 Codigo fuente del Kernel de linux 6.14

 https://elixir.bootlin.com/linux/v6.14/source/

 Memory Management Documentation (Incopleto)
 
 https://www.kernel.org/doc/html/latest/mm/

 Direccionamiento de memoria y E/S mapeada en memoria
 
 https://medium.com/@tom_84912/memory-addressing-and-memory-mapped-i-o-7b602958da94

 
