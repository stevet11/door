# Door++

## Wecome to **Door++**, a modern BBS Door development kit written in C++

### Getting Started

Clone the door++ project into a sub-directory of your project.
In your project CmakeLists.txt file, add ```add_subdirectory(door++)```.
Under the add_executable(your-door your-door.cpp) line, add ``target_link_libraries(your-door door++ pthread)```.

In main, create the door instance:

```cpp
#include "door.h"

int main( int argc, char * argv[] ) {
    door::Door door("your-door", argc, argv);

    door << "Welcome to Door++" << door::nl;
}
```

### Advanced Features of Door++

* door::Line

A line is text that can be updated, and can be colorized by the use of a rendering function.

If you want all uppercase letters one color, and lowercase another.  That can be done.

```cpp
/*
 * Custom line rendering fuction.
 * This allows for the status to be one color, and the value to be another.
 * "Score: 500"  
 * "Score:" would be status color, "500" would be value color.
 */
door::renderFunction statusValue(door::ANSIColor status,
                                 door::ANSIColor value) {
  door::renderFunction rf = [status,
                             value](const std::string &txt) -> door::Render {
    door::Render r(txt);
    size_t pos = txt.find(':');
    if (pos == std::string::npos) {
      for (char const &c : txt) {
        if (std::isdigit(c))
          r.append(value);
        else
          r.append(status);
      }
    } else {
      pos++;
      r.append(status, pos);
      r.append(value, txt.length() - pos);
    }
    return r;
  };
  return rf;
}

door::ANSIColor statusColor(door::COLOR::WHITE, door::COLOR::BLUE,
                            door::ATTR::BOLD);
door::ANSIColor valueColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                            door::ATTR::BOLD);
door::renderFunction svRender = statusValue(statusColor, valueColor);

// build the actual line here
std::unique_ptr<door::Line> scoreLine = std::make_unique<door::Line>("Score: 0", 50);
scoreLine->setRender(svRender);

// Make the scoreLine automatically update when score changes.
door::updateFunction scoreUpdate = [score](void) -> std::string {
    std::string text = "Score: ";
    text.append(std::to_string(score));
    return text;
};

scoreLine->setUpdater(scoreUpdate);
```

* door::Panel

A Panel is a group of lines with a known position.

```cpp
std::unique_ptr<door::Panel> panel = std::make_unique<door::Panel>(50);
panel->setStyle(door::BorderStyle::NONE);
// add lines to the panel
panel->addLines(std::move(scoreLine));

panel->set(5, 5);
panel->update();
door << panel;
```

* door::Menu

A Panel that displays options for the user to select

```cpp
// Define a menu starting at 5, 5 with width 25
door::Menu menu(5, 5, 25);

// Set border color
door::ANSIColor border_color(door::COLOR::CYAN, door::COLOR::BLUE);
m.setColor(border_color);

// Set the Menu Title
door::Line mtitle("Main Menu");
door::ANSIColor title_color(door::COLOR::CYAN, door::COLOR::BLUE, door::ATTR::BOLD);
mtitle.setColor(title_color);
mtitle.setPadding(" ", title_color);
m.setTitle(std::make_unique<door::Line>(mtitle), 1);

// Define colors for the menu
// menu line selected
m.setRender(true, door::Menu::makeRender(
                  door::ANSIColor(door::COLOR::CYAN, door::ATTR::BOLD),
                  door::ANSIColor(door::COLOR::BLUE, door::ATTR::BOLD),
                  door::ANSIColor(door::COLOR::CYAN, door::ATTR::BOLD),
                  door::ANSIColor(door::COLOR::BLUE, door::ATTR::BOLD)));

// menu line unselected
m.setRender(false, door::Menu::makeRender(
                   door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                   door::ATTR::BOLD),
                   door::ANSIColor(door::COLOR::WHITE, door::COLOR::BLUE,
                   door::ATTR::BOLD),
                   door::ANSIColor(door::COLOR::YELLOW, door::COLOR::BLUE,
                   door::ATTR::BOLD),
                   door::ANSIColor(door::COLOR::CYAN, door::COLOR::BLUE,
                   door::ATTR::BOLD)));

// Build the menu
// First char is [ char ], followed by the text.
// The arrow keys can be used to select the menu option, or
// hitting the character.

m.addSelection('P', "Play Cards");
m.addSelection('S', "View Scores");
m.addSelection('C', "Configure");
m.addSelection('H', "Help");
m.addSelection('A', "About this game");
m.addSelection('Q', "Quit");

int r;

// Render the menu and prompt for input
r = m.choose(door);

if (r < 0) {
    // timeout or out of time
}

if ( r == 1 ) {
    // Play Cards
}
...
```

