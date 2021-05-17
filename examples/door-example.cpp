#include "door.h"

int main(int argc, char *argv[]) {
  door::Door door("example", argc, argv);

  // reset colors, clear screen.
  door << door::reset << door::cls << door::nl;

  // set Yellow on Blue
  door << door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                          door::ATTR::BOLD);

  // display text, reset colors, NewLine.
  door << "Welcome YELLOW on BLUE!  Press a key to continue... " << door::reset
       << door::nl;

  door.sleep_key(door.inactivity); // Wait for a keypress before exiting
}
