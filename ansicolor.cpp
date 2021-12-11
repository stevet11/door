#include <string>

#include "door.h"

/**
 * @file
 * @brief ANSIColor
 */

namespace door {

/**
 * Construct a new ANSIColor::ANSIColor object
 * with sensible defaults (White on Black).
 *
 */
ANSIColor::ANSIColor() : fg(COLOR::WHITE), bg(COLOR::BLACK), attr(0) {}

/**
 * Construct a new ANSIColor::ANSIColor object
 * with attribute set.
 *
 * @param[in] a ATTR
 */
ANSIColor::ANSIColor(ATTR a) : ANSIColor() { Attr(a); }

/**
 * Construct a new ANSIColor::ANSIColor object
 * with a foreground color.
 *
 * @param[in] f COLOR
 */
ANSIColor::ANSIColor(COLOR f) : ANSIColor() { fg = f; }

/**
 * Construct a new ANSIColor::ANSIColor object
 * with a foreground color and attribute.
 *
 * @param[in] f COLOR
 * @param[in] a ATTR
 */
ANSIColor::ANSIColor(COLOR f, ATTR a) : ANSIColor() {
  fg = f;
  Attr(a);
}

/**
 * Construct a new ANSIColor::ANSIColor object
 * with a foreground color and attributes.
 *
 * @param[in] f COLOR
 * @param[in] a1 ATTR
 * @param[in] a2 ATTR
 */
ANSIColor::ANSIColor(COLOR f, ATTR a1, ATTR a2) : ANSIColor() {
  fg = f;
  Attr(a1);
  Attr(a2);
}

/**
 * Construct a new ANSIColor::ANSIColor object
 * with a foreground and background color.
 *
 * @param[in] f foreground COLOR
 * @param[in] b background COLOR
 */
ANSIColor::ANSIColor(COLOR f, COLOR b) : ANSIColor() {
  fg = f;
  bg = b;
}

/**
 * Construct a new ANSIColor::ANSIColor object
 * with a foreground color, background color,
 * and attribute.
 *
 * @param[in] f foreground COLOR
 * @param[in] b background COLOR
 * @param[in] a ATTR
 */
ANSIColor::ANSIColor(COLOR f, COLOR b, ATTR a) : ANSIColor() {
  fg = f;
  bg = b;
  Attr(a);
}

/**
 * Construct a new ANSIColor::ANSIColor object
 * with foreground, background color and attributes.
 *
 * @param[in] f foreground COLOR
 * @param[in] b background COLOR
 * @param[in] a1 ATTR
 * @param[in] a2 ATTR
 */
ANSIColor::ANSIColor(COLOR f, COLOR b, ATTR a1, ATTR a2) : ANSIColor() {
  fg = f;
  bg = b;
  Attr(a1);
  Attr(a2);
}

/**
 * Set attribute.  We return the object so
 * calls can be chained.
 *
 * @param[in] a ATTR
 * @return ANSIColor&
 */
ANSIColor &ANSIColor::Attr(ATTR a) {
  switch (a) {
    case ATTR::RESET:
      attr |= ATTR_RESET;
      break;
    case ATTR::BOLD:
      attr |= ATTR_BOLD;
      break;
    case ATTR::BLINK:
      attr |= ATTR_BLINK;
      break;
    case ATTR::INVERSE:
      attr |= ATTR_INVERSE;
      break;
  }
  return *this;
}

/**
 * Equal operator.
 *
 * This compares colors and attributes, but ignores reset.
 *
 * @param[in] c const ANSIColor &
 * @return bool
 */
bool ANSIColor::operator==(const ANSIColor &c) const {
  return ((fg == c.fg) and (bg == c.bg) and
          ((attr & (ATTR_BOLD | ATTR_BLINK | ATTR_INVERSE)) ==
           ((c.attr & (ATTR_BOLD | ATTR_BLINK | ATTR_INVERSE)))));
}

/**
 * Not-equal operator.
 *
 * This compares colors and attributes, but ignores reset.
 *
 * @param[in] c const ANSIColor &
 * @return bool
 */
bool ANSIColor::operator!=(const ANSIColor &c) const {
  return !((fg == c.fg) and (bg == c.bg) and
           ((attr & (ATTR_BOLD | ATTR_BLINK | ATTR_INVERSE)) ==
            ((c.attr & (ATTR_BOLD | ATTR_BLINK | ATTR_INVERSE)))));
}

/**
 * @brief Set foreground color
 *
 * @param[in] f foreground COLOR
 */
void ANSIColor::setFg(COLOR f) {
  fg = f;
  attr = 0;
}

/**
 * @brief Set foreground color and attribute
 *
 * @param[in] f foreground COLOR
 * @param[in] a ATTR
 */
void ANSIColor::setFg(COLOR f, ATTR a) {
  fg = f;
  setAttr(a);
}

/**
 * @brief Set background color
 *
 * @param[in] b background COLOR
 */
void ANSIColor::setBg(COLOR b) { bg = b; }

/**
 * @brief Set attribute
 *
 * This clears all the attributes before setting the selected ATTR.
 *
 * @param[in] a ATTR
 */
void ANSIColor::setAttr(ATTR a) {
  // first, clear all attributes
  attr = 0;
  Attr(a);
}

/**
 * Output the full ANSI codes for attributes and color.
 * This does not look at the previous values.
 */
std::string ANSIColor::output(void) const {
  std::string clr(CSI);

  // check for special cases
  if (((attr & ATTR_RESET) == ATTR_RESET) and (fg == COLOR::BLACK) and
      (bg == COLOR::BLACK)) {
    clr += "0m";
    return clr;
  }

  if (((attr & ATTR_RESET) == ATTR_RESET) and (fg == COLOR::WHITE) and
      (bg == COLOR::BLACK)) {
    clr += "0m";
    return clr;
  }

  if ((attr & ATTR_RESET) == ATTR_RESET) {
    clr += "0;";
  }

  if ((attr & ATTR_BOLD) == ATTR_BOLD) {
    if ((attr & ATTR_BLINK) == ATTR_BLINK) {
      clr += "5;";
    }
    clr += "1;";
  } else {
    if (!((attr & ATTR_RESET) == ATTR_RESET)) clr += "0;";
    if ((attr & ATTR_BLINK) == ATTR_BLINK) {
      clr += "5;";
    }
  }

  clr += std::to_string(30 + (int)fg) + ";";
  clr += std::to_string(40 + (int)bg) + "m";

  return clr;
}

/**
 * @brief Output debug string for ANSIColor
 *
 * @return std::string
 */
std::string ANSIColor::debug(void) {
  std::string output;
  output = "ANSIColor FG";
  output += std::to_string((int)fg);
  output += ", BG";
  output += std::to_string((int)bg);
  output += ", B";
  output += std::to_string((attr & ATTR_BOLD) == ATTR_BOLD);
  output += ", R";
  output += std::to_string((attr & ATTR_RESET) == ATTR_RESET);

  return output;
}

/**
 * Output only what ANSI attributes and colors have changed.
 * This uses the previous ANSIColor value to determine what
 * has changed.
 *
 * This sets previous to the current upon completion.
 */
std::string ANSIColor::output(ANSIColor &previous) const {
  std::string clr(CSI);
  // color output optimization

  // check for special cases
  if (((attr & ATTR_RESET) == ATTR_RESET) and (fg == COLOR::BLACK) and
      (bg == COLOR::BLACK)) {
    clr += "0m";
    previous = *this;
    previous.attr &= ~ATTR_RESET;  // reset = 0;
    return clr;
  }

  bool temp_reset = false;
  if ((!((attr & ATTR_BLINK) == ATTR_BLINK)) and
      (((attr & ATTR_BLINK) == ATTR_BLINK) !=
       ((previous.attr & ATTR_BLINK) == ATTR_BLINK))) {
    temp_reset = true;
  }

  if (((attr & ATTR_RESET) == ATTR_RESET) or (temp_reset)) {
    // current has RESET, so default to always sending colors.
    if (temp_reset) {
      clr += "0m";
    }

    // this fixes the extra \x1b that shows up with reset.
    if (clr.compare(CSI) == 0) clr.clear();
    clr += output();

    /*
        std::ofstream logf;
        logf.open("ansicolor.log", std::ofstream::out | std::ofstream::app);
        logf << "clr = [" << clr << "]" << std::endl;
        logf.close();
    */

    previous = *this;
    previous.attr &= ~ATTR_RESET;
    return clr;
  }

  if (*this == previous) {
    clr.clear();
    return clr;
  }

  // resume "optimization"

  if (((attr & ATTR_BOLD) == ATTR_BOLD) !=
      ((previous.attr & ATTR_BOLD) == ATTR_BOLD)) {
    // not the same, so handle this.
    if ((attr & ATTR_BOLD) == ATTR_BOLD) {
      if ((attr & ATTR_BLINK) == ATTR_BLINK) {
        clr += "5;";
      }
      clr += "1;";
      if (fg != previous.fg) clr += std::to_string((int)fg + 30) + ";";
      if (bg != previous.bg) clr += std::to_string((int)bg + 40) + ";";
    } else {
      clr += "0;";
      if ((attr & ATTR_BLINK) == ATTR_BLINK) {
        clr += "5;";
      }

      // RESET to turn OFF BOLD, clears previous
      if (fg != COLOR::WHITE) clr += std::to_string((int)fg + 30) + ";";
      if (bg != COLOR::BLACK) clr += std::to_string((int)bg + 40) + ";";
    }
  } else {
    // not bold.
    if ((attr & ATTR_BLINK) == ATTR_BLINK) {
      clr += "5;";
    }
    if (fg != previous.fg) clr += std::to_string((int)fg + 30) + ";";
    if (bg != previous.bg) clr += std::to_string((int)bg + 40) + ";";
  };

  // Remove ';' if last character
  std::string::iterator si = clr.end() - 1;
  if (*si == ';') {
    clr.erase(si);
  }

  if (clr.compare(CSI) == 0)
    clr.clear();
  else
    clr += "m";

  // final step, set previous to current and return the string;
  previous = *this;
  return clr;
};

/**
 * This converts ANSI \ref COLOR and \ref ATTR to ANSI codes
 * understood by the \ref Door output class.
 */
std::ostream &operator<<(std::ostream &os, const ANSIColor &c) {
  std::string out;

  Door *d = dynamic_cast<Door *>(&os);
  if (d != nullptr) {
    d->track = false;
    out = c.output(d->previous);
    // if (!out.empty())
    if (out.compare("\x1b[") == 0) std::abort();

    *d << out;
    d->track = true;
  } else {
    // "dumb" version that can't remember anything/ doesn't optimize color
    // output.
    std::string out = c.output();
    os << out;
  }
  return os;
}

ANSIColor reset(ATTR::RESET);

}  // namespace door