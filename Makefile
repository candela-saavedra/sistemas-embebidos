#TOOLCHAIN=~/toolchain/gcc-arm-none-eabi-4_9-2014q4/bin
#PREFIX=$(TOOLCHAIN)/arm-none-eabi-
PREFIX=arm-none-eabi-

CC=$(PREFIX)gcc
LD=$(PREFIX)gcc
GDB=gdb-multiarch
RM=rm -f

ARCHFLAGS=-mthumb -mcpu=cortex-m0plus
COMMONFLAGS=-g3 -O0 -Wall $(ARCHFLAGS)

# Simbolos que piden los ejemplos de la SDK
DEFS=-DCPU_MKL46Z256VLL4 -DFRDM_KL46Z -DFREEDOM \
     -DPRINTF_FLOAT_ENABLE=0 -DSCANF_FLOAT_ENABLE=0 \
     -DPRINTF_ADVANCED_ENABLE=0 -DSCANF_ADVANCED_ENABLE=0

INCLUDES=-I./board -I./drivers -I./utilities -I./device -I./CMSIS

CFLAGS=$(COMMONFLAGS) $(DEFS) $(INCLUDES) -std=gnu99 \
       -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fno-common
LDFLAGS=$(COMMONFLAGS) -T link.ld -nostartfiles \
        -specs=nano.specs -specs=nosys.specs \
        -Wl,--gc-sections,-Map,$(basename $@).map
LDLIBS=-lc -lgcc

TARGETS=led_blinky hello_world
ELFS=$(addsuffix .elf, $(TARGETS))

# Codigo comun a los dos binarios
COMMON_SRC=startup.c $(wildcard board/*.c drivers/*.c utilities/*.c device/*.c)
COMMON_OBJ=$(patsubst %.c, %.o, $(COMMON_SRC))

LED_OBJ=$(patsubst %.c, %.o, $(wildcard led_blinky/*.c))
HELLO_OBJ=$(patsubst %.c, %.o, $(wildcard hello_world/*.c))

.PHONY: all clean flash_led flash_hello debug_led debug_hello

all: $(ELFS)

# Cada ejemplo tiene su propio pin_mux.h
led_blinky/%.o: CFLAGS += -I./led_blinky
hello_world/%.o: CFLAGS += -I./hello_world

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

led_blinky.elf: $(LED_OBJ) $(COMMON_OBJ)
hello_world.elf: $(HELLO_OBJ) $(COMMON_OBJ)

$(ELFS):
	$(LD) $(LDFLAGS) $^ $(LDLIBS) -o $@

flash_led: led_blinky.elf
	openocd -f openocd.cfg -c "program $< verify reset exit"

flash_hello: hello_world.elf
	openocd -f openocd.cfg -c "program $< verify reset exit"

# Depuracion: lanza openocd como servidor y conecta GDB
debug_led: led_blinky.elf
	$(GDB) $< -ex "target extended-remote | openocd -f openocd.cfg -c 'gdb_port pipe; log_output openocd.log'"

debug_hello: hello_world.elf
	$(GDB) $< -ex "target extended-remote | openocd -f openocd.cfg -c 'gdb_port pipe; log_output openocd.log'"

clean:
	$(RM) $(ELFS) $(addsuffix .map, $(TARGETS)) $(COMMON_OBJ) $(LED_OBJ) $(HELLO_OBJ) openocd.log
