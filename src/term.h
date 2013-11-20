#ifndef PIPEDUINO_term_h
#define PIPEDUINO_term_h

#include <termios.h>

int term_speed(int, speed_t *);
int term_set_attrs(int, speed_t, const struct termios *);

#endif /* PIPEDUINO_term_h */
