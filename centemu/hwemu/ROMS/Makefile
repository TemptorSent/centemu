CC = gcc
CFLAGS = -Wall -Wno-unused -ggdb

.PHONY: all

all: romcat-uC

romcat-uC: romcat-uC.o ../ginsumatic.o ../am2901.o ../am2909.o ../clockline.o ../rom-common.o

romcat-uC.o: comments_generated.h
romcat-uC.c: comments_generated.h

comments_generated.h: generate-comments.sh
	$(SHELL) generate-comments.sh
