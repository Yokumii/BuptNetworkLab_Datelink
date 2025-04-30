#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "protocol.h"
#include "datalink.h"
#define ACK_TIMER  1000
#define DATA_TIMER 2000

struct FRAME {
	unsigned char kind; /* FRAME_DATA */
	unsigned char ack;
	unsigned int seq;
	unsigned char data[PKT_LEN];
	unsigned int padding;
};


static unsigned int frame_expected = 0;
static unsigned int sequence_number = 7;//0~7序列空间，由此可以推出，W最大是3
#define send_window_size 7
static int sending_window[send_window_size][2] = {0};//发送窗口，二维数组，索引0用于储存是否收到ack，索引1用于标记当前的seq
static unsigned char buffer[PKT_LEN*send_window_size];
//static unsigned int nbuffered;
static int phl_ready = 0;

static int power_2(int n) {
    int result = 1;
    for (int i = 0; i < n; i++) {
        result *= 2;
    }
    return result;
}
static void init_send_window(int** window){
	for(int i = 0; i < send_window_size; i++){
		sending_window[i][0] = 0;
		sending_window[i][1] = i;
	}
}
static void window_shift(int window[send_window_size][2]){ 
	//先获取需要偏移多少位
	int shift = 0;
	for(int i = 0; i < send_window_size; i++){
		if(window[i][0] != 1){
			shift = i;
			break;
		}

	}
	for(int i = 0; i < send_window_size; i++){
		if(window[i][0] == 1){
			window[i][0] = 0;
			window[i][1] = (window[i][1]+shift)%send_window_size;
		}

	}
}
static void put_frame(unsigned char *frame, int len)
{
	*(unsigned int *)(frame + len) = crc32(frame, len);
	send_frame(frame, len + 4);
	phl_ready = 0;
}
static bool isin(int ack, int** window){
	for(int i = 0; i < send_window_size; i++){
		if(window[i][1] == ack){
			return true;
		}
	}
	return false;
}
static void send_data_frame(int i)// 发送i个帧
{
	for(int j = 0;j<i;j++){
		struct FRAME s;

		s.kind = FRAME_DATA;
		s.seq = sending_window[j][1];//当前的seqnumber
		s.ack = frame_expected-1==-1?send_window_size-1:frame_expected-1;
		memcpy(s.data, buffer + j * PKT_LEN, PKT_LEN);
	
		dbg_frame("Send DATA %d %d, ID %d\n", s.seq, s.ack, *(short *)s.data);
	
		put_frame((unsigned char *)&s, 3 + PKT_LEN);
		start_timer(sending_window[j][1], DATA_TIMER);
	}
	stop_ack_timer();
}

static void send_ack_frame(void)
{
	struct FRAME s;

	s.kind = FRAME_ACK;
	s.ack = frame_expected-1==-1?send_window_size-1:frame_expected-1;

	dbg_frame("Send ACK  %d\n", s.ack);

	put_frame((unsigned char *)&s, 2);
}

int main(int argc, char **argv)
{
	int event, arg;
	struct FRAME f;
	int len = 0;
	init_send_window(sending_window);
	protocol_init(argc, argv);
	lprintf("Designed by TianyiWang, build: " __DATE__ "  "__TIME__"\n");

	disable_network_layer();// 网络层不启用

	for (;;) {//等价于while 1
		event = wait_for_event(&arg);

		switch (event) {
		case NETWORK_LAYER_READY://对于网络层已经准备好的改动，那么就是要得到很多很多的packet，填满window_size
			int i = 0;
			while(sending_window[i][0] == 0){//遇到等于1的，就不要包了，然后重发
				get_packet(buffer+i*PKT_LEN);
				i++;
			}
			send_data_frame(i);
			break;

		case PHYSICAL_LAYER_READY:
			phl_ready = 1;
			break;

		case FRAME_RECEIVED:
			len = recv_frame((unsigned char *)&f, sizeof f);
			if (len < 5 || crc32((unsigned char *)&f, len) != 0) {
				dbg_event("**** Receiver Error, Bad CRC Checksum\n");
				break;//发生错误，直接丢掉这个包（不操作），等待出发超时重传，或者可以返回NAK加速重传，都可以
			}
			if (f.kind == FRAME_ACK)
				dbg_frame("Recv ACK  %d\n", f.ack);
			if (f.kind == FRAME_DATA) {
				dbg_frame("Recv DATA %d %d, ID %d\n", f.seq,
					  f.ack, *(short *)f.data);//打印日志
				if (f.seq == frame_expected) {
					put_packet(f.data, len - 7);
					frame_expected = (frame_expected + 1)%send_window_size;//frame_expected改为发送窗口内
				}
				start_ack_timer(ACK_TIMER);

			}
			if (isin(f.ack,sending_window)) {//如果f.ack在接受窗口内,但是这个情况没有ack_timer，后续再添加
				//停止计时器
				stop_timer(sending_window[f.ack][1]);
				//窗口滑动
				sending_window[f.ack][0]=1;
				window_shift(sending_window);
				//nbuffered--;
			}
			break;

		case DATA_TIMEOUT:
			dbg_event("---- DATA %d timeout\n", arg);
			int q = 0;
			while(sending_window[q][0] == 0){//遇到等于1的，就不要包了，然后重发
				q++;
			}
			send_data_frame(q);
			break;
		case ACK_TIMEOUT:
			send_ack_frame();
			break;
		}

		if (phl_ready)//这里后续还会改
			enable_network_layer();
		else
			disable_network_layer();
	}
}