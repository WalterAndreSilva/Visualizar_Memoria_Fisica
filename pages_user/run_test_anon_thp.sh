#!/bin/bash

#  Prueba sobre comportamiento de las THP con distintos pedidos de memoria
#  NOTA: No olvidar compilar los programas antes de ejecutar este archivo

echo -e "\e[36m\nPERMITIR THP CUANDO SEA POSIBLE\e[0m"
sudo sh -c "echo always > /sys/kernel/mm/transparent_hugepage/enabled"
cat /sys/kernel/mm/transparent_hugepage/enabled
./pages_anon_thp

