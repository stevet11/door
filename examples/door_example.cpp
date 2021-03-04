#include "door.h"

int main(int argc, char *argv[]) {
  door::Door door(argc, argv);

  // reset colors, clear screen.
  door << door::reset << door::cls << door::nl;

  // set Yellow on Blue
  door << door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                          door::ATTR::BOLD);

  // display text, reset colors, NewLine.
  door << "Hello World" << door::reset << door::nl;

  od_get_key(TRUE); // Wait for a keypress before exiting
}
