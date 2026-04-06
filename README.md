# Visualizar memoria RAM fisica en tiempo real
### (En proceso)

Este proyecto consiste en una herramienta diseñada para leer las paginas fisicas perteneciestes a la memoria RAM directamente desde el **Kernel Space**, transferir los datos de manera eficiente al **User Space** y, posteriormente, procesarlos para su visualización gráfica.

### Significado de los colores de las páginas

- Rojo (VOID): Representa la discrepancia entre la cantidad de páginas teóricas y las páginas físicas detectadas por el sistema como RAM.

- Azul (RESE): Páginas reservadas exclusivamente por el Kernel. Incluye la imagen binaria del kernel y sus estructuras esenciales.

- Verde oscuro (PGTB): Páginas destinadas al almacenamiento de las Page Tables (tablas de páginas), necesarias para la traducción de direcciones.

- Amarillo (COMP): Páginas compuestas (Compound Pages) utilizadas para gestionar Huge Pages. Incluye tanto Transparent Huge Pages (THP) como hugetlbfs.

- Naranja (FILE): Páginas de caché utilizadas para el mapeo de archivos desde el almacenamiento hacia la RAM.

- Violeta (ANON): Páginas de memoria anónima. Son aquellas utilizadas por procesos en User Space para almacenar datos dinámicos (como el stack y el heap).

- Marrón (USER): Páginas referenciadas por tablas de procesos en User Space que no entran en la categoría de anónimas ni están vinculadas a archivos.

- Verde claro (FREE): Páginas que no tienen referencias activas y están disponibles para ser asignadas.

- Negro: Dentro del bloque que representa la memoria RAM, hay páginas que no entran en ninguna de las categorías anteriores y el kernel las utiliza para realizar otras tareas.


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

### Fuentes 

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
