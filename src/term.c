#include "term.h"

#include <string.h>

int term_speed(int brate, speed_t *speed) {
  switch(brate) {
  case 4800:
    *speed = B4800;
    break;
  case 9600:
    *speed = B9600;
    break;
  case 19200:
    *speed = B19200;
    break;
  case 38400:
    *speed = B38400;
    break;
  case 57600:
    *speed = B57600;
    break;
  case 115200:
    *speed = B115200;
    break;
  default:
    return -1;
  }

  return 0;
}

int term_set_attrs(int fd, speed_t speed, const struct termios *old_attr) {
  struct termios attr;

  memcpy(&attr, old_attr, sizeof(attr));

  cfsetispeed(&attr, speed);
  cfsetospeed(&attr, speed);

  attr.c_cflag &= ~(
    CSIZE     /* clear character size mask */
    | PARENB  /* no parity */
    | CSTOPB  /* dont use 2 stop bits */
    | CRTSCTS /* no flow control */
  );
  attr.c_cflag |= (
    CS8      /* 8 bit characters */
    | CREAD  /* receive data */
    | CLOCAL /* ignore modem control */
  );
  
  attr.c_iflag &= ~(
    IXON      /* disable output flow control */
    | IXANY   
    | IXOFF   /* disable flow control on input */
    | INLCR    /* dont translate NL */
    | ICRNL    /* dont translate CR */
#if 0 /* not sure if we want these or not */
    | IGNBRK  /* ignore break on input */
    | BRKINT  /* ignore break */
    | PARMRK  /* ingore framing/parity errors */
#endif
  );

  attr.c_oflag &= ~OPOST; /* implementation defined output processing */
  attr.c_lflag &= ~(
    ICANON     /* turn off canonical mode */
    | ECHO     /* no echo */
    | ECHOE    /* no erase */
    | ECHOK    /* no kill */
    | ECHONL   /* no new line */
    | ISIG     /* no signals */
  );

  /* http://unixwiz.net/techtips/termios-vmin-vtime.html
   * VTIME => 
   *   read() returns when VMIN tenths of a second have elapsed.
   * VMIN => 
   *   read() returns when at least VMIN characters are in the
   *   input buffer.
   */
  /* wait until at least 20 characters are available */
  attr.c_cc[VMIN] = 20;
  /* or until the burst of characters is done */
  attr.c_cc[VTIME] = 3;  
  
  if(tcsetattr(fd, TCSAFLUSH, &attr) != 0)
    return -1;

  return 0;
}
