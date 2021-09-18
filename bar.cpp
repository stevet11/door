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
        for (int i = 0; i < percent.length(); i++) {
          utf8::next(cp_len, end);
        }

        int unicode_len = cp_len - cp_len_start;
        // cout << " " << std::setw(3) << unicode_pos << " " << std::setw(3) << unicode_len << " ";
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
      /*
      if (door::unicode)
        text.append("\u258c");
      else
        text.append(1, '\xdd');
        */

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

} // namespace door
