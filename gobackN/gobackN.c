#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "protocol.h"
#include "datalink.h"

#define ACK_TIMER   1000
#define DATA_TIMER  2000
#define MAX_SEQ     7       // 序列号0-7
#define N           4       // 窗口大小

struct FRAME {
    unsigned char kind;
    unsigned char ack;
    unsigned char seq;
    unsigned char data[PKT_LEN];
    unsigned int padding;
};

//========= 发送方状态 =========//
static unsigned char buffer[PKT_LEN * N];  // 环形缓冲区
static unsigned int send_base = 0;         // 窗口起始
static unsigned int next_seq = 0;          // 下一个发送序号
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

static void send_data_frame(bool resend)//这里的resend参数表示是否为重传
{
    if (!resend) {
        // 发送新帧
        while (1) {
            int window_used = (next_seq - send_base + MAX_SEQ + 1) % (MAX_SEQ + 1);
            if (window_used >= N || !phl_ready) break;// 窗口已满或物理层未就绪，退出

            // 只有当网络层就绪时才获取包
            if (!get_packet(buffer + (next_seq % N) * PKT_LEN)) {
                dbg_event("No more packets, stop sending\n");
                break;
            }

            struct FRAME s;
            s.kind = FRAME_DATA;
            s.seq = next_seq;
            s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);
            memcpy(s.data, buffer + (next_seq % N) * PKT_LEN, PKT_LEN);

            dbg_frame("Send DATA %d ack%d, ID %d\n", s.seq, s.ack, *(short *)s.data);

            put_frame((unsigned char *)&s, 3 + PKT_LEN);
            start_timer(next_seq, DATA_TIMER);
            next_seq = (next_seq + 1) % (MAX_SEQ + 1);
        }
    } else {
        // 重传全部未确认帧
        dbg_event("Resend window [%d-%d]\n", send_base, (next_seq + MAX_SEQ) % (MAX_SEQ + 1));
        unsigned int seq = send_base;
        while (seq != next_seq) {
            struct FRAME s;
            
            s.kind = FRAME_DATA;
            s.seq = seq;
            s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);
            memcpy(s.data, buffer + (seq % N) * PKT_LEN, PKT_LEN);

            dbg_frame("Resend DATA %d ack%d, ID %d\n", s.seq, s.ack, *(short *)s.data);

            put_frame((unsigned char *)&s, 3 + PKT_LEN);
            start_timer(seq, DATA_TIMER);
            
            seq = (seq + 1) % (MAX_SEQ + 1);
        }
    }
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
    lprintf("Go-Back-N Protocol (MAX_SEQ=%d, N=%d)\n", MAX_SEQ, N);
    
    // 初始状态控制
    disable_network_layer();
    phl_ready = 0;

    for (;;) {
        // 网络层控制：窗口未满且物理层就绪时启用
        int window_used = (next_seq - send_base + MAX_SEQ + 1) % (MAX_SEQ + 1);
        if (phl_ready && window_used < N) {
            enable_network_layer();
        } else {
            disable_network_layer();
        }

        event = wait_for_event(&arg);

        switch (event) {
        case NETWORK_LAYER_READY://只要网络层就绪，那么就尝试发送新帧
            send_data_frame(false);  // 尝试发送新帧
            break;

        case PHYSICAL_LAYER_READY:
            phl_ready = 1;
            break;

        case FRAME_RECEIVED:
            len = recv_frame((unsigned char *)&f, sizeof(f));
            
            if (len < 5 || crc32((unsigned char *)&f, len) != 0) {
                dbg_event("**** Bad CRC, frame dropped\n");
                break;
            }

            if (f.kind == FRAME_ACK) {
                // 处理累积ACK
                unsigned int ack = f.ack;
                dbg_frame("Recv ACK  %d\n", ack);

                if ((send_base <= ack && ack < next_seq) ||
                    (next_seq < send_base && ack < next_seq)) 
                {
                    while (send_base != (ack + 1) % (MAX_SEQ + 1)) {
                        stop_timer(send_base);
                        send_base = (send_base + 1) % (MAX_SEQ + 1);
                    }
                }
            } else if (f.kind == FRAME_DATA) {
                dbg_frame("Recv DATA %d ack%d, ID %d\n",
                          f.seq, f.ack, *(short *)f.data);

                if (f.seq == frame_expected) {
                    put_packet(f.data, len - 7);
                    frame_expected = (frame_expected + 1) % (MAX_SEQ + 1);
                }
                send_ack_frame();  // 总是回复ACK
            }
            break;

        case DATA_TIMEOUT:
            dbg_event("Timeout SEQ %d\n", arg);
            send_data_frame(true);  // 触发重传
            break;

        case ACK_TIMEOUT:
            send_ack_frame();
            break;
        }
    }
}