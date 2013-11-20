#include "configure.h"

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>

#include "term.h"
#include "proto_decode.h"

void err_exit(int error, const char *format, ...) {
  va_list args;

  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);

  if(error != 0)
    fprintf(stderr, ": %s (%d)", strerror(error), error );

  fputc('\n', stderr);
  fflush(stderr);
  exit(1);
}

int serial_fd() {
  int fd, flags;

  flags = O_RDWR | O_NOCTTY | O_NDELAY; /*| O_DIRECT */
  if( (fd = open(PIPEDUINO_PATH, flags)) == -1)
    err_exit(errno, "failed to open serial connection to arduino");

  return fd;
}

void reader(int sfd) {
  int brk, status;
  uint8_t buf[256];
  ssize_t amt;
  struct proto_stream strm;
  struct proto_msg msg;
  
  proto_decode_stream_init(&strm);

  brk = 0;
  do {
    amt = read(sfd, buf, 256);
    if(amt < 0) {
      switch(errno) {
      case EAGAIN:
        continue;
      default:
        printf("error while reading from serial device %d: ", errno);
        puts(strerror(errno));
        brk = 1;
      }
    }
    else if(amt == 0)
      brk = 1;
    else {
      proto_decode_stream_buf(&strm, buf, amt);
      while( (status = proto_decode_stream_next(&strm, &msg)) == 1) {
        switch(msg.type) {
        case PD_MSG_COUNTER:
          printf("counter=%d\n", msg.contents.counter);
          break;
        case PD_MSG_ERR:
          err_exit(0, "error from arduino: byte=%02x, %s", msg.contents.error.byte, msg.contents.error.msg);
          break;
        default:
          err_exit(0, "unexpected message type: %d", msg.type);
        }
      }
      fflush(stdout);
    }

  } while(brk == 0);
}

void *reader_main(void *user) {
  int sfd = (int) user;

  reader(sfd);
  return NULL;
}

int write_all(int fd, uint8_t *buf, ssize_t amt) {
  ssize_t w_amt, total;
  
  total = 0;
  do {
    w_amt = write(fd, (buf + total), (amt-total));

    if(w_amt == 0) {
      printf("fd closed\n");
      return -1;
    }
    else if(w_amt < 0) {
      switch(errno) {
      case EAGAIN:
        continue;
      default:
        printf("error while writing to fd %d: ", errno);
        puts(strerror(errno));
        return -1;
      }
    }
    total += w_amt;
  } while(total < amt);

  return 0;
}

void write_loop(int sfd) {
  uint8_t buf[256];
  ssize_t r_amt;
  uint64_t total;
  int brk;

  total = 0;
  brk = 0;
  do {
    r_amt = read(STDIN_FILENO, buf, sizeof(buf));
    if(r_amt < 0) {
      switch(errno) {
      case EAGAIN:
        continue;
      default:
        printf("error while reading from stdin %d: ", errno);
        puts(strerror(errno));
        brk = 1;
      }
    }
    else if(r_amt == 0)
      brk = 1;

    if(write_all(sfd, buf, r_amt) != 0)
      brk = 1;
    else
      total += r_amt;
  } while(brk == 0);
}

int main(int argc, char *argv[]) {
  int sfd, status;
  speed_t speed;
  struct termios old_termios;
  pthread_t reader_thread;

  (void)(argc); (void)(argv);
  
  fprintf(stderr, "hey\n");
  sfd = serial_fd();

  if(tcgetattr(sfd, &old_termios) != 0)
    err_exit(errno, "tcgetattr failed");

  if(term_speed(PIPEDUINO_BAUD, &speed) != 0)
    err_exit(0, "invalid terminal speed %d\n", PIPEDUINO_BAUD);

  if(term_set_attrs(sfd, speed, &old_termios) != 0)
    err_exit(errno, "failed to set terminal attrs");

  status = pthread_create(&reader_thread, NULL, reader_main, (void *)sfd);
  if(status != 0)
    err_exit(status, "failed to create reader thread");

  write_loop(sfd);

  return 0;
}
