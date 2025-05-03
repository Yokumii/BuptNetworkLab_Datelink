#ifndef DATALINK_H
#define DATALINK_H

#ifndef SEQ_BITS
#define SEQ_BITS 6
#endif

#define MAX_SEQ ((1 << SEQ_BITS) - 1) /* MAX_SEQ should be 2^SEQ_BITS - 1 */
#define NR_BUFS ((MAX_SEQ + 1) / 2)
#define inc(k) \
  (k = (k + 1) % (MAX_SEQ + 1)) /* 使得序号 k 在 0 ~ MAX_SEQ 之间依次递增*/

#ifndef DATA_TIMER
#define DATA_TIMER 2000
#endif

#ifndef ACK_TIMER
#define ACK_TIMER 666
#endif

/* FRAME kind */
#define FRAME_DATA 1
#define FRAME_ACK 2
#define FRAME_NAK 3

/*
    DATA Frame
    +=========+========+========+===============+========+
    | KIND(1) | SEQ(1) | ACK(1) | DATA(240~256) | CRC(4) |
    +=========+========+========+===============+========+

    ACK Frame
    +=========+========+========+
    | KIND(1) | ACK(1) | CRC(4) |
    +=========+========+========+

    NAK Frame
    +=========+========+========+
    | KIND(1) | ACK(1) | CRC(4) |
    +=========+========+========+
*/

#endif
