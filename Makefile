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
opt1=--port=10001 --utopia --ttl=$(TEST_TIME)
opt2=--port=10002 --ttl=$(TEST_TIME)
opt3=--port=10003 --flood --utopia --ttl=$(TEST_TIME)
opt4=--port=10004 --flood --ttl=$(TEST_TIME)
opt5=--port=10005 --flood --ber=1e-4 --ttl=$(TEST_TIME)

# 定义目标文件
OBJS = protocol.o lprintf.o crc32.o $(PROTOCOL).o

# 定义测试结果目录
TEST_DIR = test_results/$(PROTOCOL)_$(SEQ_BITS)bits_$(DATA_TIMER)data_$(ACK_TIMER)ack_$(TEST_TIME)s

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

gobackN.o: Protocols/gobackN.c protocol.h datalink.h
	$(CC) $(CFLAGS) -DSEQ_BITS=$(SEQ_BITS) -DDATA_TIMER=$(DATA_TIMER) -DACK_TIMER=$(ACK_TIMER) -c Protocols/gobackN.c -o gobackN.o

protocol.o: protocol.c protocol.h
	$(CC) $(CFLAGS) -c protocol.c

lprintf.o: lprintf.c lprintf.h
	$(CC) $(CFLAGS) -c lprintf.c

crc32.o: crc32.c
	$(CC) $(CFLAGS) -c crc32.c

