CC=gcc
CFLAGS=-O2 -Wall

# 定义协议类型
PROTOCOL ?= stop_wait

# 定义协议参数
SEQ_BITS ?= 6
DATA_TIMER ?= 2000
ACK_TIMER ?= 666

# 定义测试时间（秒）
TEST_TIME ?= 30

# 定义测试选项
opt1=--port=10001 --utopia
opt2=--port=10002
opt3=--port=10003 --flood --utopia
opt4=--port=10004 --flood
opt5=--port=10005 --flood --ber=1e-4

# 定义目标文件
OBJS = protocol.o lprintf.o crc32.o $(PROTOCOL).o

# 定义测试结果目录
TEST_DIR = test_results/$(PROTOCOL)_$(SEQ_BITS)bits_$(DATA_TIMER)data_$(ACK_TIMER)ack

# 默认目标
all: datalink

# 编译可执行文件
datalink: $(OBJS)
	$(CC) $(OBJS) -o datalink -lm

# 编译各个源文件
stop_wait.o: Protocols/stop_wait.c protocol.h datalink.h
	$(CC) $(CFLAGS) -DSEQ_BITS=$(SEQ_BITS) -DDATA_TIMER=$(DATA_TIMER) -DACK_TIMER=$(ACK_TIMER) -c Protocols/stop_wait.c -o stop_wait.o

selective_repeat.o: Protocols/selective_repeat.c protocol.h datalink.h
	$(CC) $(CFLAGS) -DSEQ_BITS=$(SEQ_BITS) -DDATA_TIMER=$(DATA_TIMER) -DACK_TIMER=$(ACK_TIMER) -c Protocols/selective_repeat.c -o selective_repeat.o

protocol.o: protocol.c protocol.h
	$(CC) $(CFLAGS) -c protocol.c

lprintf.o: lprintf.c lprintf.h
	$(CC) $(CFLAGS) -c lprintf.c

crc32.o: crc32.c
	$(CC) $(CFLAGS) -c crc32.c

# 自动测试目标
test: datalink
	@echo "Starting test for protocol: $(PROTOCOL)"
	@echo "Using parameters: SEQ_BITS=$(SEQ_BITS), DATA_TIMER=$(DATA_TIMER), ACK_TIMER=$(ACK_TIMER)"
	@mkdir -p $(TEST_DIR)
	@$(foreach i,$(shell seq 1 5),\
		echo "Running test case $(i)";\
		screen -L -Logfile $(TEST_DIR)/$(i)_a.log -dmS $(i)_datalinkA bash -c 'cd $(PWD); timeout $(TEST_TIME) ./datalink a $(opt$(i)) --log=$(TEST_DIR)/$(i)_a.log; exit';\
		screen -L -Logfile $(TEST_DIR)/$(i)_b.log -dmS $(i)_datalinkB bash -c 'cd $(PWD); timeout $(TEST_TIME) ./datalink b $(opt$(i)) --log=$(TEST_DIR)/$(i)_b.log; exit';\
	)
	@sleep $(TEST_TIME)
	@echo "Test completed. Results:"
	@echo "Test Case | Station A | Station B"
	@echo "----------|-----------|-----------"
	@$(foreach i,$(shell seq 1 5),\
		printf "%-9d | %-9s | %-9s\n" $(i) \
		$$(tail -n 2 $(TEST_DIR)/$(i)_a.log | head -n 1 | awk -F',' '{print $$3}' | awk '{print $$1}') \
		$$(tail -n 2 $(TEST_DIR)/$(i)_b.log | head -n 1 | awk -F',' '{print $$3}' | awk '{print $$1}');\
	)
	@echo "Test results are saved in '$(TEST_DIR)'"

# 清理编译生成的文件
clean:
	${RM} *.o datalink

# 清理测试结果
clean_test:
	${RM} -rf test_results

# 帮助信息
help:
	@echo "Usage: make [PROTOCOL=protocol_name] [SEQ_BITS=bits] [DATA_TIMER=ms] [ACK_TIMER=ms] [TEST_TIME=seconds]"
	@echo "Available protocols:"
	@echo "  stop_wait        - Stop and Wait protocol (default)"
	@echo "  selective_repeat - Selective Repeat protocol"
	@echo ""
	@echo "Protocol parameters:"
	@echo "  SEQ_BITS        - Number of bits for sequence numbers (default: 6)"
	@echo "  DATA_TIMER      - Data frame timeout in milliseconds (default: 2000)"
	@echo "  ACK_TIMER       - ACK frame timeout in milliseconds (default: 666)"
	@echo ""
	@echo "Test options:"
	@echo "  TEST_TIME       - Test duration in seconds (default: 30)"
	@echo ""
	@echo "Example:"
	@echo "  make PROTOCOL=selective_repeat SEQ_BITS=4 DATA_TIMER=1500 ACK_TIMER=500 TEST_TIME=300 test"