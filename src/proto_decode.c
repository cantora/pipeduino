#include "proto.h"

#include <stdlib.h>

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
    pkt->contents.err_msg = "unspecified error";
  case PD_OP_ERR_WRITE:
    pkt->contents.err_msg = "device failed to write to output";
  case PD_OP_ERR_OVERFLOW:
    pkt->contents.err_msg = "device counter overflowed";
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
    pkt->contents.err_msg = "protocol error";
  }

  return pkt_sz;
}
