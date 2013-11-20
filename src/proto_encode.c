#include "proto_encode.h"

#include <string.h>

#include "ccan/endian/endian.h"
#include "ccan/build_assert/build_assert.h"

void proto_encode_uint32(uint8_t *buf, uint32_t val) {
  val = CPU_TO_BE32(val);

  /* code that calls this function will assume that
   * 4 bytes will be written here, so we want to fail
   * compilation if this is untrue for some reason. */
  BUILD_ASSERT(sizeof(val) == 4);
  memcpy(buf, &val, 4);
}


void proto_encode_uint16(uint8_t *buf, uint16_t val) {
  val = CPU_TO_BE16(val);

  /* code that calls this function will assume that
   * 2 bytes will be written here, so we want to fail
   * compilation if this is untrue for some reason. */
  BUILD_ASSERT(sizeof(val) == 2);
  memcpy(buf, &val, 2);
}

size_t proto_encode_op_vint(uint8_t *buf, size_t size, uint32_t val) {
  size_t pkt_sz;

  if(val <= 0x255) {
    pkt_sz = 2;

    if(size >= pkt_sz) {
      buf[0] = PD_OP_VINT1;
      buf[1] = (uint8_t) val;
    }
  }
  else if(val <= 0xffff) {
    pkt_sz = 3;

    if(size >= pkt_sz) {
      buf[0] = PD_OP_VINT2;
      proto_encode_uint16(buf+1, (uint16_t) val);        
    }
  }
  else {
    pkt_sz = 5;

    if(size >= pkt_sz) {
      buf[0] = PD_OP_VINT4;
      proto_encode_uint32(buf+1, val);
    }
  }

  return pkt_sz;
}
