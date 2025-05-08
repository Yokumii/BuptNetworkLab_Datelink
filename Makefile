CC=gcc
CFLAGS=-O2 -Wall

# 为datalink程序定义选项
opt1=--port=10001 --utopia
opt2=--port=10002
opt3=--port=10003 --flood --utopia
opt4=--port=10004 --flood
opt5=--port=10005 --flood --ber=1e-4

# 设置部分
SEQ_BITS=
ifeq ($(SEQ_BITS),)
SEQ_BITS=5
endif

DATA_TIMER=
ifeq ($(DATA_TIMER),)
DATA_TIMER=2500
endif

ACK_TIMER=
ifeq ($(ACK_TIMER),)
ACK_TIMER=300
endif

TEST_TIME=30

# 根据参数定义输出目录名
OUTPUT_DIR= Results/$(PROTOCOL)/TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK_$(TEST_TIME)S

# 删除输出目录名中的空格
space=$(empty) $(empty)
OUTPUT_DIR:=$(subst $(space),,$(OUTPUT_DIR))

# 定义测试目标
test: test_selectiverepeat test_stopwait test_gobackn

test_selectiverepeat: clean
	mkdir -p Results/selectiverepeat/TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK
	$(CC) $(CFLAGS) -DSEQ_BITS=$(SEQ_BITS) -DDATA_TIMER=$(DATA_TIMER) -DACK_TIMER=$(ACK_TIMER) -c Protocols/selectiverepeat.c
	$(CC) $(CFLAGS) -c protocol.c
	$(CC) $(CFLAGS) -c lprintf.c
	$(CC) $(CFLAGS) -c crc32.c
	$(CC) selectiverepeat.o protocol.o lprintf.o crc32.o -o Results/selectiverepeat/TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK/datalink -lm
	@$(foreach i,$(shell seq 1 5),\
		screen -dmS $(i)_datalinkA bash -c 'cd Results/selectiverepeat/TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK; timeout $(TEST_TIME) ./datalink a $(opt$(i)) --log=$(i)_a.log; exit';\
		screen -dmS $(i)_datalinkB bash -c 'cd Results/selectiverepeat/TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK; timeout $(TEST_TIME) ./datalink b $(opt$(i)) --log=$(i)_b.log; exit';\
	)
	sleep $(TEST_TIME)
	${RM} *.o
	@$(foreach i,$(shell seq 1 5),\
		echo $$(tail -n 2 Results/selectiverepeat/TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK/$(i)_a.log | head -n 1 | awk -F',' '{print $$3}' | awk '{print $$1}') $$(tail -n 2 Results/selectiverepeat/TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK/$(i)_b.log | head -n 1 | awk -F',' '{print $$3}' | awk '{print $$1}');\
	)

test_stopwait: clean
	mkdir -p Results/stopwait/TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK
	$(CC) $(CFLAGS) -DSEQ_BITS=$(SEQ_BITS) -DDATA_TIMER=$(DATA_TIMER) -DACK_TIMER=$(ACK_TIMER) -c Protocols/stopwait.c
	$(CC) $(CFLAGS) -c protocol.c
	$(CC) $(CFLAGS) -c lprintf.c
	$(CC) $(CFLAGS) -c crc32.c
	$(CC) stopwait.o protocol.o lprintf.o crc32.o -o Results/stopwait/TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK/datalink -lm
	@$(foreach i,$(shell seq 1 5),\
		screen -dmS $(i)_datalinkA bash -c 'cd Results/stopwait/TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK; timeout $(TEST_TIME) ./datalink a $(opt$(i)) --log=$(i)_a.log; exit';\
		screen -dmS $(i)_datalinkB bash -c 'cd Results/stopwait/TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK; timeout $(TEST_TIME) ./datalink b $(opt$(i)) --log=$(i)_b.log; exit';\
	)
	sleep $(TEST_TIME)
	${RM} *.o
	@$(foreach i,$(shell seq 1 5),\
		echo $$(tail -n 2 Results/stopwait/TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK/$(i)_a.log | head -n 1 | awk -F',' '{print $$3}' | awk '{print $$1}') $$(tail -n 2 Results/stopwait/TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK/$(i)_b.log | head -n 1 | awk -F',' '{print $$3}' | awk '{print $$1}');\
	)

test_gobackn: clean
	mkdir -p Results/gobackn/TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK
	$(CC) $(CFLAGS) -DSEQ_BITS=$(SEQ_BITS) -DDATA_TIMER=$(DATA_TIMER) -DACK_TIMER=$(ACK_TIMER) -c Protocols/gobackn.c
	$(CC) $(CFLAGS) -c protocol.c
	$(CC) $(CFLAGS) -c lprintf.c
	$(CC) $(CFLAGS) -c crc32.c
	$(CC) gobackn.o protocol.o lprintf.o crc32.o -o Results/gobackn/TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK/datalink -lm
	@$(foreach i,$(shell seq 1 5),\
		screen -dmS $(i)_datalinkA bash -c 'cd Results/gobackn/TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK; timeout $(TEST_TIME) ./datalink a $(opt$(i)) --log=$(i)_a.log; exit';\
		screen -dmS $(i)_datalinkB bash -c 'cd Results/gobackn/TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK; timeout $(TEST_TIME) ./datalink b $(opt$(i)) --log=$(i)_b.log; exit';\
	)
	sleep $(TEST_TIME)
	${RM} *.o
	@$(foreach i,$(shell seq 1 5),\
		echo $$(tail -n 2 Results/gobackn/TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK/$(i)_a.log | head -n 1 | awk -F',' '{print $$3}' | awk '{print $$1}') $$(tail -n 2 Results/gobackn/TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK/$(i)_b.log | head -n 1 | awk -F',' '{print $$3}' | awk '{print $$1}');\
	)

# 清理目标文件
clean:
	${RM} *.o datalink