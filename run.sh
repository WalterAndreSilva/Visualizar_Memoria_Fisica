#!/bin/bash

# Este archivo ejecuta los comando necesarios para la compilacion,
# carga del modulo, ejecucion, quitar el modulo y limpieza de los
# archivos de compilacion, en ese orden.

make
sudo insmod mmap_kernel.ko
./mmap_user
sudo rmmod mmap_kernel
make clean
