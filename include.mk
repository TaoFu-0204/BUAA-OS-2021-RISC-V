# Common includes in Makefile
#
# Copyright (C) 2007 Beihang University
# Written By Zhu Like ( zlike@cse.buaa.edu.cn )

# Exercise 1.1. Please modify the CROSS_COMPILE path.

# CROSS_COMPILE :=  bin/mips_4KC-
#CROSS_COMPILE := /OSLAB/compiler/usr/bin/mips_4KC-
CROSS_COMPILE := riscv64-unknown-elf-
CC                        := $(CROSS_COMPILE)gcc
CFLAGS            := -O -g  -fno-builtin -Wall -fPIC 
CFLAGS            += -mcmodel=medany 
LD                        := $(CROSS_COMPILE)ld
