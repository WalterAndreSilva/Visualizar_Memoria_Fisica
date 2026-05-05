#!/bin/bash

make
sudo insmod pages_reserved.ko
read -p "PAGINAS RESERVADAS: Presiona ENTER para continuar..."
sudo rmmod pages_reserved
# make clean
