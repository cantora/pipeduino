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

struct pipeduino {
  pthread_mutex_t counter_mtx;
  pthread_cond_t counter_cond;
  uint64_t counter;
  int sfd, zeros, done, waiting;
};

void err_exit(int error, const char *format, ...) {
  va_list args;

  printf("\n");
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);

  if(error != 0)
    fprintf(stderr, ": %s (%d)", strerror(error), error );

  fputc('\n', stderr);
  fflush(stderr);
  exit(1);
}

void pipeduino_counter_lock(struct pipeduino *p) {
  int status;
  if( (status = pthread_mutex_lock(&p->counter_mtx)) != 0)
    err_exit(status, "failed to lock counter");
}

void pipeduino_counter_unlock(struct pipeduino *p) {
  int status;
  if( (status = pthread_mutex_unlock(&p->counter_mtx)) != 0)
    err_exit(status, "failed to unlock counter");
}

void pipeduino_counter_wait(struct pipeduino *p) {
  int status;
  if( (status = pthread_cond_wait(&p->counter_cond, &p->counter_mtx)) != 0)
    err_exit(status, "cond wait returned an error");
}

void pipeduino_counter_signal(struct pipeduino *p) {
  int status;
  if( (status = pthread_cond_signal(&p->counter_cond)) != 0)
    err_exit(status, "cond signal returned an error");
}

void flush_all(int fd) {
  uint8_t buf[4096];
  while(read(fd, buf, sizeof(buf)) > 0) {}
}

int serial_fd() {
  int fd, flags;

  flags = O_RDWR | O_NOCTTY | O_NDELAY; /*| O_DIRECT */
  if( (fd = open(PIPEDUINO_PATH, flags)) == -1)
    err_exit(errno, "failed to open serial connection to arduino");

  return fd;
}

void on_counter_init(struct pipeduino *p, uint32_t counter) {
  printf("\nzeros=%d -> ", p->zeros);
  if(counter == 0) {
    printf("increment\n");
    p->zeros += 1;
  }
  else {
    printf("no zero matched, reset.\n");
    p->zeros = 0;
  }
}

void on_counter(struct pipeduino *p, uint32_t counter) {
  const char *err;
  uint64_t amt;
  const uint64_t max_amt = 212;
  static uint64_t prev = 0;
  static int stalls = 0;

  err = NULL;
  //printf("counter=%d\r", counter);

  if(p->zeros < PIPEDUINO_INIT_ZEROS) 
    return on_counter_init(p, counter);

  pipeduino_counter_lock(p);
  if(counter < p->counter) {
    if(prev >= counter) {
      if(stalls > 2)
        err = "arduino counter stalled out, there must have been an overflow";
      else
        stalls += 1;

      amt = 0; //p->counter - counter;
    }
    else if(prev < counter) {
      /*
      amt = counter - prev;
      if(amt > max_amt)
        amt = max_amt;
      */
      amt = 0;
      stalls = 0;
    }
  }
  else if(counter == p->counter) {
    amt = max_amt;
    stalls = 0;
  }
  else if(p->counter > 0) {
    err = "error: somehow arduino counter is ahead of ours";
  }

  /*printf("counter=%u, arduino=%u\n", p->counter, counter);*/
  if(err == NULL && amt > 0) {
    p->counter += amt;
    if(p->waiting != 0)
      pipeduino_counter_signal(p);
  }
  pipeduino_counter_unlock(p);

  if(err != NULL) {
    err_exit(0, err);
  }

  prev = counter;
}

void on_msg(struct pipeduino *p, const struct proto_msg *msg) {

  switch(msg->type) {
  case PD_MSG_COUNTER:
    on_counter(p, msg->contents.counter);
    break;
  case PD_MSG_ERR:
    switch(msg->contents.error.op) {
    case PD_OP_ERR_PROTO:
      if(p->zeros < PIPEDUINO_INIT_ZEROS)
        break;
      err_exit(0, "proto error from arduino: byte=%02x\n", msg->contents.error.byte);
      break;
    default:
      err_exit(0, "error from arduino: %s", msg->contents.error.msg);
    }
    break;
  default:
    err_exit(0, "unexpected message type: %d", msg->type);
  }

}

void reader(struct pipeduino *p) {
  int brk, status, first_read;
  uint8_t buf[1024];
  ssize_t amt;
  struct proto_stream strm;
  struct proto_msg msg;
  
  proto_decode_stream_init(&strm);

  flush_all(p->sfd);

  brk = 0;
  first_read = 0;
  printf("wait for signal from arduino...\n");
  do {
    if(p->done != 0)
      return;

    amt = read(p->sfd, buf, 256);
    if(amt < 0) {
      switch(errno) {
      case EAGAIN:
        continue;
      default:
        fprintf(stderr, "error while reading from serial device %d: ", errno);
        puts(strerror(errno));
        brk = 1;
      }
    }
    else if(amt == 0)
      brk = 1;
    else {
      if(first_read == 0) {
        printf("read %ld bytes from arduino\n", amt);
        first_read = 1;
      }
      /* int i; printf("read: "); for(i = 0; i < amt; i++) { printf("%02x", buf[i]);}; puts(""); */
      proto_decode_stream_buf(&strm, buf, amt);
      while( (status = proto_decode_stream_next(&strm, &msg)) == 1) {
        on_msg(p, &msg);
      }
      fflush(stdout);
    }

  } while(brk == 0);

  fprintf(stderr, "reader finished\n");
}

