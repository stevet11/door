#include "door.h"

#include "utf8.h"
#include <iomanip>
#include <iostream>

namespace door {

Bar::Bar(int width, BarStyle s) : text(' ', width), line(text) {
  length = width;
  style = s;
  // set updater
  // set colorizer
  // profit
}

void Bar::update_bar(void) {
  unsigned long step_width;

  text.clear();

  switch (style) {
  case BarStyle::SOLID:
  case BarStyle::PERCENTAGE:
  case BarStyle::PERCENT_SPACE: {
    step_width = 100 * 100 / length;
    int steps = current_percent / step_width;
    // number of steps visible.

    // for now:
    if (door::unicode)
      for (int i = 0; i < steps; ++i)
        text.append("\u2588");
    else
      text.assign(steps, '\xdb');

    for (int x = steps; x < length; ++x)

      text.append(" ");

    // line.setText(text);
    /*
    cout << "percent " << std::setw(5) << current_percent << " : step_width "
         << std::setw(5) << step_width << std::setw(5) << steps << " of "
         << std::setw(5) << length << " ";
    */
    if ((style == BarStyle::PERCENTAGE) || (style == BarStyle::PERCENT_SPACE)) {
      // Put the % text in the text.
      std::string percent;
      percent = std::to_string(current_percent / 100);

      int pos = (length / 2) - 1;

      if (percent != "100") {
        percent.append(1, '%');
        if (percent.length() < 3)
          percent.insert(0, " ");
      }
      if (style == BarStyle::PERCENT_SPACE) {
        percent.insert(0, 1, ' ');
        percent.append(1, ' ');
        pos -= 1;
      }

      // cout << "[" << percent << "] " << std::setw(3) << pos << " ";

      // unicode ... is unipain.
      if (door::unicode) {
        // handle unicode here
        // Find start position, and ending position for the replacement.
        const char *cp = text.c_str();
        const char *end = cp;
        while (*end != 0)
          end++;

        for (int i = 0; i < pos; i++) {
          utf8::next(cp, end);
        }
        // find position using unicode position
        int unicode_pos = cp - text.c_str();

        const char *cp_len_start = cp;
        const char *cp_len = cp;
        for (int i = 0; i < (int)percent.length(); i++) {
          utf8::next(cp_len, end);
        }

        int unicode_len = cp_len - cp_len_start;
        // cout << " " << std::setw(3) << unicode_pos << " " << std::setw(3) <<
        // unicode_len << " ";
        text.replace(unicode_pos, unicode_len, percent);
      } else {
        text.replace(pos, percent.length(), percent);
      };
    }

    line.setText(text);
    break;
  };
  case BarStyle::HALF_STEP: {
    step_width = 100 * 100 / length;
    int steps = current_percent * 2 / step_width;

    /*
    cout << "percent " << std::setw(5) << current_percent << " : step_width "
         << std::setw(5) << step_width << std::setw(5) << steps << " of "
         << std::setw(5) << length << " ";
    */

    if (door::unicode)
      for (int i = 0; i < steps / 2; i++)
        text.append("\u2588");
    else
      text.assign(steps / 2, '\xdb');

    if (steps % 2 == 1) {
      if (door::unicode)
        text.append("\u258c");
      else
        text.append(1, '\xdd');
      steps++;
    }
    for (int x = steps; x < length * 2; x += 2)
      text.append(" ");
    line.setText(text);

    break;
  }

  case BarStyle::GRADIENT: {
    step_width = 100 * 100 / length;
    int steps = current_percent * 4 / step_width;

    /*
    cout << "percent " << std::setw(5) << current_percent << " : step_width "
         << std::setw(5) << step_width << std::setw(5) << steps << " of "
         << std::setw(5) << length << " ";
    */

    if (door::unicode)
      for (int i = 0; i < steps / 4; i++)
        text.append("\u2588");
    else
      text.assign(steps / 4, '\xdb');

    if (steps % 4 != 0) {
      // display the gradient
      switch (steps % 4) {
      case 1:
        if (door::unicode)
          text.append("\u2591");
        else
          text.append(1, '\xb0');
        break;

      case 2:
        if (door::unicode)
          text.append("\u2592");
        else
          text.append(1, '\xb1');
        break;

      case 3:
        if (door::unicode)
          text.append("\u2593");
        else
          text.append(1, '\xb2');
        break;
      }

      while (steps % 4 != 0)
        steps++;
    }
    for (int x = steps; x < length * 4; x += 4)
      text.append(" ");
    line.setText(text);

    break;
  }
  }
  // cout << "percent" << current_percent << " : " << steps << " of " << length
  // << " " << std::endl;
}

void Bar::set(float percent) {
  current_percent = (unsigned long)(percent * 100.0);
  update_bar();
}

void Bar::set(int value, int max) {
  // I'm thinking the ulong would be % * 100.
  unsigned long percentage = value * 10000 / max;
  // float percentage = ((float)pos / (float)max) * 100.0;
  set(percentage);
}

void Bar::set(unsigned long percent) {
  current_percent = percent;
  update_bar();
}

/**
 * Output Bar
 *
 * This outputs the Progress Bar's internal line.
 *
 * @param os std::ostream
 * @param b const Bar &
 * @return std::ostream&
 */
std::ostream &operator<<(std::ostream &os, const Bar &b) {
  os << b.line;
  // reset?
  os << door::reset;
  return os;
}

BarLine::BarLine(const std::string &txt, int width)
    : Line(txt, width), barstyle{BarStyle::SOLID},
      current_percent{0}, length{width} {
  init();
}

BarLine::BarLine(const char *txt, int width)
    : Line(txt, width), barstyle{BarStyle::SOLID},
      current_percent{0}, length{width} {
  init();
}

BarLine::BarLine(const std::string &txt, int width, ANSIColor c)
    : Line(txt, width, c), barstyle{BarStyle::SOLID},
      current_percent{0}, length{width} {
  init();
}

BarLine::BarLine(const char *txt, int width, ANSIColor c)
    : Line(txt, width, c), barstyle{BarStyle::SOLID},
      current_percent{0}, length{width} {
  init();
}

void BarLine::init(void) {
  // set update function.
  door::updateFunction barLineUpdate = [this](void) -> std::string {
    if (!colorRange.empty()) {
      // Ok, there is a range, so test for it.
      ANSIColor ac;
      // unsigned long p;
      for (auto bc : colorRange) {
        if (current_percent <= bc.percent) {
          ac = bc.c;
          // p = bc.percent;
          break;
        };
      }
      // cout << "!" << current_percent << "," << p << " ";
      setColor(ac);
    }
    return this->update_bar();
  };
  setUpdater(barLineUpdate);
  update();
}

void BarLine::setStyle(BarStyle s) { barstyle = s; }
void BarLine::set(int value, int max) {
  unsigned long percentage = value * 100 * 100 / max;
  set(percentage);
}

void BarLine::set(float percent) {
  unsigned long percentage = (unsigned long)(percent * 100.0);
  set(percentage);
}

void BarLine::set(unsigned long percent) {
  current_percent = percent;
  update_bar();
}

std::string BarLine::update_bar(void) {
  unsigned long step_width;
  std::string btext;

  switch (barstyle) {
  case BarStyle::SOLID:
  case BarStyle::PERCENTAGE:
  case BarStyle::PERCENT_SPACE: {
    step_width = 100 * 100 / length;
    int steps = current_percent / step_width;
    // number of steps visible.

    // for now:
    if (door::unicode)
      for (int i = 0; i < steps; ++i)
        btext.append("\u2588");
    else
      btext.assign(steps, '\xdb');

    for (int x = steps; x < length; ++x)

      btext.append(" ");

    // line.setText(text);
    /*
    cout << "percent " << std::setw(5) << current_percent << " : step_width "
         << std::setw(5) << step_width << std::setw(5) << steps << " of "
         << std::setw(5) << length << " ";
    */
    if ((barstyle == BarStyle::PERCENTAGE) ||
        (barstyle == BarStyle::PERCENT_SPACE)) {
      // Put the % text in the text.
      std::string percent;
      percent = std::to_string(current_percent / 100);

      int pos = (length / 2) - 1;

      if (percent != "100") {
        percent.append(1, '%');
        if (percent.length() < 3)
          percent.insert(0, " ");
      }
      if (barstyle == BarStyle::PERCENT_SPACE) {
        percent.insert(0, 1, ' ');
        percent.append(1, ' ');
        pos -= 1;
      }

      // cout << "[" << percent << "] " << std::setw(3) << pos << " ";

      // unicode ... is unipain.
      if (door::unicode) {
        // handle unicode here
        // Find start position, and ending position for the replacement.
        const char *cp = btext.c_str();
        const char *end = cp;
        while (*end != 0)
          end++;

        for (int i = 0; i < pos; i++) {
          utf8::next(cp, end);
        }
        // find position using unicode position
        int unicode_pos = cp - btext.c_str();

        const char *cp_len_start = cp;
        const char *cp_len = cp;
        for (int i = 0; i < (int)percent.length(); i++) {
          utf8::next(cp_len, end);
        }

        int unicode_len = cp_len - cp_len_start;
        // cout << " " << std::setw(3) << unicode_pos << " " << std::setw(3) <<
        // unicode_len << " ";
        btext.replace(unicode_pos, unicode_len, percent);
      } else {
        btext.replace(pos, percent.length(), percent);
      };
    }

    return btext;
    break;
  };
  case BarStyle::HALF_STEP: {
    step_width = 100 * 100 / length;
    int steps = current_percent * 2 / step_width;

    /*
    cout << "percent " << std::setw(5) << current_percent << " : step_width "
         << std::setw(5) << step_width << std::setw(5) << steps << " of "
         << std::setw(5) << length << " ";
    */

    if (door::unicode)
      for (int i = 0; i < steps / 2; i++)
        btext.append("\u2588");
    else
      btext.assign(steps / 2, '\xdb');

    if (steps % 2 == 1) {
      if (door::unicode)
        btext.append("\u258c");
      else
        btext.append(1, '\xdd');
      steps++;
    }
    for (int x = steps; x < length * 2; x += 2)
      btext.append(" ");

    return btext;
    break;
  }

  case BarStyle::GRADIENT: {
    step_width = 100 * 100 / length;
    int steps = current_percent * 4 / step_width;

    /*
    cout << "percent " << std::setw(5) << current_percent << " : step_width "
         << std::setw(5) << step_width << std::setw(5) << steps << " of "
         << std::setw(5) << length << " ";
    */

    if (door::unicode)
      for (int i = 0; i < steps / 4; i++)
        btext.append("\u2588");
    else
      btext.assign(steps / 4, '\xdb');

    if (steps % 4 != 0) {
      // display the gradient
      switch (steps % 4) {
      case 1:
        if (door::unicode)
          btext.append("\u2591");
        else
          btext.append(1, '\xb0');
        break;

      case 2:
        if (door::unicode)
          btext.append("\u2592");
        else
          btext.append(1, '\xb1');
        break;

      case 3:
        if (door::unicode)
          btext.append("\u2593");
        else
          btext.append(1, '\xb2');
        break;
      }

      while (steps % 4 != 0)
        steps++;
    }
    for (int x = steps; x < length * 4; x += 4)
      btext.append(" ");

    return btext;
    break;
  }
  }
  // cout << "percent" << current_percent << " : " << steps << " of " << length
  // << " " << std::endl;
  return btext;
}

void BarLine::setColorRange(vector<BarColorRange> bcr) { colorRange = bcr; }

} // namespace door
