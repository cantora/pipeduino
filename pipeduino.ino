#include "SerialTX.h"

#define STATUS_INTERVAL 1000 /* milliseconds */
#define PING_INTERVAL (5*1000) /* milliseconds */

SerialTX s_output(11);
unsigned long g_time, g_count, g_write_fails, g_last_ping;

void send(const char *name, unsigned long v) {
  Serial.write('\r');
  Serial.write('!');
  Serial.print(name);
  Serial.write('=');
  Serial.print(v);
  Serial.print('\n');

  /* any message counts as a ping */
  g_last_ping = millis();
}

void ping(unsigned long t) {
  Serial.println("\r*");
  g_last_ping = t;
}

void send_status() {
  unsigned long t;

  if(g_count > 0) {
    send("count", g_count);
    g_count = 0;
  }

  if(g_write_fails > 0) {
    send("write_fails", g_write_fails);
    g_write_fails = 0;
  }

  t = millis();
  if(t - g_last_ping > PING_INTERVAL) {
    ping(t);
  }
}

void status() {
  unsigned long t;

  t = millis();
  /* t overflow is ok, it just means
   * we might update the status early */
  if(t - g_time > STATUS_INTERVAL) {
    send_status();
    g_time = t;
  }
}

void setup() {
  int rate = 9600;
  /* set the rate for the input */
  Serial.begin(rate);
  s_output.begin(rate);

  g_time = millis();
  g_count = 0;
  g_write_fails = 0;
  g_last_ping = 0;

  ping(millis());
}

void loop() {
  int b;

  if((b = Serial.read()) >= 0) {
    if(s_output.write(b) != 1)
      g_write_fails += 1;
    else
      g_count += 1;
  }

  status();
}
