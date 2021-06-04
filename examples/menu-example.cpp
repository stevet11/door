#include "door.h"

int main(int argc, char *argv[]) {
  door::Door door("example", argc, argv);

  // reset colors, clear screen.
  door << door::reset << door::cls << door::nl;

  door::Menu menu(10, 5, 25);
  menu.setColor(door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                                door::ATTR::BOLD)); // set border
  menu.setTitle(std::make_unique<door::Line>("Menu Example"), 1);

  int arrows = 15;
  door::updateFunction arrowsUpdate = [&arrows](void) -> std::string {
    std::string text = "Arrows (";
    text += std::to_string(arrows);
    text += " in stock)";
    return text;
  };
  menu.addSelection('A', "Arrows (0 in stock)", arrowsUpdate);
  menu.addSelection('C', "Color selection");
  menu.addSelection('P', "Pants");
  menu.addSelection('Q', "Quit");

  int r = 0;

  while (r >= 0) {
    menu.update();

    r = menu.choose(door);
    if (r > 0) {
      // we did not timeout
      char c = menu.which(r - 1);
      if (c == 'A') {
        if (arrows > 0)
          --arrows;
        else {
          door << door::Goto(1, 17) << door::reset
               << "Sorry, we're out of stock.";
          continue;
        }
      }

      door << door::Goto(1, 17) << door::reset << "You chose " << c << "!";
      if (c == 'Q')
        break;
    }
  }

  door << door::nl << "Returing to BBS... " << door::nl;
}
