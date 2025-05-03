CC=gcc
CFLAGS=-O2 -Wall

# 定义协议类型
PROTOCOL ?= stop_wait

# 定义目标文件
OBJS = protocol.o lprintf.o crc32.o $(PROTOCOL).o

# 默认目标
all: datalink

# 编译可执行文件
datalink: $(OBJS)
	$(CC) $(OBJS) -o datalink -lm

# 编译各个源文件
stop_wait.o: Protocols/stop_wait.c protocol.h datalink.h
	$(CC) $(CFLAGS) -c Protocols/stop_wait.c -o stop_wait.o

selective_repeat.o: Protocols/selective_repeat.c protocol.h datalink.h
	$(CC) $(CFLAGS) -c Protocols/selective_repeat.c -o selective_repeat.o

protocol.o: protocol.c protocol.h
	$(CC) $(CFLAGS) -c protocol.c

lprintf.o: lprintf.c lprintf.h
	$(CC) $(CFLAGS) -c lprintf.c

crc32.o: crc32.c
	$(CC) $(CFLAGS) -c crc32.c

# 清理编译生成的文件
clean:
	${RM} *.o datalink *.log

# 帮助信息
help:
	@echo "Usage: make [PROTOCOL=protocol_name]"
	@echo "Available protocols:"
	@echo "  stop_wait        - Stop and Wait protocol (default)"
	@echo "  selective_repeat - Selective Repeat protocol"