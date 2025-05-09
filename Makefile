CC=gcc
CFLAGS=-O2 -Wall

# 为datalink程序定义选项
opt1=--port=10001 --utopia
opt2=--port=10002
opt3=--port=10003 --flood --utopia
opt4=--port=10004 --flood
opt5=--port=10005 --flood --ber=1e-4

# 定义测试目标
test: datalink
	# 对每个选项，在新的screen会话中运行datalink程序
	@$(foreach i,$(shell seq 1 5),\
		screen -dmS $(i)_datalinkA bash -c 'cd $(OUTPUT_DIR); timeout $(TEST_TIME) ./datalink a $(opt$(i)) --log=$(i)_a.log; exit';\
		screen -dmS $(i)_datalinkB bash -c 'cd $(OUTPUT_DIR); timeout $(TEST_TIME) ./datalink b $(opt$(i)) --log=$(i)_b.log; exit';\
	)
	# 休眠TEST_TIME秒
	sleep $(TEST_TIME)
	# 删除所有目标文件
	${RM} *.o
	# 打印每个日志文件的最后一行
	@$(foreach i,$(shell seq 1 5),\
		echo $$(tail -n 2 $(OUTPUT_DIR)/$(i)_a.log | head -n 1 | awk -F',' '{print $$3}' | awk '{print $$1}') $$(tail -n 2 $(OUTPUT_DIR)/$(i)_b.log | head -n 1 | awk -F',' '{print $$3}' | awk '{print $$1}');\
	)

gobackn: gobackn.o protocol.o lprintf.o crc32.o
	$(CC) gobackn.o protocol.o lprintf.o crc32.o -o gobackn -lm

gobackn.o: Protocols/gobackN.c datalink.h
	$(CC) $(CFLAGS) -c Protocols/gobackN.c -o gobackn.o

selective: selectiverepeat.o protocol.o lprintf.o crc32.o
	$(CC) selectiverepeat.o protocol.o lprintf.o crc32.o -o selective -lm

selectiverepeat.o: Protocols/selectiverepeat.c datalink.h
	$(CC) $(CFLAGS) -DSEQ_BITS=$(SEQ_BITS) -DDATA_TIMER=$(DATA_TIMER) -DACK_TIMER=$(ACK_TIMER) -c Protocols/selectiverepeat.c -o selectiverepeat.o

# 清理目标文件
clean:
	${RM} *.o

# 设置部分

# 检查SEQ_BITS是否设置
SEQ_BITS=
ifeq ($(SEQ_BITS),)
SEQ_BITS=5
endif

# 检查DATA_TIMER是否设置
DATA_TIMER=
ifeq ($(DATA_TIMER),)
DATA_TIMER=2500
endif

# 检查ACK_TIMER是否设置
ACK_TIMER=
ifeq ($(ACK_TIMER),)
ACK_TIMER=300
endif

# 根据参数定义输出目录名
OUTPUT_DIR= Results/TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK

# 删除输出目录名中的空格
space=$(empty) $(empty)
OUTPUT_DIR:=$(subst $(space),,$(OUTPUT_DIR))

TEST_TIME=30