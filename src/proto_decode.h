#include "proto.h"

#include <stdlib.h>

size_t proto_decode(const uint8_t *, size_t, struct proto_msg *);

struct proto_stream {
  uint8_t buf1[16];
  size_t end;

  uint8_t *buf2;
  size_t buf2_sz;
};

static inline void proto_decode_stream_init(struct proto_stream *strm) {
  strm->end = 0;
  strm->buf2 = NULL;
  strm->buf2_sz = 0;
}

static inline void proto_decode_stream_buf(struct proto_stream *strm,
                             uint8_t *buf, size_t size) {
  strm->buf2 = buf;
  strm->buf2_sz = size;
}

int proto_decode_stream_next(struct proto_stream *, struct proto_msg *);
