#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../datalink.h"
#include "../protocol.h"

typedef unsigned char seq_nr;
typedef unsigned char frame_type;

struct FRAME {
    unsigned char kind;   /* FRAME_DATA */
    unsigned char ack;
    unsigned char seq;
    unsigned char data[PKT_LEN];
    unsigned int padding;
};

struct BUFFER {
    unsigned char data[PKT_LEN];
};

/* 发送窗口相关变量 */
static seq_nr oldest_frame = MAX_SEQ + 1;   /* 发送窗口中最早发送但还未被确认的帧的序列号，初始化为无效的序列号，表示初始状态下没有未确认的帧 */
static seq_nr ack_expected = 0;   /* 发送窗口下界 */
static seq_nr frame_nr = 0;   /* next_frame_to_send，即发送窗口上界 + 1 */
static struct BUFFER out_buf[NR_BUFS];   /* 发送方缓存区 */
static unsigned char nbuffered = 0;   /* 发送缓存区被使用个数 */

/* 接收窗口相关变量 */
static seq_nr frame_expected = 0;   /* 接收窗口的下界 */
static seq_nr too_far = NR_BUFS;   /* 接收窗口的上界 + 1 */
static struct BUFFER in_buf[NR_BUFS];   /* 接收方缓存区 */
static bool arrived[NR_BUFS];   /* 接收方缓存区状态 */

static seq_nr i = 0;   /* 缓存区索引 */
static int phl_ready = 0;
static bool no_nak = true;

/* 数据链路层向物理层发送帧 */
static void put_frame(unsigned char *frame, int len) {
    *(unsigned int *)(frame + len) = crc32(frame, len); /* 将校验位放在帧之后 */
    send_frame(frame, len + 4); /* 将组合后的帧发送给物理层 */
    phl_ready = 0;
}

/* 发送数据帧 */
static void send_data_frame(void) {
    struct FRAME s;

    s.kind = FRAME_DATA; /* 数据帧 */
    s.seq = frame_nr;    /* 帧序号 */
    s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1); /* 此时，站点虽然作为发送方，但由于是全双工通信，站在接收方的角度，它的接收窗口的下界是frame_expected，所以它已经正确接收到了所以 < frame_expected 的帧，所以捎带 ack 应该是frame_expected - 1，取模是因为要处理回绕 */

    memcpy(s.data, &out_buf[frame_nr % NR_BUFS], PKT_LEN);

    dbg_frame("Send DATA %d with ACK %d, ID %d\n", s.seq, s.ack, *(short *)s.data);

    put_frame((unsigned char *)&s, 3 + PKT_LEN);

    start_timer(frame_nr, DATA_TIMER); /* 启动帧计时器 */
    dbg_event("Start %d's DATA_TIMER\n", s.seq);
    stop_ack_timer();   /* 由于捎带了ACK，所以停止ACK计时器 */
}

/* 重传数据帧 */
static void resend_data_frame(void) {
    struct FRAME s;

    s.kind = FRAME_DATA;   /* 数据帧 */
    s.seq = oldest_frame;
    s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);
    memcpy(s.data, &out_buf[oldest_frame % NR_BUFS], PKT_LEN);

    dbg_frame("Resend DATA %d with ACK %d, ID %d\n", s.seq, s.ack, *(short *)s.data);

    put_frame((unsigned char *)&s, 3 + PKT_LEN);
    start_timer(oldest_frame, DATA_TIMER);
    dbg_event("Restart %d's DATA_TIMER\n", s.seq);
    stop_ack_timer();
}

/* 发送ACK帧 */
static void send_ack_frame(void) {
    struct FRAME s;

    s.kind = FRAME_ACK;                                 /* 确认帧 */
    s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1); /* 解释同上 */

    dbg_frame("Send ACK %d\n", s.ack);

    put_frame((unsigned char *)&s, 2);

    stop_ack_timer(); /* 单独发送ACK帧，同样停止ACK计时器 */
}

/* 发送NAK帧 */
static void send_nak_frame(void) {
    struct FRAME s;

    s.kind = FRAME_NAK;
    s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);

    no_nak = false;

    dbg_frame("Send NAK with ACK %d\n", s.ack);

    put_frame((unsigned char *)&s, 2);

    stop_ack_timer();
}

static bool between(seq_nr a, seq_nr b, seq_nr c) {
    return ((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a));
}

