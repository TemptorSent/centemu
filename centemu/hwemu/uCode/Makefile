CC = gcc
CFLAGS = -Wall -Wno-unused -ggdb

.PHONY: all

all: uCemu

uCemu: \
	../common/ginsumatic.o \
	../components/am2901.o \
	../components/am2909.o \
	../common/clockline.o \
	../common/rom-common.o \
	uCemu.o


uCemu.o: comments_generated.h bootstrap_rom.h
uCemu.c: comments_generated.h bootstrap_rom.h

comments_generated.h: generate-comments.sh
	$(SHELL) generate-comments.sh

bootstrap_rom.h: rom2include
	../tools/rom2include ../../Bootstrap.rom bootstrap_rom  > bootstrap_rom.h

rom2include: ../tools/rom2include.o
