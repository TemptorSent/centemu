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

INCLUDES = \
	   include/comments_generated.h \
	   include/bootstrap_rom.h \
	   include/uCode_rom.h \
	   include/maprom_rom.h

uCemu.o: ${INCLUDES}
uCemu.c: ${INCLUDES}

include/comments_generated.h: Comments/generate-comments.sh
	$(SHELL) ./Comments/generate-comments.sh > include/comments_generated.h

include/bootstrap_rom.h: ../tools/rom2include
	../tools/rom2include --name bootstrap_rom ../../Bootstrap.rom > include/bootstrap_rom.h

include/maprom_rom.h: ../tools/rom2include
	../tools/rom2include --name maprom ./ROMs/CPU_UB13_MAPROM_6309.rom > include/maprom_rom.h

include/uCode_rom.h: ../tools/romcat ../tools/rom2include
	../tools/romcat --stride=7 --format=b \
		./ROMs/CPU_UH3_MWK3.11_A3.11.rom \
		./ROMs/CPU_UL3_MWF3.11_B3.11.rom \
		./ROMs/CPU_UK3_MWH3.11_C3.11.rom \
		./ROMs/CPU_UF3_MWL3.11_D3.11.rom \
		./ROMs/CPU_UE3_MWM3.11_E3.11.rom \
		./ROMs/CPU_UJ3_MWJ3.11_F3.11.rom \
		./ROMs/CPU_UM3_MWE3.11_M3.11.rom \
		| ../tools/rom2include.exe --word-size=7 --name=uCode_rom - \
		> include/uCode_rom.h

../tools/rom2include: ../tools/rom2include.o

../tools/romcat: ../tools/romcat.o
