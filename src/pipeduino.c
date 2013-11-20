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

#include "term.h"

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

  flags = O_RDWR | O_NOCTTY | O_NDELAY; /*| O_DIRECT | O_NOTTY;*/
  if( (fd = open(PIPEDUINO_PATH, flags)) == -1)
    err_exit(errno, "failed to open serial connection to arduino");

  return fd;
}

int main(int argc, char *argv[]) {
  int sfd, brk;
  speed_t speed;
  struct termios old_termios;
  uint8_t buf[256];
  ssize_t amt;
  (void)(argc); (void)(argv);
  
  fprintf(stderr, "hey\n");
  sfd = serial_fd();

  if(tcgetattr(sfd, &old_termios) != 0)
    err_exit(errno, "tcgetattr failed");

  if(term_speed(PIPEDUINO_BAUD, &speed) != 0)
    err_exit(0, "invalid terminal speed %d\n", PIPEDUINO_BAUD);

  if(term_set_attrs(sfd, speed, &old_termios) != 0)
    err_exit(errno, "failed to set terminal attrs");

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
    else
      if(write(STDOUT_FILENO, buf, amt) != amt)
        err_exit(errno, "failed to write to stdout");
  } while(brk == 0);

  tcsetattr(sfd, TCSAFLUSH, &old_termios);
  close(sfd);
  return 0;
}
