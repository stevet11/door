#include <iostream>

// raw mode
#include <termios.h>
#include <unistd.h>

#include "door.h"

// let's try this!
#include <signal.h>
#include <unistd.h>

void done(int signal) {
  std::cout << "\r\nWORP WORP\r\n";
  std::cout.flush();
}

#include <ctype.h>

struct termios tio_default;

void raw(void) {
  // enable terminal RAW mode
  struct termios tio_raw;
  tcgetattr(STDIN_FILENO, &tio_default);
  tio_raw = tio_default;
  cfmakeraw(&tio_raw);
  /*
  This works in the console, but fails with a terminal.
  Ok, I am getting (what I would expect), but we're never timing out
  now.  (So it has to fill up the buffer before exiting...)
  CRAP!
  */

  // Ok!  I need the extra sauce here

  tio_raw.c_cc[VMIN] = 0;
  tio_raw.c_cc[VTIME] = 1;

  // tio_raw.c_iflag &= ~(ICRNL | IXON);

  tcsetattr(STDIN_FILENO, TCSANOW, &tio_raw);
}

void reset(void) { tcsetattr(STDIN_FILENO, TCOFLUSH, &tio_default); }

#define CRNL "\r\n"

/*
NOTE:  cr (from syncterm), gives 0x0d 0x00

 */
signed int getch(void) {
  fd_set socket_set;
  struct timeval tv;
  int select_ret = -1;
  int recv_ret;
  char key;

  while (select_ret == -1) {
    FD_ZERO(&socket_set);
    FD_SET(STDIN_FILENO, &socket_set);

    tv.tv_sec = 0;
    tv.tv_usec = 100;

    select_ret = select(STDIN_FILENO + 1, &socket_set, NULL, NULL, &tv);
    // select(STDIN_FILENO + 1, &socket_set, NULL, NULL, bWait ? NULL : &tv);
    if (select_ret == -1) {
      if (errno == EINTR)
        continue;
      return (-2);
    }
    if (select_ret == 0)
      return (-1);
  }

  recv_ret = read(STDIN_FILENO, &key, 1);
  if (recv_ret != 1) {
    std::cout << "eof?" << CRNL;
    std::cout.flush();
    return -2;
  }
  return key;
}

char buffer[10];
int bpos = 0;

void unget(char c) {
  if (bpos < sizeof(buffer) - 1) {
    buffer[bpos] = c;
    bpos++;
  }
}

char get(void) {
  if (bpos == 0) {
    return 0;
  }
  bpos--;
  char c = buffer[bpos];
  return c;
}

signed int getkey(void) {
  signed int c, c2;

  if (bpos != 0) {
    c = get();
  } else {
    c = getch();
  };

  if (c < 0)
    return c;

  /*
  What happens:  syncterm gives us 0x0d 0x00 on [Enter].
  This strips out the possible null.
   */

  if (c == 0x0d) {
    c2 = getch();
    if ((c2 != 0) and (c2 >= 0))
      unget(c2);
    return c;
  }

  if (c == 0x1b) {
    // possible extended key
    c2 = getch();
    if (c2 < 0) {
      // nope, just plain ESC
      return c;
    }

    char extended[10];
    int pos = 0;
    extended[pos] = (char)c2;
    extended[pos + 1] = 0;
    pos++;
    while ((pos < sizeof(extended) - 1) and ((c2 = getch()) >= 0)) {
      // handle special case when I'm smashing out cursor location requests
      // and \x1b[X;YR strings get buffered
      if (c2 == 0x1b) {
        unget(c2);
        break;
      }
      extended[pos] = (char)c2;
      extended[pos + 1] = 0;
      pos++;
    }

    // FUTURE:  Display debug when we fail to identify the key
#ifdef DEBUGGS
    std::cout << CRNL "DEBUG:" CRNL "ESC + ";
    for (int x = 0; x < pos; x++) {
      char z = extended[x];
      if (iscntrl(z)) {
        std::cout << (int)z << " ";
      } else {
        std::cout << "'" << (char)z << "'"
                  << " ";
      };
    }
#endif

    if (extended[0] == '[') {
      switch (extended[1]) {
      case 'A':
        return XKEY_UP_ARROW;
      case 'B':
        return XKEY_DOWN_ARROW;
      case 'C':
        return XKEY_RIGHT_ARROW;
      case 'D':
        return XKEY_LEFT_ARROW;
      case 'H':
        return XKEY_HOME;
      case 'F':
        return XKEY_END; // terminal
      case 'K':
        return XKEY_END;
      case 'U':
        return XKEY_PGUP;
      case 'V':
        return XKEY_PGDN;
      case '@':
        return XKEY_INSERT;
      };

      if (extended[pos - 1] == '~') {
        // This ends with ~
        int number = atoi(extended + 1);
        switch (number) {
        case 2:
          return XKEY_INSERT; // terminal
        case 3:
          return XKEY_DELETE; // terminal
        case 5:
          return XKEY_PGUP; // terminal
        case 6:
          return XKEY_PGDN; // terminal
        case 15:
          return XKEY_F5; // terminal
        case 17:
          return XKEY_F6; // terminal
        case 18:
          return XKEY_F7; // terminal
        case 19:
          return XKEY_F8; // terminal
        case 20:
          return XKEY_F9; // terminal
        case 21:
          return XKEY_F10; // terminal
        case 23:
          return XKEY_F11;
        case 24:
          return XKEY_F12; // terminal
        }
      }
    }

    if (extended[0] == 'O') {
      switch (extended[1]) {
      case 'P':
        return XKEY_F1;
      case 'Q':
        return XKEY_F2;
      case 'R':
        return XKEY_F3;
      case 'S':
        return XKEY_F4;
      case 't':
        return XKEY_F5; // syncterm
      }
    }

    // unknown -- display debug output
    std::cout << CRNL "DEBUG:" CRNL "ESC + ";
    for (int x = 0; x < pos; x++) {
      char z = extended[x];
      if (iscntrl(z)) {
        std::cout << (int)z << " ";
      } else {
        std::cout << "'" << (char)z << "'"
                  << " ";
      };
    }

    return XKEY_UNKNOWN;
  }
  return c;
}