void *reader_main(void *user) {
  struct pipeduino *state = (struct pipeduino *) user;

  reader(state);
  return NULL;
}

/* specific to writing to a serial device in that
 * a return status of 0 from write is interpreted
 * as EAGAIN */
int write_all(int sfd, uint8_t *buf, ssize_t amt) {
  ssize_t w_amt, total;
  
  total = 0;
  do {
    w_amt = write(sfd, (buf + total), (amt-total));

    if(w_amt == 0) {
      continue;
    }
    else if(w_amt < 0) {
      switch(errno) {
      case EAGAIN:
        continue;
      default:
        fprintf(stderr, "error while writing to sfd %d: ", errno);
        puts(strerror(errno));
        return -1;
      }
    }
    total += w_amt;
  } while(total < amt);

  return 0;
}

void print_status(const struct pipeduino *state, time_t start_time, uint64_t total) {
  time_t diff;
  double avg, total_k;
  (void)(state);

  diff = time(NULL) -  start_time;

  if(diff > 0) {
    total_k = total/((double) 1024);
    avg = total_k/diff;
    printf("[%.2f kb/s] >> %.1f k            \r", avg, total_k);
    fflush(stdout);
  }
}

uint64_t wait_for_counter(struct pipeduino *p, 
                      int show_status, 
                      time_t start_time, 
                      uint64_t total) {
  uint64_t limit;

  pipeduino_counter_lock(p);
  while(p->counter <= total) {
    if(show_status != 0)
      print_status(p, start_time, total);
    p->waiting = 1;
    //printf("waiting=%u\n", total);
    pipeduino_counter_wait(p);
    //printf("woke up\n");
    /* counter is locked */
    p->waiting = 0;
  } 
  limit = p->counter;
  pipeduino_counter_unlock(p);

  return limit;
}

uint64_t write_loop(struct pipeduino *state) {
  uint8_t buf[256];
  ssize_t buf_amt;
  uint64_t total, limit, amt;
  time_t start_time;

  total = 0;
  limit = 0;
  buf_amt = 0;
  start_time = time(NULL);

  //printf("write loop\n");
  while(1) {

    if(total >= limit)
      limit = wait_for_counter(state, 1, start_time, total);

    //printf("limit=%u\n", limit);
    amt = (limit - total);
    if(amt < 1)
      continue;
    else if(amt > sizeof(buf))
      amt = sizeof(buf);

    buf_amt = read(STDIN_FILENO, buf, amt);
    if(buf_amt < 0) {
      if(errno == EAGAIN)
        continue;
      fprintf(stderr, "error while reading from stdin %d: %s", errno, strerror(errno));
      break;
    }
    else if(buf_amt == 0) {
      fprintf(stderr, "\nstdin closed. wait for arduino\n");
      wait_for_counter(state, 0, start_time, total);
      break;
    }

    //printf("write buf_amt=%u\n", buf_amt);
    if(write_all(state->sfd, buf, buf_amt) != 0)
      break;
    else
      total += buf_amt;
  }

  return total;
}

int main(int argc, char *argv[]) {
  struct pipeduino state;
  int status;
  speed_t speed;
  struct termios old_termios;
  pthread_t reader_thread;
  size_t amt_written;

  (void)(argc); (void)(argv);
  
  if( (status = pthread_mutex_init(&state.counter_mtx, NULL)) != 0)
    err_exit(status, "mutex init failed");
  if( (status = pthread_cond_init(&state.counter_cond, NULL)) != 0)
    err_exit(status, "cond init failed");

  state.counter = 0;
  state.sfd = serial_fd();
  state.zeros = 0;
  state.done = 0;
  state.waiting = 0;
  if(tcgetattr(state.sfd, &old_termios) != 0)
    err_exit(errno, "tcgetattr failed");

  if(term_speed(PIPEDUINO_BAUD, &speed) != 0)
    err_exit(0, "invalid terminal speed %d\n", PIPEDUINO_BAUD);

  if(term_set_attrs(state.sfd, speed, &old_termios) != 0)
    err_exit(errno, "failed to set terminal attrs");

  status = pthread_create(&reader_thread, NULL, reader_main, (void *)&state);
  if(status != 0)
    err_exit(status, "failed to create reader thread");

  amt_written = write_loop(&state);
  state.done = 1;
  printf("wrote: %zu bytes\n", amt_written);

  return 0;
}
