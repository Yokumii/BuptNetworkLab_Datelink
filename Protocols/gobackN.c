#include <stdbool.h>
#include <string.h>

#include "../datalink.h"
#include "../protocol.h"

#ifdef SEQ_BITS
#  undef SEQ_BITS
#  define SEQ_BITS 6
#endif

#ifdef DATA_TIMER
#  undef DATA_TIMER
#  define DATA_TIMER 263
#endif

#define ACCUM_ACK()                                 \
  while (between(ack_expected, f.ack, send_base)) { \
    --nbuffered;                                    \
    stop_timer(ack_expected);                       \
    inc(ack_expected);                              \
  }

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
static struct BUFFER buffer[NR_BUFS]; /* 发送方缓存区 */
static unsigned int send_base = 0;    /* next_frame_to_send */
static unsigned int ack_expected = 0; /* oldest frame yet unack */
static unsigned int nbuffered = 0;
static int phl_ready = 0;
static bool ack_pending = false;
static bool is_first = false;
//========= 接收方状态 =========//

static unsigned int frame_expected = 0;

//========= 协议核心函数 =========//
static void put_frame(unsigned char *frame, int len) {
  *(unsigned int *)(frame + len) = crc32(frame, len);
  send_frame(frame, len + 4);
  phl_ready = 0;
}

static bool between(int a, int b, int c) {
  return ((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a));
}

static void send_data_frame(void) {
  struct FRAME s;
  s.kind = FRAME_DATA;
  s.seq = send_base;
  s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);
  memcpy(s.data, &buffer[send_base % NR_BUFS], PKT_LEN);

  dbg_frame("Send DATA %d ack%d, ID %d\n", s.seq, s.ack, *(short *)s.data);
  put_frame((unsigned char *)&s, 3 + PKT_LEN);

  start_timer(send_base, DATA_TIMER);
}

static void send_ack_frame(void) {
  struct FRAME s;
  s.kind = FRAME_ACK;
  s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);

  dbg_frame("Send ACK  %d\n", s.ack);
  put_frame((unsigned char *)&s, 2);
}

//========= 主协议循环 =========//
int main(int argc, char **argv) {
  int event, arg;
  struct FRAME f;
  int len;

  protocol_init(argc, argv);
  lprintf("Go-Back-N Protocol (MAX_SEQ=%d, N=%d, DATA_TIMER=%d)\n", MAX_SEQ, NR_BUFS, DATA_TIMER);
  lprintf("designed by Wang tianyi");
  enable_network_layer();
  phl_ready = 0;

  for (;;) {
    event = wait_for_event(&arg);

    switch (event) {
      case NETWORK_LAYER_READY:
        is_first = true;
        while (nbuffered < NR_BUFS && phl_ready) {
          get_packet((unsigned char *)&buffer[send_base % NR_BUFS]);
          dbg_event("Buffer DATA %d, ID %d at buffer[%d]\n", send_base, *(short *)buffer[send_base % NR_BUFS].data, send_base % NR_BUFS);
          ++nbuffered;
          send_data_frame();
          if (is_first) {
            is_first = false;
            stop_ack_timer();     // 清除 ACK 延迟机制
            ack_pending = false;  // 进入数据帧 piggyback 模式
          }
          inc(send_base);
        }
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
          /* ACK 累计确认 */
          dbg_frame("Recv ACK  %d\n", f.ack);
          ACCUM_ACK();
        } else if (f.kind == FRAME_DATA) {
          dbg_frame("Recv DATA %d ack%d, ID %d\n", f.seq, f.ack, *(short *)f.data);
          if (f.seq == frame_expected) {
            put_packet(f.data, len - 7);
            inc(frame_expected);

            if (!ack_pending) {
              start_ack_timer(ACK_TIMER);
              ack_pending = true;
            }
          }
          /* 处理 piggybacking */
          ACCUM_ACK();
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
        ack_pending = false;
        break;
    }
    // 网络层控制：窗口未满且物理层就绪时启用
    if (nbuffered < NR_BUFS && phl_ready) {
      enable_network_layer();
    } else {
      disable_network_layer();
    }
  }
}