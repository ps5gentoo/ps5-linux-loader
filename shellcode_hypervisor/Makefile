ifndef PS5_PAYLOAD_SDK
    PS5_PAYLOAD_SDK = /opt/ps5-payload-sdk/
endif

# 1. Variables
ifeq ($(shell uname -m),aarch64)
CC      = x86_64-linux-gnu-gcc
LD      = x86_64-linux-gnu-ld
OBJCOPY = x86_64-linux-gnu-objcopy
else
CC      = gcc
LD      = ld
OBJCOPY = objcopy
endif
CFLAGS  = -O2 -fno-stack-protector -ffreestanding -nostdlib -fcf-protection=none -m64 -I$(PS5_PAYLOAD_SDK)/target/include
LDFLAGS = -T linker.ld
TARGET  = shellcode_hypervisor.elf
TEXT_BIN = shellcode_hypervisor.bin
dump = shellcode_hypervisor.h

SRC = main.c utils.c boot_linux.c
OBJ = $(SRC:.c=.o)

all: $(dump)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TEXT_BIN): $(TARGET)
	$(OBJCOPY) -O binary -j .shell_code $(TARGET) $(TEXT_BIN)

clean:
	rm -f $(OBJ) $(TARGET) $(TEXT_BIN) $(dump)

$(dump): $(TEXT_BIN)
	python3 bin_to_c_hypervisor.py $(TEXT_BIN)