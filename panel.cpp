#include "door.h"
#include <string.h>

// #include <memory>

namespace door {

Panel::Panel(int xp, int yp, int panelWidth) : border_color() {
  x = xp;
  y = yp;
  width = panelWidth;
  hidden = false;
  border_style = BorderStyle::NONE;
  // border_color = ANSIColor();
}

Panel::Panel(Panel &&ref) {
  x = ref.x;
  y = ref.y;
  width = ref.width;
  hidden = ref.hidden;
  border_style = ref.border_style;
  title = std::move(ref.title);
  offset = ref.offset;
  lines = std::move(lines);
}

/*
Panel::Panel(const Panel &original) : border_color(original.border_color) {
  x = original.x;
  y = original.y;
  width = original.width;
  hidden = original.hidden;
  border_style = original.border_style;
  // door::Line title_copy(*original.title);
  // title = std::move(std::make_unique<door::Line>(title_copy));

  // What's wrong with making copies of unique_ptr objects?
  title = std::move(std::make_unique<door::Line>(*original.title));
  offset = original.offset;

  for (auto &line : original.lines) {
    // door::Line l(*line);
    // lines.push_back(std::move(std::make_unique<door::Line>(l)));
    // door::Line l(*line);
    lines.push_back(std::move(std::make_unique<door::Line>(*line)));
  }
}
*/

void Panel::set(int xp, int yp) {
  x = xp;
  y = yp;
}

void Panel::setTitle(std::unique_ptr<Line> t, int off) {
  title = std::move(t);
  offset = off;
}

void Panel::setStyle(BorderStyle bs) { border_style = bs; }
// Panel::Panel(Panel &old) = { }
void Panel::setColor(ANSIColor c) { border_color = c; }

void Panel::hide(void) { hidden = true; }
void Panel::show(void) { hidden = false; }
void Panel::addLine(std::unique_ptr<Line> l) { lines.push_back(std::move(l)); }
// or possibly std::move(l)); }

/*
bool Panel::delLine(std::shared_ptr<Line> l) {
  size_t size = lines.size();
  remove(lines.begin(), lines.end(), l);
  return (size != lines.size());
}
*/

/**
 * Utility structure that stores the characters needed to
 * render boxes.
 *
 * We assume that Top can be used in Bottom and Middle.
 * We assume that Side can be used on Left and Right.
 */
struct box_styles {
  /// Top Left
  const char *tl;
  /// Top Right
  const char *tr;
  /// Top
  const char *top;
  /// Side
  const char *side;
  /// Bottom Left
  const char *bl;
  /// Bottom Right
  const char *br;
  /// Middle Left
  const char *ml;
  /// Middle Right
  const char *mr;
};

/**
 *
 * use https://en.wikipedia.org/wiki/Code_page_437 for translations between
 * CP437 and unicode symbols.
 *
 * This holds the characters needed to render the different box styles.
 * tl tr top side bl br ml mr
 */
struct box_styles UBOXES[] = {{"\u250c", "\u2510", "\u2500", "\u2502", "\u2514",
                               "\u2518", "\u251c", "\u2524"},
                              {"\u2554", "\u2557", "\u2550", "\u2551", "\u255a",
                               "\u255d", "\u2560", "\u2563"},
                              {"\u2553", "\u2556", "\u2500", "\u2551", "\u2559",
                               "\u255c", "\u255f", "\u2562"},
                              {"\u2552", "\u2555", "\u2550", "\u2502", "\u2558",
                               "\u255b", "\u255e", "\u2561"}};

struct box_styles BOXES[] = {
    /*
            # ┌──┐
            # │  │
            # ├──┤
            # └──┘
     */
    {
        "\xda",
        "\xbf",
        "\xc4",
        "\xb3",
        "\xc0",
        "\xd9",
        "\xc3",
        "\xb4",
    },
    /*
            # ╔══╗
            # ║  ║
            # ╠══╣
            # ╚══╝
     */
    {
        "\xc9",
        "\xbb",
        "\xcd",
        "\xba",
        "\xc8",
        "\xbc",
        "\xcc",
        "\xb9",
    },
    /*
    # ╓──╖
    # ║  ║
    # ╟──╢
    # ╙──╜
     */
    {
        "\xd6",
        "\xb7",
        "\xc4",
        "\xba",
        "\xd3",
        "\xbd",
        "\xc7",
        "\xb6",
    },
    /*
            # ╒══╕
            # │  │
            # ╞══╡
            # ╘══╛
     */
    {
        "\xd5",
        "\xb8",
        "\xcd",
        "\xb3",
        "\xd4",
        "\xbe",
        "\xc6",
        "\xb5",
    },
};

/*
void Panel::display(void) {

}

void Panel::update(void) {

}
*/

// operator<< Panel is called to output the Menu.
// Menu has been massively changed to use Render instead of Colorizer.

std::ostream &operator<<(std::ostream &os, const Panel &p) {
  if (p.hidden)
    return os;

  // Handle borders
  int style = (int)p.border_style;
  struct box_styles s;

  // If there's no style, then everything works.
  // If I try style, it prints out first line
  // and dies.  (Yet it says there's 4 lines!)

  if (style > 0) {
    // Ok, there needs to be something in this style;
    if (style < 5) {
      if (unicode)
        s = UBOXES[style - 1];
      else
        s = BOXES[style - 1];
    } else {
      s.bl = s.br = s.mr = s.ml = " ";
      s.top = s.side = " ";
      s.tl = s.tr = " ";
    }
  }

  /*
    Door *d = dynamic_cast<Door *>(&os);
    if (d != nullptr) {
  */
  /*
    os << "style " << style << "."
       << " width " << p.width;
    os << " SIZE " << p.lines.size() << " ! " << nl;
  */

  // os << s.tl << s.top << s.tr << s.side << s.br << s.bl;

  int row = p.y;

  if (style > 0) {
    // Top line of border (if needed)
    os << door::Goto(p.x, row);
    os << p.border_color << s.tl;

    if (p.title) {
      for (int c = 0; c < p.offset; c++)
        os << s.top;
      os << *(p.title);
      os << p.border_color;
      int left = p.width - (p.offset + (p.title)->length());
      if (left > 0) {
        for (int c = 0; c < left; c++)
          os << s.top;
      };
      os << s.tr;
    } else {
      for (int c = 0; c < p.width; c++)
        os << s.top;
      os << s.tr;
    };
    // os << "";

    ++row;
  };

  for (auto &line : p.lines) {
    os << door::Goto(p.x, row);
    if (style > 0) {
      os << p.border_color << s.side;
    };

    // os << "[" << row << "," << p.x << "] ";
    os << *line;

    if (style > 0) {
      os << p.border_color << s.side;
    };

    // os << "row " << row;
    row++;
    // forcing reset works.  But why doesn't it work without this?
    // os << Color();
  }

  // Display bottom (if needed)
  if (style > 0) {
    os << door::Goto(p.x, row);
    os << p.border_color << s.bl;
    for (int c = 0; c < p.width; c++)
      os << s.top;
    // os << "";
    os << s.br;
  };
  // };
  // os << flush;
  return os;
} // namespace door

/*
std::function<void(Door &d, std::string &)> Menu::defaultSelectedColorizer =
    Menu::makeColorizer(ANSIColor(COLOR::BLUE, COLOR::WHITE),
                        ANSIColor(COLOR::BLUE, COLOR::WHITE),
                        ANSIColor(COLOR::BLUE, COLOR::WHITE),
                        ANSIColor(COLOR::BLUE, COLOR::WHITE));

std::function<void(Door &d, std::string &)> Menu::defaultUnselectedColorizer =
    makeColorizer(ANSIColor(COLOR::WHITE, COLOR::BLUE, ATTR::BOLD),
                  ANSIColor(COLOR::WHITE, COLOR::BLUE, ATTR::BOLD),
                  ANSIColor(COLOR::WHITE, COLOR::BLUE, ATTR::BOLD),
                  ANSIColor(COLOR::YELLOW, COLOR::BLUE, ATTR::BOLD));
*/

renderFunction Menu::defaultSelectedRender = Menu::makeRender(
    ANSIColor(COLOR::BLUE, COLOR::WHITE), ANSIColor(COLOR::BLUE, COLOR::WHITE),
    ANSIColor(COLOR::BLUE, COLOR::WHITE), ANSIColor(COLOR::BLUE, COLOR::WHITE));
renderFunction Menu::defaultUnselectedRender =
    Menu::makeRender(ANSIColor(COLOR::WHITE, COLOR::BLUE, ATTR::BOLD),
                     ANSIColor(COLOR::WHITE, COLOR::BLUE, ATTR::BOLD),
                     ANSIColor(COLOR::WHITE, COLOR::BLUE, ATTR::BOLD),
                     ANSIColor(COLOR::YELLOW, COLOR::BLUE, ATTR::BOLD));

Menu::Menu(int x, int y, int width) : Panel(x, y, width) {
  setStyle(BorderStyle::DOUBLE);
  // Setup initial sensible default values.
  // setColorizer(true, defaultSelectedColorizer);
  setRender(true, defaultSelectedRender);
  /* makeColorizer(Color(Colors::BLUE, Colors::WHITE, 0),
                                   Color(Colors::BLUE, Colors::WHITE, 0),
                                   Color(Colors::BLUE, Colors::WHITE, 0),
                                   Color(Colors::BLUE, Colors::WHITE, 0))); */
  setRender(false, defaultUnselectedRender);
  // setColorizer(false, defaultUnselectedColorizer);
  /* makeColorizer(Color(Colors::LWHITE, Colors::BLUE, 0),
                                    Color(Colors::LWHITE, Colors::BLUE),
                                    Color(Colors::LWHITE, Colors::BLUE, 0),
                                    Color(Colors::LYELLOW, Colors::BLUE))); */
  chosen = 0;
}

/*
Menu::Menu(const Menu &original)
    : Panel(original.x, original.y, original.width) {
  x = original.x;
  y = original.y;
  width = original.width;
  setStyle(original.border_style);
  setRender(true, original.selectedRender);
  setRender(false, original.unselectedRender);
  options = original.options;
  chosen = 0;
}
*/

Menu::Menu(Menu &&ref) : Panel(ref.x, ref.y, ref.width) {
  x = ref.x;
  y = ref.y;
  width = ref.width;
  border_style = ref.border_style;
  setRender(true, ref.selectedRender);
  setRender(false, ref.unselectedRender);
  options = ref.options;
  lines = std::move(ref.lines);
  chosen = ref.chosen;
}

void Menu::addSelection(char c, const char *line) {
  std::string menuline;
  menuline.reserve(5 + strlen(line));
  menuline = "[ ] ";
  menuline[1] = c;
  menuline += line;

  // problem:  How do I update the "Lines" from this point?
  // L->makeWidth(width);

  addLine(std::make_unique<Line>(menuline, width));
  options.push_back(c);
}

void Menu::defaultSelection(int d) { chosen = d; }

/*
void Menu::setColorizer(bool selected,
                        std::function<void(Door &d, std::string &)> colorizer) {
  if (selected)
    selectedColorizer = colorizer;
  else
    unselectedColorizer = colorizer;
}
*/

void Menu::setRender(bool selected, renderFunction render) {
  if (selected)
    selectedRender = render;
  else
    unselectedRender = render;
}

renderFunction Menu::makeRender(ANSIColor c1, ANSIColor c2, ANSIColor c3,
                                ANSIColor c4) {
  renderFunction render = [c1, c2, c3, c4](const std::string &txt) -> Render {
    Render r(txt);

    bool option = true;
    ColorOutput co;

    /*
      bool uc = true;
      ANSIColor blue(COLOR::BLUE, ATTR::BOLD);
      ANSIColor cyan(COLOR::YELLOW, ATTR::BOLD);
    */

    co.pos = 0;
    co.len = 0;
    co.c = c1;
    // d << blue;

    int tpos = 0;
    for (char const &c : txt) {
      if (option) {
        if (c == '[' or c == ']') {
          if (co.c != c1)
            if (co.len != 0) {
              r.outputs.push_back(co);
              co.reset();
              co.pos = tpos;
            }
          co.c = c1;
          if (c == ']')
            option = false;
        } else {
          if (co.c != c2)
            if (co.len != 0) {
              r.outputs.push_back(co);
              co.reset();
              co.pos = tpos;
            }
          co.c = c2;
        }
      } else {
        if (isupper(c)) {
          // possible color change
          if (co.c != c3)
            if (co.len != 0) {
              r.outputs.push_back(co);
              co.reset();
              co.pos = tpos;
            }
          co.c = c3;
        } else {
          if (co.c != c4)
            if (co.len != 0) {
              r.outputs.push_back(co);
              co.reset();
              co.pos = tpos;
            }
          co.c = c4;
        }
      }
      co.len++;
      tpos++;
    }
    if (co.len != 0) {
      r.outputs.push_back(co);
    }
    return r;
  };
  return render;
}

/*
std::function<void(Door &d, std::string &)>
Menu::makeColorizer(ANSIColor c1, ANSIColor c2, ANSIColor c3, ANSIColor c4) {
  std::function<void(Door & d, std::string & txt)> colorize =
      [c1, c2, c3, c4](Door &d, std::string txt) {
        bool option = true;
        for (char const &c : txt) {
          if (option) {
            if (c == '[' or c == ']') {
              d << c1 << c;
              if (c == ']')
                option = false;
            } else {
              d << c2 << c;
            }
          } else {
            if (isupper(c)) {
              d << c3 << c;
            } else {
              d << c4 << c;
            }
          }
        }
      };
  return colorize;
}
*/

/*
  Should this return the index number, or
  the actual option?
 */

/**
 * @todo Fix this, so it only updates the lines that have been changed when the
 * user selects something.  Also, add the "Up/Down Move" maybe to the bottom?
 *
 * Needs timeout.
 *
 * Should we return the index offset, or return the actual char?  (Like in the
 * case of Quit or Help?)
 *
 * @param door
 * @return int
 */
int Menu::choose(Door &door) {
  // Display menu and make a choice
  // step 1:  fix up the lines

  bool updated = true;
  bool update_and_exit = false;

  while (true) {
    if (updated) {
      for (unsigned int x = 0; x < lines.size(); x++) {

        if (x == chosen) {
          lines[x]->setRender(
              selectedRender); // setColorize(selectedColorizer);
        } else {
          lines[x]->setRender(
              unselectedRender); // setColorize(unselectedColorizer);
        }
      }
      // this outputs the entire menu
      door << *this;
      // door << flush;
      // door.update();
    };

    if (update_and_exit)
      return chosen + 1;

    // Maybe we want to position the cursor on the currently
    // selected item?

    updated = false;
    // Ok, when the option changes, can I control what gets updated??!?
    int event = door.sleep_key(door.inactivity);

    if (event < 0) {
      // timeout!
      return event;
    }

    // od_get_input(&event, OD_NO_TIMEOUT, GETIN_NORMAL);

    if (event > XKEY_START) {
      switch (event) {
      case XKEY_UP_ARROW:
        if (chosen > 0) {
          chosen--;
          updated = true;
        }
        break;
      case XKEY_DOWN_ARROW:
        if (chosen < lines.size() - 1) {
          chosen++;
          updated = true;
        }
        break;

      case XKEY_HOME:
        if (chosen != 0) {
          chosen = 0;
          updated = true;
        }
        break;
      case XKEY_END:
        if (chosen != lines.size() - 1) {
          chosen = lines.size() - 1;
          updated = true;
        }
      }
    } else if (event == 0x0d) {
      // ENTER -- use current selection
      return chosen + 1;
    }
    for (unsigned int x = 0; x < lines.size(); x++) {
      if (toupper(options[x]) == toupper(event)) {
        // is the selected one current chosen?

        if (chosen == x) {
          return x + 1;
        }
        // No, it isn't!
        // Update the screen, and then exit
        updated = true;
        chosen = x;
        update_and_exit = true;
      }
    }
  }

  return 0;
}

Screen::Screen() { hidden = false; }

/*
Screen::Screen(Screen &s) {
  hidden = s.hidden;
  parts = s.parts;
}
*/

void Screen::addPanel(std::shared_ptr<Panel> p) { parts.push_back(p); }

void Screen::hide(void) { hidden = true; }
void Screen::show(void) { hidden = false; }

std::ostream &operator<<(std::ostream &os, const Screen &s) {
  if (!s.hidden) {
    for (auto part : s.parts) {
      os << part;
    };
    // os << flush;
  }
  return os;
}

} // namespace door