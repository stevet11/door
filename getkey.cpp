#include <iostream>

// raw mode
#include <termios.h>
#include <unistd.h>

#include "door.h"

// let's try this!
#include <signal.h>
#include <unistd.h>

/**
 * @file
 * @brief Key and door input routines
 */

/*
void done(int signal) {
  std::cout << "\r\nWORP WORP\r\n";
  std::cout.flush();
}
*/

#include <ctype.h>

/**
 * @brief Original terminal termios defaults.
 */
struct termios tio_default;

/**
 * @brief Enable terminal raw mode.
 *
 * This sets up the linux console so the door library will work correctly in
 * local mode.
 */
void raw(void) {
  // enable terminal RAW mode
  struct termios tio_raw;
  tcgetattr(STDIN_FILENO, &tio_default);
  tio_raw = tio_default;
  cfmakeraw(&tio_raw);

  // Ok!  I need the extra sauce here

  tio_raw.c_cc[VMIN] = 0;
  tio_raw.c_cc[VTIME] = 1;

  // tio_raw.c_iflag &= ~(ICRNL | IXON);

  tcsetattr(STDIN_FILENO, TCSANOW, &tio_raw);
}

/**
 * @brief Reset the terminal termios to the original values.
 */
void reset(void) { tcsetattr(STDIN_FILENO, TCOFLUSH, &tio_default); }

/**
 * @brief used by output routines.
 *
 * Sending "\n" isn't enough.
 */
#define CRNL "\r\n"

/*
NOTE:  cr (from syncterm), gives 0x0d 0x00
 */

/**
 * @brief low level getch key read
 *
 * This reads a key with a defined timeout value.
 * This is called by other routines to handle arrow keys, F-keys.
 * returns -1 on timeout (no key), -2 on error (connection closed)
 * @return signed int
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

/**
 * @brief pushback buffer to store keys we're not ready for yet.
 */
char buffer[10];
/**
 * @brief pushback buffer position 
 */
int bpos = 0;

/**
 * @brief ungets (pushes key back)
 *
 * If we read ahead, and we can't use it, we push it back into the buffer for
 * next time.
 * @param c
 */
void unget(char c) {
  if (bpos < sizeof(buffer) - 1) {
    buffer[bpos] = c;
    bpos++;
  }
}

/**
 * @brief get a key from the pushback buffer.
 * 
 * @return char 
 */
char get(void) {
  if (bpos == 0) {
    return 0;
  }
  bpos--;
  char c = buffer[bpos];
  return c;
}

/**
 * @brief high level getkey
 * 
 * This returns function keys, arrow keys, see XKEY_* defines.
 * returns -1 (no key avaiable) or -2 (hangup)
 * or XKEY_UNKNOWN (don't know what it is)
 * 
 * @return signed int 
 */
signed int getkey(void) {
  signed int c, c2;

  // consume pushback buffer before reading more keys.
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
#ifdef DEBUG_OUTPUT
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
