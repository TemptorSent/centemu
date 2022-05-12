CC = gcc
CFLAGS = -Wall -Wno-unused -ggdb

.PHONY: all

all: romcat-uC

romcat-uC: romcat-uC.o ../ginsumatic.o ../am2901.o ../am2909.o ../clockline.o ../rom-common.o

romcat-uC.o: comments_generated.h bootstrap_rom.h
romcat-uC.c: comments_generated.h bootstrap_rom.h

comments_generated.h: generate-comments.sh
	$(SHELL) generate-comments.sh

bootstrap_rom.h: rom2include
	./rom2include ../../Bootstrap.rom bootstrap_rom  > bootstrap_rom.h

rom2include: rom2include.o
