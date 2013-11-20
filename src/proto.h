#ifndef PIPEDUINO_proto_h
#define PIPEDUINO_proto_h

#include <stdint.h>

typedef enum {
  PD_OP_ERR = 0x21,         /* unspecified error */

  PD_OP_ERR_WRITE = 0x23,   /* serial device had an error writing to output */ 
  PD_OP_ERR_OVERFLOW,       /* counter overflowed */
  PD_OP_ERR_PROTO,          /* protocol error */

  PD_OP_VINT1 = 0x31,       /* N byte unsigned int representing      */
  PD_OP_VINT2,              /* the current byte counter to follow.   */
  PD_OP_VINT4
} proto_op_t;

typedef enum {
  PD_MSG_ERR = 0x0,
  PD_MSG_COUNTER
} proto_type_t;

struct proto_msg {
  proto_type_t type;
  union {
    struct {
      char *msg;
      uint8_t byte;
    } error;
    uint32_t counter;
  } contents;
};

#endif /* PIPEDUINO_proto_h */