# 自动测试目标
test: datalink
	@echo "Starting test for protocol: $(PROTOCOL)"
	@echo "Using parameters: SEQ_BITS=$(SEQ_BITS), DATA_TIMER=$(DATA_TIMER), ACK_TIMER=$(ACK_TIMER), TEST_TIME=$(TEST_TIME)s"
	@mkdir -p $(TEST_DIR)
	@$(foreach i,$(shell seq 1 5),\
		echo "Running test case $(i)";\
		screen -dmS $(i)_datalinkA bash -c 'cd $(PWD); ./datalink a $(opt$(i)) --log=$(TEST_DIR)/$(i)_a.log; exit';\
		screen -dmS $(i)_datalinkB bash -c 'cd $(PWD); ./datalink b $(opt$(i)) --log=$(TEST_DIR)/$(i)_b.log; exit';\
	)
	@sleep $(shell echo $$(($(TEST_TIME) + 5)))
	@echo "Test completed. Results:"
	@echo "Test Case | Station | Packets | Rate(bps) | Efficiency | Error Rate"
	@echo "----------|---------|---------|-----------|------------|-----------"
	@$(foreach i,$(shell seq 1 5),\
		packets_a=$$(tail -n 2 $(TEST_DIR)/$(i)_a.log | head -n 1 | sed 's/^[0-9.]* .... \([0-9]\+\) packets.*/\1/');\
		rate_a=$$(tail -n 2 $(TEST_DIR)/$(i)_a.log | head -n 1 | sed 's/.* \([0-9]\+\) bps.*/\1/');\
		eff_a=$$(tail -n 2 $(TEST_DIR)/$(i)_a.log | head -n 1 | sed 's/.* \([0-9.]\+\)%.*/\1%/');\
		err_a=$$(tail -n 2 $(TEST_DIR)/$(i)_a.log | head -n 1 | sed 's/.*Err \([0-9]\+\) (\([0-9.e-]\+\)).*/\1 (\2)/;t;d');\
		if [ -z "$$err_a" ]; then err_a="0 (0.0e+00)"; fi;\
		packets_b=$$(tail -n 2 $(TEST_DIR)/$(i)_b.log | head -n 1 | sed 's/^[0-9.]* .... \([0-9]\+\) packets.*/\1/');\
		rate_b=$$(tail -n 2 $(TEST_DIR)/$(i)_b.log | head -n 1 | sed 's/.* \([0-9]\+\) bps.*/\1/');\
		eff_b=$$(tail -n 2 $(TEST_DIR)/$(i)_b.log | head -n 1 | sed 's/.* \([0-9.]\+\)%.*/\1%/');\
		err_b=$$(tail -n 2 $(TEST_DIR)/$(i)_b.log | head -n 1 | sed 's/.*Err \([0-9]\+\) (\([0-9.e-]\+\)).*/\1 (\2)/;t;d');\
		if [ -z "$$err_b" ]; then err_b="0 (0.0e+00)"; fi;\
		printf "%-9d | %-7s | %-7s | %-9s | %-10s | %-9s\n" $(i) "A" "$$packets_a" "$$rate_a" "$$eff_a" "$$err_a";\
		printf "%-9s | %-7s | %-7s | %-9s | %-10s | %-9s\n" "" "B" "$$packets_b" "$$rate_b" "$$eff_b" "$$err_b";\
	)
	@echo "Generating detailed results..."
	@echo "Protocol: $(PROTOCOL)" > $(TEST_DIR)/results.txt
	@echo "Parameters: SEQ_BITS=$(SEQ_BITS), DATA_TIMER=$(DATA_TIMER), ACK_TIMER=$(ACK_TIMER), TEST_TIME=$(TEST_TIME)s" >> $(TEST_DIR)/results.txt
	@echo "" >> $(TEST_DIR)/results.txt
	@echo "Test Case | Station | Packets | Rate(bps) | Efficiency | Error Rate" >> $(TEST_DIR)/results.txt
	@echo "----------|---------|---------|-----------|------------|-----------" >> $(TEST_DIR)/results.txt
	@$(foreach i,$(shell seq 1 5),\
		packets_a=$$(tail -n 2 $(TEST_DIR)/$(i)_a.log | head -n 1 | sed 's/^[0-9.]* .... \([0-9]\+\) packets.*/\1/');\
		rate_a=$$(tail -n 2 $(TEST_DIR)/$(i)_a.log | head -n 1 | sed 's/.* \([0-9]\+\) bps.*/\1/');\
		eff_a=$$(tail -n 2 $(TEST_DIR)/$(i)_a.log | head -n 1 | sed 's/.* \([0-9.]\+\)%.*/\1%/');\
		err_a=$$(tail -n 2 $(TEST_DIR)/$(i)_a.log | head -n 1 | sed 's/.*Err \([0-9]\+\) (\([0-9.e-]\+\)).*/\1 (\2)/;t;d');\
		if [ -z "$$err_a" ]; then err_a="0 (0.0e+00)"; fi;\
		packets_b=$$(tail -n 2 $(TEST_DIR)/$(i)_b.log | head -n 1 | sed 's/^[0-9.]* .... \([0-9]\+\) packets.*/\1/');\
		rate_b=$$(tail -n 2 $(TEST_DIR)/$(i)_b.log | head -n 1 | sed 's/.* \([0-9]\+\) bps.*/\1/');\
		eff_b=$$(tail -n 2 $(TEST_DIR)/$(i)_b.log | head -n 1 | sed 's/.* \([0-9.]\+\)%.*/\1%/');\
		err_b=$$(tail -n 2 $(TEST_DIR)/$(i)_b.log | head -n 1 | sed 's/.*Err \([0-9]\+\) (\([0-9.e-]\+\)).*/\1 (\2)/;t;d');\
		if [ -z "$$err_b" ]; then err_b="0 (0.0e+00)"; fi;\
		printf "%-9d | %-7s | %-7s | %-9s | %-10s | %-9s\n" $(i) "A" "$$packets_a" "$$rate_a" "$$eff_a" "$$err_a" >> $(TEST_DIR)/results.txt;\
		printf "%-9s | %-7s | %-7s | %-9s | %-10s | %-9s\n" "" "B" "$$packets_b" "$$rate_b" "$$eff_b" "$$err_b" >> $(TEST_DIR)/results.txt;\
	)
	@echo "Detailed results saved to '$(TEST_DIR)/results.txt'"

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
	@echo "Test cases:"
	@echo "  1. Utopia channel (error-free)"
	@echo "  2. Normal channel"
	@echo "  3. Utopia channel with flood traffic"
	@echo "  4. Flood traffic"
	@echo "  5. Flood traffic with BER=1e-4"
	@echo ""
	@echo "Example:"
	@echo "  make PROTOCOL=selective_repeat SEQ_BITS=4 DATA_TIMER=1500 ACK_TIMER=500 TEST_TIME=300 test"