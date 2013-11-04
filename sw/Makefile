# Commands
CC = or1k-elf-gcc
CPP = or1k-elf-g++
OBJCOPY = or1k-elf-objcopy
OBJDUMP = or1k-elf-objdump
REMOVE = rm -rf
# Compiler flags
CFLAGS = -c -Wall -std=c99 -O2 -I./ -Idrivers/ -Isynth/
# Linker flags
LDFLAGS = -mnewlib -lm

# Sources
TARGET = main.c
SRC = $(TARGET)
SRC+= board_conf.c
SRC+= irq.c
SRC+= timer.c
SRC+= synth/sublime.c
SRC+= drivers/ssm2603.c
SRC+= drivers/opencores_i2c.c
SRC+= drivers/mmiomidi.c
SRC+= drivers/midi.c

# Output filename
OUT = sublime

# Object defines
COBJ = $(SRC:.c=.o)

all: $(SRC) $(OUT) $(OUT).bin

$(OUT).bin: $(OUT)
	$(OBJCOPY) -O binary $< $@

$(OUT): $(COBJ)
	$(CC) $(COBJ) $(LDFLAGS) -o $@

$(COBJ) : %.o : %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(REMOVE) $(COBJ) $(OUT) $(OUT).bin
