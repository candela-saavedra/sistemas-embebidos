#TOOLCHAIN=~/toolchain/gcc-arm-none-eabi-4_9-2014q4/bin
#PREFIX=$(TOOLCHAIN)/arm-none-eabi-
PREFIX=arm-none-eabi-

# Nivel de optimizacion: make OPT=-O0 o make OPT=-Ofast
OPT?=-O0

ARCHFLAGS=-mthumb -mcpu=cortex-m0plus
COMMONFLAGS=-g3 $(OPT) -Wall -Werror $(ARCHFLAGS)

CFLAGS=-I./includes $(COMMONFLAGS)
LDFLAGS=$(COMMONFLAGS) -T link.ld -Wl,--gc-sections,-Map,$(TARGET).map
LDLIBS=-lgcc

CC=$(PREFIX)gcc
LD=$(PREFIX)gcc
NM=$(PREFIX)nm
RM=rm -f

TARGET=main

SRC=$(wildcard *.c)
ASRC=$(wildcard *.S)
OBJ=$(patsubst %.c, %.o, $(SRC)) $(patsubst %.S, %.o, $(ASRC))

all: elf
elf: $(TARGET).elf

clean:
	$(RM) $(TARGET).srec $(TARGET).elf $(TARGET).bin $(TARGET).map $(OBJ) .optflags

# Guarda el ultimo OPT usado para recompilar todo cuando cambia
.optflags: FORCE
	@echo '$(OPT)' | cmp -s - $@ || echo '$(OPT)' > $@

$(OBJ): .optflags

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) $(LDLIBS) -o $@

flash: all
	openocd -f openocd.cfg -c "program $(TARGET).elf verify reset exit"

# Lee los resultados de la placa (despues de make flash)
results: all
	NM=$(NM) sh results.sh $(TARGET).elf '$(OPT)'

FORCE:

.PHONY: all elf clean flash results FORCE
