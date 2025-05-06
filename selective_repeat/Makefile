CC=gcc
CFLAGS=-O2 -Wall

# Define the options for the datalink program
opt1=--port=10001 --utopia
opt2=--port=10002
opt3=--port=10003 --flood --utopia
opt4=--port=10004 --flood
opt5=--port=10005 --flood --ber=1e-4

# Define the test target
test: datalink
	# For each option, run the datalink program in a new screen session
	@$(foreach i,$(shell seq 1 5),\
		screen -dmS $(i)_datalinkA bash -c 'cd $(OUTPUT_DIR); timeout $(TEST_TIME) ./datalink -d3 a $(opt$(i)) --log=$(i)_a.log; exit';\
		screen -dmS $(i)_datalinkB bash -c 'cd $(OUTPUT_DIR); timeout $(TEST_TIME) ./datalink -d3 b $(opt$(i)) --log=$(i)_b.log; exit';\
	)
	# Sleep for TEST_TIME seconds
	sleep $(TEST_TIME)
	# Remove all object files
	${RM} *.o
	# Print the last line of each log file
	@$(foreach i,$(shell seq 1 5),\
		echo $$(tail -n 2 $(OUTPUT_DIR)/$(i)_a.log | head -n 1 | awk -F',' '{print $$3}' | awk '{print $$1}') $$(tail -n 2 $(OUTPUT_DIR)/$(i)_b.log | head -n 1 | awk -F',' '{print $$3}' | awk '{print $$1}');\
	)

# Define the datalink target
datalink: clean selective_repeat.o protocol.o lprintf.o crc32.o
	# Create the output directory if it does not exist
	mkdir $(OUTPUT_DIR) -p
	# Link the object files to create the datalink executable
	gcc selective_repeat.o protocol.o lprintf.o crc32.o -o $(OUTPUT_DIR)/datalink -lm

selective_repeat.o: selective_repeat.c
	$(CC) $(CFLAGS) -DSEQ_BITS=$(SEQ_BITS) -DDATA_TIMER=$(DATA_TIMER) -DACK_TIMER=$(ACK_TIMER) -c selective_repeat.c

# 清理目标文件
clean:
	${RM} *.o datalink *.log

# 设置部分

# Check if SEQ_BITS is not set
SEQ_BITS=
ifeq ($(SEQ_BITS),)
SEQ_BITS=4
endif

# Check if DATA_TIMER is not set
DATA_TIMER=
ifeq ($(DATA_TIMER),)
DATA_TIMER=1500
endif

# Check if ACK_TIMER is not set
ACK_TIMER=
ifeq ($(ACK_TIMER),)
ACK_TIMER=280
endif

# Define the output directory name based on the parameters
OUTPUT_DIR=TEST_$(SEQ_BITS)BITS_$(DATA_TIMER)DATA_$(ACK_TIMER)ACK

# Remove spaces from the output directory name
space=$(empty) $(empty)
OUTPUT_DIR:=$(subst $(space),,$(OUTPUT_DIR))

TEST_TIME=1200