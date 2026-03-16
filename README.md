# Visualizar memoria RAM fisica en tiempo real
### (En proceso)

Este proyecto consiste en una herramienta diseñada para leer la memoria RAM física directamente desde el **Kernel Space**, transferir los datos de manera eficiente al **User Space** y, posteriormente, procesarlos para su visualización gráfica.

## Instalar librerias de desarrollo del kenrel
Instalar las herramientas esenciales y los encabezados del kernel actual: 
```bash
$ sudo apt update && sudo apt install build-essential linux-headers-$(uname -r)
```
Instalar librerias de OpenGL
```bash
$ sudo apt-get install libglfw3 libglfw3-dev
```
Son necesarias para poder compilar la parte grafica. 

## Compilacion 

Se incluye un Makefile que automatiza la compilación tanto del módulo del kernel como de las herramientas de usuario. Esto abarca la aplicación gráfica de visualización y un programa de prueba diseñado para llenar la memoria con datos aleatorios.

## Ejecutar el programa

### Cargar nodulo en el Kernel:

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
 
