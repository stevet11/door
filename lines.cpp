#include "door.h"
#include "utf8.h"

namespace door {

#ifdef EXPERIMENTAL

BasicLine::BasicLine(std::string txt) : text{txt}, hasColor{false} {}

BasicLine::BasicLine(std::string txt, ANSIColor c)
    : text{txt}, hasColor{true}, color{c} {}

bool BasicLine::hasRender(void) {
  if (render)
    return true;
  return false;
}

void BasicLine::setText(std::string txt) { text = txt; }

void BasicLine::setColor(ANSIColor c) {
  color = c;
  hasColor = true;
}

void BasicLine::setRender(renderFunction rf) { render = rf; }

void BasicLine::setUpdater(updateFunction uf) { updater = uf; }

/**
 * Update BasicLine, if we have an updater.
 *
 * If we have an updater, call it.  If the text is different,
 * update setText() and return true.
 * Otherwise false.
 *
 * This doesn't detect changes (like if the render has been changed, for
 * example)
 *
 * @return bool
 */
bool BasicLine::update(void) {
  if (updater) {
    std::string temp = updater();
    if (temp == text)
      return false;
    setText(temp);
    return true;
  }
  return false;
}

/**
 * Output Line
 *
 * This looks for padding and paddingColor.
 * This uses the render function if set.
 *
 * @param os std::ostream
 * @param l const BasicLine &
 * @return std::ostream&
 */
std::ostream &operator<<(std::ostream &os, const BasicLine &l) {
  if (l.render) {
    // This has a renderer.  Use it.
    Render r = l.render(l.text);
    r.output(os);
  } else {
    if (l.hasColor) {
      os << l.color;
    };
    os << l.text;
  }
  return os;
}

MultiLine::MultiLine(){};
void MultiLine::append(std::shared_ptr<BasicLine> bl) { lines.push_back(bl); }

bool MultiLine::update() {
  bool updated = false;

  for (auto line : lines) {
    if (line->update())
      updated = true;
  }
  return updated;
}

/**
 * Output Line
 *
 * This looks for padding and paddingColor.
 * This uses the render function if set.
 *
 * @param os std::ostream
 * @param ml const MultiLine &
 * @return std::ostream&
 */
std::ostream &operator<<(std::ostream &os, const MultiLine &ml) {
  for (auto line : ml.lines) {
    os << *line;
  }
  return os;
}

#endif

/**
 * Construct a new Line:: Line object with
 * string and total width.
 *
 * @param txt std::string
 * @param width int
 */
Line::Line(const std::string &txt, int w) : text{txt}, width{w} {
  hasColor = false;
}

Line::Line(const std::string &txt, int w, ANSIColor c)
    : text{txt}, color{c}, width{w} {

  hasColor = true;
}

Line::Line(const char *txt, int w, ANSIColor c)
    : text{txt}, color{c}, width{w} {
  hasColor = true;
}

Line::Line(const std::string &txt, int w, renderFunction rf)
    : text{txt}, render{rf}, width{w} {
  hasColor = false;
}

Line::Line(const char *txt, int w, renderFunction rf)
    : text{txt}, render{rf}, width{w} {
  hasColor = false;
}

/**
 * Construct a new Line:: Line object with
 * const char * and total width
 *
 * @param txt const char *
 * @param width int
 */
Line::Line(const char *txt, int w) : text{txt}, width{w} { hasColor = false; }

/**
 * Construct a new Line:: Line object from an
 * existing Line
 *
 * @param rhs const Line&
 */
Line::Line(const Line &rhs)
    : text{rhs.text}, hasColor{rhs.hasColor}, color{rhs.color},
      padding{rhs.padding}, paddingColor{rhs.paddingColor} {
  if (rhs.render) {
    render = rhs.render;
  }
  if (rhs.updater) {
    updater = rhs.updater;
  }
  width = rhs.width;
}

Line::Line(Line &&rhs)
    : text{rhs.text}, hasColor{rhs.hasColor}, color{rhs.color},
      padding{rhs.padding}, paddingColor{rhs.paddingColor} {
  if (rhs.render)
    render = rhs.render;
  if (rhs.updater)
    updater = rhs.updater;
  width = rhs.width;
}

/**
 * Has a render function been set?
 *
 * @return bool
 */
bool Line::hasRender(void) {
  if (render) {
    return true;
  } else {
    return false;
  }
}

/**
 * Return total length of Line
 *
 * text.length + 2 * padding length
 *
 * @return int
 */
int Line::length(void) {
  if (!padding.empty()) {
    if (unicode) {
      return utf8::distance(padding.begin(), padding.end()) * 2 +
             utf8::distance(text.begin(), text.end());
    } else {
      return padding.length() * 2 + text.length();
    }
  }

  if (unicode) {
    return utf8::distance(text.begin(), text.end());
  } else {
    return text.length();
  }
}

/**
 * Make text the given width by padding string with spaces.
 *
 * @param width int
 */
void Line::fit(void) {
  int need;
  if (door::unicode)
    need = width - utf8::distance(text.begin(), text.end());
  else
    need = width - text.length();

  need -= padding.length() * 2;

  if (need > 0) {
    text.append(std::string(need, ' '));
  }
}

/**
 * Set Line text.
 * @param txt std::string
 */
void Line::setText(std::string &txt) { text = txt; }
/**
 * Set Line text.
 * @param txt const char *
 */
void Line::setText(const char *txt) { text = txt; }

/**
 * set padding (color and text)
 *
 * @param padstring std::string
 * @param padColor ANSIColor
 */
void Line::setPadding(std::string &padstring, ANSIColor padColor) {
  padding = padstring;
  paddingColor = padColor;
}

/**
 * set padding (color and text)
 *
 * @param padstring const char *
 * @param padColor ANSIColor
 */
void Line::setPadding(const char *padstring, ANSIColor padColor) {
  padding = padstring;
  paddingColor = padColor;
}

/**
 * set color
 *
 * @param c ANSIColor
 */
void Line::setColor(ANSIColor c) {
  color = c;
  hasColor = true;
}

/**
 * set render
 *
 * Set the renderFunction to use for this Line.  This
 * replaces the colorizer.
 * @param rf renderFunction
 */
void Line::setRender(renderFunction rf) { render = rf; }

/**
 * set updater function
 *
 * This can update the line text when called.
 * @todo Define an updateFunction.
 * @param newUpdater updateFunction
 */
void Line::setUpdater(updateFunction newUpdater) { updater = newUpdater; }

std::string Line::debug(void) {
  std::string desc;

  desc = "Line(";
  desc += text;
  desc += "): ";
  if (updater) {
    desc += "[U]";
  }
  if (render) {
    desc += "[R]";
  }
  return desc;
}
/**
 * Call updater, report if the text was actually changed.
 *
 * @return bool
 */
bool Line::update(void) {
  if (updater) {
    std::string newText = updater();
    // int line_len;
    int need;
    if (unicode) {
      // line_len = utf8::distance(text.begin(), text.end());
      need = width - utf8::distance(newText.begin(), newText.end());
    } else {
      // line_len = text.length();
      need = width - newText.length();
    }

    need -= padding.length() * 2;
    if (need > 0) {
      newText.append(std::string(need, ' '));
    }
    if (newText != text) {
      text = newText;
      return true;
    }
  }
  return false;
}

/**
 * Output Line
 *
 * This looks for padding and paddingColor.
 * This uses the render function if set.
 *
 * @param os std::ostream
 * @param l const Line &
 * @return std::ostream&
 */
std::ostream &operator<<(std::ostream &os, const Line &l) {
  // Door *d = dynamic_cast<Door *>(&os);

  if (!l.padding.empty()) {
    os << l.paddingColor << l.padding;
  }
  if (l.render) {
    // This has a renderer.  Use it.
    Render r = l.render(l.text);
    r.output(os);
  } else {
    if (l.hasColor) {
      os << l.color;
    };
    os << l.text;
  }
  if (!l.padding.empty()) {
    os << l.paddingColor << l.padding;
  }

  return os;
}

} // namespace door