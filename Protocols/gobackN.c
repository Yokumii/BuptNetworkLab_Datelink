#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "../protocol.h"
#include "../datalink.h"


struct FRAME {
    unsigned char kind;
    unsigned char ack;
    unsigned char seq;
    unsigned char data[PKT_LEN];
    unsigned int padding;
};
struct BUFFER {
    unsigned char data[PKT_LEN];
};


//========= 发送方状态 =========//
// 缓冲区大小需匹配序列号空间
static struct BUFFER buffer[NR_BUFS];   /* 发送方缓存区 */
static unsigned int send_base = 0;         // 窗口起始，即最早的未确认的帧,等价next_frame_to_send
static unsigned int ack_expected = 0;
static unsigned int nbuffered = 0;
bool no_nak = true;
static int phl_ready = 0;

//========= 接收方状态 =========//

static unsigned int frame_expected = 0;

//========= 协议核心函数 =========//
static void put_frame(unsigned char *frame, int len)
{
    *(unsigned int *)(frame + len) = crc32(frame, len);
    send_frame(frame, len + 4);
    phl_ready = 0;
}

static bool between(int a, int b, int c) {
    return ((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a));
}

static void send_data_frame()//这里的resend参数表示是否为重传
{
    struct FRAME s;
    s.kind = FRAME_DATA;
    s.seq = send_base;
    s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);
    memcpy(s.data, &buffer[send_base % NR_BUFS], PKT_LEN);

    dbg_frame("Send DATA %d ack%d, ID %d\n", s.seq, s.ack, *(short *)s.data);
    put_frame((unsigned char *)&s, 3 + PKT_LEN);

    start_timer(send_base, DATA_TIMER);
}
static void send_nak_frame(void)
{
    struct FRAME s;
    s.kind = FRAME_NAK;
    s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);
    no_nak = false;
    dbg_frame("Send NAK  %d\n", s.ack);
    put_frame((unsigned char*)&s, 2);
    stop_ack_timer();
}

static void send_ack_frame(void)
{
    struct FRAME s;
    s.kind = FRAME_ACK;
    s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);
    
    dbg_frame("Send ACK  %d\n", s.ack);
    put_frame((unsigned char *)&s, 2);
}

//========= 主协议循环 =========//
int main(int argc, char **argv)
{
    int event, arg;
    struct FRAME f;
    int len;

    protocol_init(argc, argv);
    lprintf("Go-Back-N Protocol (MAX_SEQ=%d, N=%d)\n", MAX_SEQ, NR_BUFS);
    lprintf("designed by Wang tianyi");
    // 初始状态控制
    enable_network_layer();
    phl_ready = 0;

    for (;;) {
        event = wait_for_event(&arg);

        switch (event) {
        case NETWORK_LAYER_READY://只要网络层就绪，那么就尝试发送新帧
            get_packet((unsigned char*)&buffer[send_base % NR_BUFS]);
            dbg_event("Buffer DATA %d, ID %d at buffer[%d]\n", send_base, *(short*)buffer[send_base % NR_BUFS].data, send_base % NR_BUFS);
            nbuffered++;//拓宽window
            send_data_frame();
            stop_ack_timer();
            inc(send_base);

            break;

        case PHYSICAL_LAYER_READY:
            phl_ready = 1;
            break;

        case FRAME_RECEIVED:
            len = recv_frame((unsigned char *)&f, sizeof(f));
            
            if (len < 5 || crc32((unsigned char *)&f, len) != 0) {
                dbg_event("**** Bad CRC, frame dropped\n");
                if (no_nak) send_nak_frame();
                break;
            }

            if (f.kind == FRAME_ACK) {
                // 处理累积ACK
                dbg_frame("Recv ACK  %d\n", f.ack);
				//这里默认窗口大小和序号空间没有歧义，需要提前定义好MAX_SEQ和N来防止出错
            } else if (f.kind == FRAME_DATA) {
                dbg_frame("Recv DATA %d ack%d, ID %d\n",
                          f.seq, f.ack, *(short *)f.data);
                if (f.seq == frame_expected) {
                    put_packet(f.data, len - 7);
                    no_nak = true;
                    inc(frame_expected);
                    start_ack_timer(ACK_TIMER);
                }
            } else if(f.kind == FRAME_NAK) {
                dbg_frame("Recv NAK  %d\n", f.ack);
                send_base = ack_expected;
                for (int i = 0; i < nbuffered; i++) {
                    send_data_frame();
                    inc(send_base);
                }
            }
            while (between(ack_expected, f.ack, send_base)) //处理piggybacking
            {
                nbuffered = nbuffered - 1;
                stop_timer(ack_expected);
                inc(ack_expected);
            }
            break;

        case DATA_TIMEOUT:
            dbg_event("Timeout SEQ %d\n", arg);
            send_base = ack_expected;
            for (int i = 0; i < nbuffered; i++) {
                send_data_frame();
                inc(send_base);
            }
            break;

        case ACK_TIMEOUT:
            send_ack_frame();
            break;
        }
        // 网络层控制：窗口未满且物理层就绪时启用
        if (nbuffered<NR_BUFS&&phl_ready) {
            enable_network_layer();
        }
        else {
            disable_network_layer();
        }

    }

}
