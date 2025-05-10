#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../protocol.h"
#include "../datalink.h"

typedef unsigned char seq_nr;
typedef unsigned char frame_kind;

struct FRAME {
    unsigned char kind;   /* FRAME_DATA */
    unsigned char ack;
    unsigned char seq;
    unsigned char data[PKT_LEN];
    unsigned int padding;
};

struct BUFFER {
    unsigned char data[PKT_LEN];
    size_t len;
};

/* 发送窗口相关变量 */
static struct BUFFER out_buf[NR_BUFS];   /* 发送方缓存区 */
static unsigned char nbuffered = 0;   /* 发送缓存区被使用个数 */

/* 接收窗口相关变量 */
static struct BUFFER in_buf[NR_BUFS];   /* 接收方缓存区 */
static bool arrived[NR_BUFS];   /* 接收方缓存区状态 */

static seq_nr i = 0;   /* 缓存区索引 */
static int phl_ready = 0;
static bool no_nak = true;

// 判断是否处于在窗口中
static bool between(seq_nr a, seq_nr b, seq_nr c)
{
    return a <= b && b < c || c < a && a <= b || b < c && c < a;
}

// 添加校验段，发送DATA帧到物理层
static void put_frame(unsigned char *frame, int len)
{
    *(unsigned int *)(frame + len) = crc32(frame, len);
    send_frame(frame, len + 4);
    phl_ready = 0;
}

// 根据不同类型构建帧、发送帧、处理计时器
static void send_frame_datalink(frame_kind fk, seq_nr frame_nr, seq_nr frame_expected)
{
    struct FRAME s;
    s.kind = fk;
    s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);
    if (fk == FRAME_DATA)
    {
        s.seq = frame_nr;
        memcpy(s.data, out_buf[frame_nr % NR_BUFS].data, out_buf[frame_nr % NR_BUFS].len);
        dbg_frame("Send DATA %d %d, ID %d\n", s.seq, s.ack, *(short *)s.data);
        put_frame((unsigned char *)&s, 3 + out_buf[frame_nr % NR_BUFS].len);
        start_timer(frame_nr % NR_BUFS, DATA_TIMER);
    }
    else
    {
        if (fk == FRAME_NAK)
        {
            s.ack = frame_expected;
            dbg_frame("Send NAK %d\n", frame_expected);
        }
        else
            dbg_frame("Send ACK %d\n", s.ack);
        send_frame((unsigned char *)&s, 3);
    }
    stop_ack_timer();
}

int main(int argc, char **argv)
{
    static seq_nr ack_expected = 0;   /* 发送窗口下界 */
    static seq_nr frame_nr = 0;   /* next_frame_to_send，即发送窗口上界 + 1 */
    static seq_nr frame_expected = 0;   /* 接收窗口的下界 */
    static seq_nr too_far = NR_BUFS;   /* 接收窗口的上界 + 1 */

    int event, arg;
    struct FRAME f;
    int len = 0;

    /* 初始化接收窗口状态，并没有被缓存的帧 */
    for (i = 0; i < NR_BUFS; i++) {
        arrived[i] = false;
    }

    protocol_init(argc, argv);
    lprintf("Datalink using Selective-Repeat protocol designed by Zhang Keming, "
            "build: " __DATE__ "  " __TIME__ "\n");

    disable_network_layer();

    while (true)
    {
        event = wait_for_event(&arg);

        switch (event)
        {
        case NETWORK_LAYER_READY: // 网络层就绪
            nbuffered++;
            out_buf[frame_nr % NR_BUFS].len = get_packet(out_buf[frame_nr % NR_BUFS].data);
            send_frame_datalink(FRAME_DATA, frame_nr, frame_expected); // 发送DATA帧
            inc(frame_nr);
            break;

        case PHYSICAL_LAYER_READY:
            phl_ready = 1;
            break;

        case FRAME_RECEIVED:
            len = recv_frame((unsigned char *)&f, sizeof f);
            if (len < 5 && len != 3)
                break;
            if (len >= 5 && crc32((unsigned char *)&f, len) != 0) // 校验错误
            {
                dbg_event("**** Receiver Error, Bad CRC Checksum ****\n");
                send_frame_datalink(FRAME_NAK, 0, f.seq);
                break;
            }

            if (f.kind == FRAME_ACK)
                dbg_frame("Recv ACK %d\n", f.ack);

            if (f.kind == FRAME_DATA)
            {
                dbg_frame("Recv DATA %d %d, ID %d\n", f.seq, f.ack, *(short *)f.data);
                start_ack_timer(ACK_TIMER);
                if (between(frame_expected, f.seq, too_far) && !arrived[f.seq % NR_BUFS])
                {
                    arrived[f.seq % NR_BUFS] = true;
                    memcpy(in_buf[f.seq % NR_BUFS].data, f.data, len - 7);
                    in_buf[f.seq % NR_BUFS].len = len - 7;
                    while (arrived[frame_expected % NR_BUFS]) // 将接收缓冲区中连续的已收到的帧发送到网络层
                    {
                        put_packet(in_buf[frame_expected % NR_BUFS].data, in_buf[frame_expected % NR_BUFS].len);
                        arrived[frame_expected % NR_BUFS] = false;
                        inc(frame_expected);
                        inc(too_far);
                        start_ack_timer(ACK_TIMER);
                    }
                }
            }

            if (f.kind == FRAME_NAK && between(ack_expected, f.ack, frame_nr)) // 发送特定的NAK帧
            {
                dbg_frame("Recv NAK %d\n", f.ack);
                send_frame_datalink(FRAME_DATA, f.ack, frame_expected);
                break;
            }

            while (between(ack_expected, f.ack, frame_nr)) // 累计确认
            {
                nbuffered--;
                stop_timer(ack_expected % NR_BUFS);
                inc(ack_expected);
            }
            break;

        case DATA_TIMEOUT:
            dbg_event("---- DATA %d timeout ----\n", arg);
            if (!between(ack_expected, arg, frame_nr)) // 计时器中的序号只有一半，要将序号转化到窗口区间内
                arg += NR_BUFS;
            send_frame_datalink(FRAME_DATA, arg, frame_expected);
            break;

        case ACK_TIMEOUT:
            dbg_event("---- ACK %d timeout ----\n", frame_expected);
            send_frame_datalink(FRAME_ACK, 0, frame_expected);
            break;
        }

        if (nbuffered < NR_BUFS && phl_ready)
            enable_network_layer();
        else
            disable_network_layer();
    }
}