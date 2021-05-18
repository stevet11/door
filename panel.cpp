#include "door.h"
#include <set>
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

Panel::Panel(int panelWidth) : border_color() {
  x = 0;
  y = 0;
  width = panelWidth;
  hidden = false;
  border_style = BorderStyle::NONE;
}

Panel::Panel(Panel &&ref) {
  x = ref.x;
  y = ref.y;
  width = ref.width;
  hidden = ref.hidden;
  border_style = ref.border_style;
  title = std::move(ref.title);
  offset = ref.offset;
  lines = std::move(ref.lines);
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
void Panel::addLine(std::unique_ptr<Line> l) {
  l->fit();
  lines.push_back(std::move(l));
}
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

bool Panel::update(Door &d) {
  int row = y;
  int style = (int)border_style;
  if (style > 0)
    ++row;

  bool updated = false;

  for (auto &line : lines) {
    if (line->update()) {
      /*
      std::string output = d.previous.debug();
      d.log(output);

      output = "update():";
      output.append(line->debug());
      d.log(output);
      */
      updated = true;
      int col = x;
      if (style > 0)
        ++col;
      d << door::Goto(col, row);
      d << *line;
    }
    ++row;
  }
  return updated;
}

void Panel::update(Door &d, int line) {
  int row = y;
  int style = (int)border_style;
  if (style > 0)
    ++row;

  // ok, I have the line number to update.
  auto &l = lines[line];
  row += line;

  int col = x;
  if (style > 0)
    ++col;
  d << door::Goto(col, row);
  d << *l;
}

void Panel::update(void) {
  for (auto &line : lines) {
    line->update();
  }
}

door::Goto Panel::gotoEnd(void) {
  int row = y;
  int style = (int)border_style;
  if (style > 0)
    ++row;
  row += lines.size();
  int col = x;
  if (style > 0)
    col += 2;
  col += width;
  return door::Goto(col, row);
}

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

std::function<void(Door &d, std::string &)> Menu::defaultUnselectedColorizer
= makeColorizer(ANSIColor(COLOR::WHITE, COLOR::BLUE, ATTR::BOLD),
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
                                   Color(Colors::BLUE, Colors::WHITE, 0)));
   */
  setRender(false, defaultUnselectedRender);
  // setColorizer(false, defaultUnselectedColorizer);
  /* makeColorizer(Color(Colors::LWHITE, Colors::BLUE, 0),
                                    Color(Colors::LWHITE, Colors::BLUE),
                                    Color(Colors::LWHITE, Colors::BLUE, 0),
                                    Color(Colors::LYELLOW, Colors::BLUE)));
   */
  chosen = 0;
}

Menu::Menu(int width) : Panel(width) {
  setStyle(BorderStyle::DOUBLE);
  setRender(true, defaultSelectedRender);
  setRender(false, defaultUnselectedRender);
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

char Menu::which(int d) { return options[d]; }

/*
void Menu::setColorizer(bool selected,
                        std::function<void(Door &d, std::string &)>
colorizer) { if (selected) selectedColorizer = colorizer; else
unselectedColorizer = colorizer;
}
*/

void Menu::setRender(bool selected, renderFunction render) {
  if (selected)
    selectedRender = render;
  else
    unselectedRender = render;
}

/**
 * make Render function for menus
 *
 * [O] Menu Item Text
 *
 * "[" and "]" are in c1, the "O" in c2
 *
 * "Menu Item Text" upper case letters are in c3, everything else c4.
 */
renderFunction Menu::makeRender(ANSIColor c1, ANSIColor c2, ANSIColor c3,
                                ANSIColor c4) {
  renderFunction render = [c1, c2, c3, c4](const std::string &txt) -> Render {
    Render r(txt);

    bool option = true;
    for (char const &c : txt) {
      if (option) {
        if (c == '[' or c == ']') {
          r.append(c1);
          option = (c == '[');
        } else {
          r.append(c2);
        }
      } else {
        if (isupper(c))
          r.append(c3);
        else
          r.append(c4);
      }
    }

    return r;
  };
  return render;
}

/*
  Should this return the index number, or
  the actual option?
 */

/**
 * @todo Fix this, so it only updates the lines that have been changed when
 * the user selects something.  Also, add the "Up/Down Move" maybe to the
 * bottom?
 *
 * Needs timeout.
 *
 * Should we return the index offset, or return the actual char?  (Like in
 * the case of Quit or Help?)
 *
 * @param door
 * @return int
 */
int Menu::choose(Door &door) {
  // Display menu and make a choice
  // step 1:  fix up the lines

  bool updated = true;
  bool update_and_exit = false;
  std::set<int> changed;

  while (true) {
    if (updated) {
      for (unsigned int x = 0; x < lines.size(); ++x) {
        if (x == chosen) {
          lines[x]->setRender(
              selectedRender); // setColorize(selectedColorizer);
        } else {
          lines[x]->setRender(
              unselectedRender); // setColorize(unselectedColorizer);
        }
      }
      // this outputs the entire menu
      if (changed.empty())
        door << *this;
      else {
        // update just the lines that have changed.
        for (auto si : changed) {
          // *this->update(door, si);
          update(door, si);
        }
        // Cursor is positioned at the end of the panel/menu.
        // The cursor changes colors as you arrow up or down.
        // Interesting!
        door << gotoEnd();
      }
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

    unsigned int previous_choice = chosen;
    changed.clear();

    bool use_numberpad = true;

    // Don't use the numberpad, if any of the options are 8 or 2 (up/down)
    for (const char &c : options) {
      if ((c == '8') or (c == '2'))
        use_numberpad = false;
    }

    switch (event) {
    case '8':
      if (!use_numberpad)
        break;
    case XKEY_UP_ARROW:
      if (chosen > 0) {
        chosen--;
        updated = true;
      }
      break;

    case '2':
      if (!use_numberpad)
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
    if (event == 0x0d) {
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

    if (previous_choice != chosen) {
      changed.insert(previous_choice);
      changed.insert(chosen);
    }
  }

  return 0;
}

Screen::Screen(){}; // hidden = false; }

/*
Screen::Screen(Screen &s) {
  hidden = s.hidden;
  parts = s.parts;
}
*/

void Screen::addPanel(std::unique_ptr<Panel> p) {
  panels.push_back(std::move(p));
}

/*
void Screen::hide(void) { hidden = true; }
void Screen::show(void) { hidden = false; }
*/

bool Screen::update(Door &d) {
  bool updated = false;
  for (auto &panel : panels) {
    if (panel->update(d))
      updated = true;
  }
  return updated;
}

void Screen::update(void) {
  for (auto &panel : panels) {
    panel->update();
  }
}

std::ostream &operator<<(std::ostream &os, const Screen &s) {
  // if (!s.hidden) {
  for (auto &panel : s.panels) {
    os << *panel;
  };
  // os << flush;
  // }
  return os;
}

} // namespace door