int main(int argc, char **argv) {
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
    dbg_event("SEQ_BITS = %d, MAX_SEQ = %d, NR_BUFS = %d, DATA_TIMER = %d, ACK_TIMER = %d\n", 
              SEQ_BITS, MAX_SEQ, NR_BUFS, DATA_TIMER, ACK_TIMER);
    disable_network_layer();

    for (;;) {
        event = wait_for_event(&arg);

        switch (event) {
            case NETWORK_LAYER_READY:
                /* 从网络层获取一帧，并存入发送缓存区 */
                get_packet((unsigned char *)&out_buf[frame_nr % NR_BUFS]);
                dbg_event("Buffer DATA %d, ID %d at out_buf[%d]\n", frame_nr, *(short *)out_buf[frame_nr % NR_BUFS].data, frame_nr % NR_BUFS);
                nbuffered++; /* 发送缓存区数量 + 1 */

                send_data_frame(); /* 发送数据帧 */
                dbg_event("Now Sender's window is from %d to %d\n", ack_expected, frame_nr);
                inc(frame_nr);

                break;

            case PHYSICAL_LAYER_READY:
                phl_ready = 1;
                break;

            case FRAME_RECEIVED:
                /* 收到一帧 */
                len = recv_frame((unsigned char *)&f, sizeof f);

                /* 判断校验位是否出错，如果出错，即属于 Frame Damage 的情况，则直接发送NAK帧 */
                if (len < 5 || crc32((unsigned char *)&f, len) != 0) {
                    dbg_event("**** Receiver Error, Bad CRC Checksum\n");
                    if (no_nak) {
                        send_nak_frame();
                    }
                    break;
                }

                if (f.kind == FRAME_ACK) {
                    dbg_frame("Recv ACK %d\n", f.ack);
                }
                if (f.kind == FRAME_DATA) {
                    dbg_frame("Recv DATA %d with ACK %d, ID %d\n", f.seq, f.ack, *(short *)f.data);

                    /* 不是期望的帧序号且没有发送过NAK帧 */
                    if (f.seq != frame_expected && no_nak) {
                        send_nak_frame();
                    } else {
                        start_ack_timer(ACK_TIMER); /* 启动ACK计时器 */
                    }

                    /* 收到的数据帧处于接收窗口内且缓存区对应位置没有其他帧缓存 */
                    if (between(frame_expected, f.seq, too_far) && arrived[f.seq % NR_BUFS] == false) {
                        /* 更新缓存区状态 */
                        arrived[f.seq % NR_BUFS] = true;
                        memcpy(&in_buf[f.seq % NR_BUFS], f.data, PKT_LEN);
                        dbg_event("Buffer DATA %d, ID %d at in_buf[%d]\n", f.seq, *(short *)f.data, f.seq % NR_BUFS);

                        /* 如果接收缓存区内有数据，则更新缓存区，前移窗口，并向网络层依次提交数据 */
                        while (arrived[frame_expected % NR_BUFS] == true) {
                            /* 将接收窗口下界的数据提交给网络层 */
                            put_packet((unsigned char *)&in_buf[frame_expected % NR_BUFS], len - 7);
                            dbg_event("Put Packet at in_buf[%d] to Network Layer\n", frame_expected % NR_BUFS);

                            no_nak = true;
                            arrived[frame_expected % NR_BUFS] = false;

                            /* 前移接收窗口 */
                            inc(frame_expected);
                            dbg_event("Now Recv's window is from %d to %d\n", frame_expected, too_far);
                            inc(too_far);

                            start_ack_timer(ACK_TIMER);   /* 启动ACK计时器 */
                        }
                    }
                }

                /* 收到NAK帧且需要重传的帧在发送窗口内 */
                if (f.kind == FRAME_NAK && between(ack_expected, (f.ack + 1) % (MAX_SEQ + 1), frame_nr)) {
                    dbg_frame("Recv NAK with ACK %d\n", f.ack);
                    oldest_frame = (f.ack + 1) % (MAX_SEQ + 1);   /* 重传当前收到的ack + 1序号的帧 */
                    resend_data_frame();
                }

                /* 如果收到的ACK处于当前的发送窗口内，则需更新发送窗口 */
                while (between(ack_expected, f.ack, frame_nr)) {
                    nbuffered--;   /* 发送缓存区数量 - 1 */
                    stop_timer(ack_expected);   /* 停止发送窗口下界对应帧的计时器 */
                    dbg_event("Stop DATA_TIMER %d\n", ack_expected);

                    /* 前移发送窗口 */
                    inc(ack_expected);
                    dbg_event("Now Sender's window is from %d to %d\n", ack_expected, (frame_nr + MAX_SEQ) % (MAX_SEQ + 1));
                }

                break;

            case DATA_TIMEOUT:
                dbg_event("---- DATA_TIMER %d timeout\n", arg);
                oldest_frame = arg;   /* 超时，则重传超时的数据帧 */
                resend_data_frame();
                break;
            
            case ACK_TIMEOUT:
                /* ACK计时器超时，说明规定时间内没有需要发送的帧，无法捎带ACK，则单独发送ACK帧 */
                dbg_event("---- ACK_TIMER %d timeout\n", arg);
                send_ack_frame();
                break;
        }

        /* 发送缓存区未满且物理层准备好，可以从网络层获取数据包 */
        if (nbuffered < NR_BUFS && phl_ready) {
            enable_network_layer();
        } else {
            disable_network_layer();
        }
    }
}