#include "proto_decode.h"

#include <string.h>
#include <assert.h>

#include "ccan/endian/endian.h"

static uint32_t proto_decode_uint32(const uint8_t *buf) {
  uint32_t val = *((uint32_t *) buf);

  return BE32_TO_CPU(val);
}

static uint16_t proto_decode_uint16(const uint8_t *buf) {
  uint16_t val = *((uint16_t *) buf);

  return BE16_TO_CPU(val);
}

size_t proto_decode_varint(const uint8_t *buf, size_t size, uint32_t *val) {
  size_t pkt_sz;

  if(size < 1)
    return 0;

  switch(buf[0]) {
  case PD_OP_VINT1:
    pkt_sz = 2;
    if(size >= pkt_sz)
      *val = (uint32_t) buf[1];
    break;

  case PD_OP_VINT2:
    pkt_sz = 3;
    if(size >= pkt_sz)
      *val = (uint32_t) proto_decode_uint16(buf+1);
      break;

  default: /* PD_OP_VINT4 */
    pkt_sz = 5;
    if(size >= pkt_sz)
      *val = (uint32_t) proto_decode_uint32(buf+1);
  }

  return pkt_sz;
}

size_t proto_decode(const uint8_t *buf, size_t size, struct proto_msg *pkt) {
  size_t pkt_sz;

  if(size < 1)
    return 0;

  switch(buf[0]) {
  case PD_OP_ERR:
    pkt->contents.error.msg = "unspecified error";
  case PD_OP_ERR_WRITE:
    pkt->contents.error.msg = "device failed to write to output";
  case PD_OP_ERR_OVERFLOW:
    pkt->contents.error.msg = "device counter overflowed";
    pkt->contents.error.op = buf[0];
    pkt_sz = 1;
    pkt->type = PD_MSG_ERR;
    break;

  case PD_OP_VINT1:
  case PD_OP_VINT2:
  case PD_OP_VINT4:
    pkt->type = PD_MSG_COUNTER;
    pkt_sz = proto_decode_varint(buf, size, &pkt->contents.counter);
    break;

  case PD_OP_ERR_PROTO:
  default:
    pkt_sz = 1; /* move to next byte if we want to keep going */
    pkt->type = PD_MSG_ERR;
    pkt->contents.error.msg = "protocol error";
    pkt->contents.error.op = PD_OP_ERR_PROTO;
    pkt->contents.error.byte = buf[0];
  }

  return pkt_sz;
}

int proto_decode_stream_next(struct proto_stream *strm, 
                        struct proto_msg *msg) {
  size_t sz, needed;

  sz = 0;
  if(strm->end > 0
      && (sz = proto_decode(strm->buf1, strm->end, msg)) <= strm->end) {
    strm->end -= sz;
    return 1;
  }
  needed = sz - strm->end;

#define PD_BOUNDS_CHECK(x) \
  if(strm->end + x >= sizeof(strm->buf1)) return -1

#define PD_ADJUST_BUF2(x) \
  do { \
    strm->buf2_sz -= x; \
    if(strm->buf2_sz < 1) \
      strm->buf2 = NULL; \
    else \
      strm->buf2 += x; \
  } while(0)

  if(strm->end > 0) { /* didnt have enough data in buf1 for full packet */
    if(strm->buf2_sz < needed) {
      PD_BOUNDS_CHECK(strm->buf2_sz);

      memcpy(strm->buf1+strm->end, strm->buf2, strm->buf2_sz);
      strm->end += strm->buf2_sz;
      PD_ADJUST_BUF2(strm->buf2_sz);
      return 0;
    }
    else {
      PD_BOUNDS_CHECK(needed);
      memcpy(strm->buf1+strm->end, strm->buf2, needed);
      PD_ADJUST_BUF2(needed);
      sz = strm->end + needed;
      if(proto_decode(strm->buf1, sz, msg) != sz)
        assert(0); /* this shouldnt happen */
      strm->end = 0;
      return 1;
    }
  }
  
  /* strm->end == 0 */
  if(strm->buf2_sz > 0
      && (sz = proto_decode(strm->buf2, strm->buf2_sz, msg)) <= strm->buf2_sz) {
	PD_ADJUST_BUF2(sz);
    return 1;
  }

  /* we have exhausted buf1 and buf2 */
  PD_BOUNDS_CHECK(strm->buf2_sz);
  memcpy(strm->buf1, strm->buf2, strm->buf2_sz);
  strm->end = strm->buf2_sz;
  PD_ADJUST_BUF2(strm->buf2_sz);

#undef PD_BOUNDS_CHECK
#undef PD_ADJUST_BUF2
  return 0;
}