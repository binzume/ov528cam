CC=avr-gcc
CFLAGS=-std=c99 -O2 -Wall
TARGET=atmega328p
#AVR_F_CPU=8000000
AVR_F_CPU=20000000
AVR_WRITER=hidspx
AVR_WRITER_OPT=-ph

APP=cam_test
SRC=cam_test.c
ELF=$(APP).elf

all: $(APP).hex

%.hex: $(SRC)
	avr-gcc $(CFLAGS) -mmcu=$(TARGET) -D F_CPU=$(AVR_F_CPU) -o $(ELF) $(SRC)
	avr-objcopy -O ihex -R .eeprom $(ELF) $@
	rm $(ELF)

write: $(APP).hex
	@echo writing $? ...
	$(AVR_WRITER) $(AVR_WRITER_OPT) $?

clean:
	rm *.elf *.hex

