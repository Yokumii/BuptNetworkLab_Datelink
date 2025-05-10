CC=gcc
CFLAGS=-O2 -Wall

# 为datalink程序定义选项
opt1=--port=10001 --utopia
opt2=--port=10002
opt3=--port=10003 --flood --utopia
opt4=--port=10004 --flood
opt5=--port=10005 --flood --ber=1e-4

TEST_TIME=300

# 定义编译目标
build: build_selective build_stopwait build_gobackn

build_selective: clean
	$(CC) $(CFLAGS) -c selective
	$(CC) $(CFLAGS) -c ../protocol.c
	$(CC) $(CFLAGS) -c ../lprintf.c
	$(CC) $(CFLAGS) -c ../crc32.c
	$(CC) selective.o protocol.o lprintf.o crc32.o -o datalink -lm

build_stopwait: clean
	$(CC) $(CFLAGS) -c stopwait
	$(CC) $(CFLAGS) -c ../protocol.c
	$(CC) $(CFLAGS) -c ../lprintf.c
	$(CC) $(CFLAGS) -c ../crc32.c
	$(CC) stopwait.o protocol.o lprintf.o crc32.o -o datalink -lm

build_gobackn: clean
	$(CC) $(CFLAGS) -c gobackn
	$(CC) $(CFLAGS) -c ../protocol.c
	$(CC) $(CFLAGS) -c ../lprintf.c
	$(CC) $(CFLAGS) -c ../crc32.c
	$(CC) gobackn.o protocol.o lprintf.o crc32.o -o datalink -lm

# 定义测试目标
test: test_selective test_stopwait test_gobackn

test_selective: clean
	mkdir -p ../Results/example/selective
	$(CC) $(CFLAGS) -c selective
	$(CC) $(CFLAGS) -c ../protocol.c
	$(CC) $(CFLAGS) -c ../lprintf.c
	$(CC) $(CFLAGS) -c ../crc32.c
	$(CC) selective.o protocol.o lprintf.o crc32.o -o ../Results/example/selective/datalink -lm
	@$(foreach i,$(shell seq 1 5),\
		screen -dmS $(i)_datalinkA bash -c 'cd ../Results/example/selective; timeout $(TEST_TIME) ./datalink a $(opt$(i)) --log=$(i)_a.log; exit';\
		screen -dmS $(i)_datalinkB bash -c 'cd ../Results/example/selective; timeout $(TEST_TIME) ./datalink b $(opt$(i)) --log=$(i)_b.log; exit';\
	)
	sleep $(TEST_TIME)
	${RM} *.o
	@$(foreach i,$(shell seq 1 5),\
		echo $$(tail -n 2 ../Results/example/selective/$(i)_a.log | head -n 1 | awk -F',' '{print $$3}' | awk '{print $$1}') $$(tail -n 2 ../Results/example/selective/$(i)_b.log | head -n 1 | awk -F',' '{print $$3}' | awk '{print $$1}');\
	)

test_stopwait: clean
	mkdir -p ../Results/example/stopwait
	$(CC) $(CFLAGS) -c stopwait
	$(CC) $(CFLAGS) -c ../protocol.c
	$(CC) $(CFLAGS) -c ../lprintf.c
	$(CC) $(CFLAGS) -c ../crc32.c
	$(CC) stopwait.o protocol.o lprintf.o crc32.o -o ../Results/example/stopwait/datalink -lm
	@$(foreach i,$(shell seq 1 5),\
		screen -dmS $(i)_datalinkA bash -c 'cd ../Results/example/stopwait; timeout $(TEST_TIME) ./datalink a $(opt$(i)) --log=$(i)_a.log; exit';\
		screen -dmS $(i)_datalinkB bash -c 'cd ../Results/example/stopwait; timeout $(TEST_TIME) ./datalink b $(opt$(i)) --log=$(i)_b.log; exit';\
	)
	sleep $(TEST_TIME)
	${RM} *.o
	@$(foreach i,$(shell seq 1 5),\
		echo $$(tail -n 2 ../Results/example/stopwait/$(i)_a.log | head -n 1 | awk -F',' '{print $$3}' | awk '{print $$1}') $$(tail -n 2 ../Results/example/stopwait/$(i)_b.log | head -n 1 | awk -F',' '{print $$3}' | awk '{print $$1}');\
	)

test_gobackn: clean
	mkdir -p ../Results/example/gobackn
	$(CC) $(CFLAGS) -c gobackn
	$(CC) $(CFLAGS) -c ../protocol.c
	$(CC) $(CFLAGS) -c ../lprintf.c
	$(CC) $(CFLAGS) -c ../crc32.c
	$(CC) gobackn.o protocol.o lprintf.o crc32.o -o ../Results/example/gobackn/datalink -lm
	@$(foreach i,$(shell seq 1 5),\
		screen -dmS $(i)_datalinkA bash -c 'cd ../Results/example/gobackn; timeout $(TEST_TIME) ./datalink a $(opt$(i)) --log=$(i)_a.log; exit';\
		screen -dmS $(i)_datalinkB bash -c 'cd ../Results/example/gobackn; timeout $(TEST_TIME) ./datalink b $(opt$(i)) --log=$(i)_b.log; exit';\
	)
	sleep $(TEST_TIME)
	${RM} *.o
	@$(foreach i,$(shell seq 1 5),\
		echo $$(tail -n 2 ../Results/example/gobackn/$(i)_a.log | head -n 1 | awk -F',' '{print $$3}' | awk '{print $$1}') $$(tail -n 2 ../Results/example/gobackn/$(i)_b.log | head -n 1 | awk -F',' '{print $$3}' | awk '{print $$1}');\
	)

# 清理目标文件
clean:
	${RM} *.o datalink 