#include "SerialTX.h"

#include <stdint.h>

extern "C" {
  #include "proto_encode.h"
}

#define STATUS_INTERVAL 1000 /* milliseconds */
#define PING_INTERVAL (5*1000) /* milliseconds */
#define INPUT_TIMEOUT (5*1000) /* milliseconds */

SerialTX s_output(11);
uint32_t g_time,
         g_last_ping,
         g_last_input,
         g_total,
         g_count,
         g_write_fails;

void send_err() {
  Serial.write(PD_OP_ERR);
}

void send(uint8_t *buf, size_t amt) {
  if(Serial.write(buf, amt) != amt) {
    send_err();
  }

  g_last_ping = millis();
}

void send_op(proto_op_t op) {
  if(Serial.write(op) != 1)
    send_err();
}

void send_err_write() {
  send_op(PD_OP_ERR_WRITE);
}

void send_err_overflow() {
  send_op(PD_OP_ERR_OVERFLOW);
}

void send_count(uint32_t v) {
  uint8_t buf[16];
  size_t sz;

  sz = proto_encode_op_vint(buf, sizeof(buf), v);
  if(sz > sizeof(buf))
    send_err();
  else
    send(buf, sz);
}

void send_status() {
  uint32_t t;
  uint32_t new_count;

  t = millis();
  if((g_count > 0) || (t - g_last_ping > PING_INTERVAL)) {
    new_count = g_total + g_count;
    if(new_count < g_total)
      send_err_overflow();

    g_total = new_count;
    send_count(g_total);
    g_count = 0;
  }
}

void status(int force) {
  uint32_t t;

  t = millis();
  /* t overflow is ok, it just means
   * we might update the status early */
  if( (force != 0) || (t - g_time > STATUS_INTERVAL) ) {
    send_status();
    g_time = t;
  }
}

void setup() {
  /* set the rate for the input */
  Serial.begin(115200);
  s_output.begin(115200);

  g_time = millis();
  g_count = 0;
  g_total = 0;
  g_write_fails = 0;
  g_last_ping = 0;

  send_count(0);
}

void loop() {
  int b;

  if((b = Serial.read()) >= 0) {
    if(s_output.write(b) != 1)
      send_err_write();
    else {
      g_last_input = millis();
      g_count += 1;
    }

    status(0);
  }
  else {
    status(1);
    
    if(millis() - g_last_input > INPUT_TIMEOUT) {
      /* reset state on idle input */
      g_count = 0;
      g_total = 0;
    }
  }
}
