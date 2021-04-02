#ifndef DOOR_H
#define DOOR_H

#include "anyoption.h"
#include <cstdint>
#include <ctime>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <ostream>
#include <vector>

// raw mode
#include <termios.h>
#include <unistd.h>

#define CSI "\x1b["

// getkey definitions
#define XKEY_START 0x1000

#define XKEY_UP_ARROW 0x1001
#define XKEY_DOWN_ARROW 0x1002
#define XKEY_RIGHT_ARROW 0x1003
#define XKEY_LEFT_ARROW 0x1004

#define XKEY_HOME 0x1010
#define XKEY_END 0x1011
#define XKEY_PGUP 0x1012
#define XKEY_PGDN 0x1023
#define XKEY_INSERT 0x1024
#define XKEY_DELETE 0x7f

#define XKEY_F1 0x1021
#define XKEY_F2 0x1022
#define XKEY_F3 0x1023
#define XKEY_F4 0x1024
#define XKEY_F5 0x1025
#define XKEY_F6 0x1026
#define XKEY_F7 0x1027
#define XKEY_F8 0x1028
#define XKEY_F9 0x1029
#define XKEY_F10 0x102a
#define XKEY_F11 0x102b
#define XKEY_F12 0x102c

#define XKEY_UNKNOWN 0x1111

/**
 * @brief The BBS door project.
 * This is an attempt at writing a C++ BBS door toolkit.
 */

namespace door {

extern bool unicode;
extern bool full_cp437;
extern bool debug_capture;

/*
Translate CP437 strings to unicode for output.

if (door::unicode) {
  // perform translation
}

 */
void cp437toUnicode(std::string input, std::string &out);
void cp437toUnicode(const char *input, std::string &out);

/*
door 2.0
 */

/**
 * ANSI Color codes
 */

/**
 * @brief The colors available under ANSI-BBS
 */
enum class COLOR : std::int8_t {
  /// BLACK (0)
  BLACK,
  /// RED (1)
  RED,
  /// GREEN (2)
  GREEN,
  /// BROWN (3)
  BROWN,
  /// YELLOW (3)
  YELLOW = 3,
  /// BLUE (4)
  BLUE,
  /// MAGENTA (5)
  MAGENTA,
  /// CYAN (6)
  CYAN,
  /// WHITE (7)
  WHITE
};

/**
 * @brief ANSI-BBS text attributes
 */
enum class ATTR : std::int8_t {
  /// RESET forces all attributes (and Colors) to be sent.
  RESET,
  /// BOLD is the same as BRIGHT.
  BOLD,
  /// BRIGHT is the same as BOLD.
  BRIGHT = 1,
  /// SLOW BLINK
  BLINK = 5,
  /// INVERSE is Background on Foreground.
  INVERSE = 7
};

/**
 * @class ANSIColor
 * This holds foreground, background and ANSI-BBS attribute
 * information.
 * The special attribute RESET forces attribute and color
 * output always.
 *
 * @brief Foreground, Background and Attributes
 *
 */
class ANSIColor {
  /// Foreground color
  COLOR fg;
  /// Background color
  COLOR bg;
  // Track attributes (ATTR)
  /// reset flag / always send color and attributes
  unsigned int reset : 1;
  /// bold / bright flag
  unsigned int bold : 1;
  /// blink slow blinking text
  unsigned int blink : 1;
  /// inverse
  unsigned int inverse : 1;

public:
  // default initialization here
  ANSIColor();
  ANSIColor(ATTR a);
  ANSIColor(COLOR f);
  ANSIColor(COLOR f, ATTR a);
  ANSIColor(COLOR f, ATTR a1, ATTR a2);
  ANSIColor(COLOR f, COLOR b);
  ANSIColor(COLOR f, COLOR b, ATTR a);
  ANSIColor(COLOR f, COLOR b, ATTR a1, ATTR a2);
  ANSIColor &Attr(ATTR a);
  bool operator==(const ANSIColor &c) const;
  bool operator!=(const ANSIColor &c) const;
  void setFg(COLOR f);
  void setBg(COLOR b);
  COLOR getFg() { return fg; };
  COLOR getBg() { return bg; };
  void attr(ATTR a);

  /**
   * @return std::string
   */
  std::string output(void) const;

  std::string debug(void);

  /**
   * @param previous the previous attributes and colors
   * @return std::string
   */
  std::string output(ANSIColor &previous) const;

  /**
   * @param os Output stream
   * @param c ANSIColor
   * @return std::ostream&
   */
  friend std::ostream &operator<<(std::ostream &os, const ANSIColor &c);
};

/**
 * @class Door
 *
 * This handles output to the caller, via ostream.
 *
 */
class Door : public std::ostream, private std::streambuf {

private:
  virtual std::streamsize xsputn(const char *s, std::streamsize n);
  virtual int overflow(char c);
  std::string doorname;
  void parse_dropfile(const char *filepath);
  void init(void);
  std::time_t startup;
  struct termios tio_default;
  // getkey functions
  signed int getch(void);
  void unget(char c);
  char get(void);
  char buffer[5];
  unsigned int bpos;
  bool has_dropfile;
  bool debugging;
  std::string dropfilename;
  vector<std::string> dropfilelines;
  ofstream logf;
  void detect_unicode_and_screen(void);

  // time thread - time left
  std::promise<void> stop_thread;
  // std::future<void> stop_future;

  // atomic seconds_elapsed ?
  int seconds_elapsed;
  void time_thread_run(std::future<void> future);
  std::thread time_thread;

public:
  /**
   * @param argc int
   * @param argv char *[]
   */
  Door(std::string dname, int argc, char *argv[]);
  /// Default copy ctor deleted
  Door(Door &) = delete;
  virtual ~Door();
  ofstream &log(void);
  // void log(std::string output);
  AnyOption opt;
  std::string debug_buffer;

  /**
   * Previous ANSI-BBS colors and attributes sent.
   * This is used to optimize our output.
   * \see ANSIColor::output()
   */
  ANSIColor previous;
  bool track;
  int cx;
  int cy;
  int width;
  int height;
  int inactivity;
  std::string username;
  std::string handle;
  std::string location;
  std::string sysop;
  // std::string bbsname;
  int node;
  atomic<int> time_left;

  signed int getkey(void);
  bool haskey(void);
  int get_input(void);
  signed int sleep_key(int secs);
  std::string input_string(int max);
};

// Use this to define the deprecated colorizer  [POC]
// typedef std::function<void(Door &, std::string &)> colorFunction;

/**
 * @class ColorOutput
 * This works with \ref Render to create the output.  This consists
 * of ANSIColor and text position + length.
 *
 * @brief This holds an ANSIColor and text position + length
 *
 */
class ColorOutput {
public:
  ColorOutput();
  void reset(void);

  /// Color to use for this fragment
  ANSIColor c;
  /// Starting position of Render.text
  int pos;
  /// Length
  int len;
};

/*
No, don't do this.

Instead, return an iterator/generator.
 */

/**
 * @class Render
 * This holds the string, and a vector that contains ColorOutput parts.
 *
 * @see Render::output()
 *
 * @brief Rendering a string with ANSIColor
 *
 */
class Render {
public:
  Render(const std::string txt);
  /// Complete text to be rendered.
  const std::string text;
  /// Vector of ColorOutput object.
  std::vector<ColorOutput> outputs;
  void output(std::ostream &os);
};

/**
 * This defines the render output function.  This is used
 * to define the setRender functions, as well as the creation
 * of render functions.
 *
 * @brief Render output function
 *
 */
typedef std::function<Render(const std::string &)> renderFunction;

/**
 * This defines the update function.
 *
 * This updates the text.
 */
typedef std::function<std::string(void)> updateFunction;

/**
 * @class Clrscr
 * Clear the screen
 * @brief Clear the screen
 */
class Clrscr {
public:
  Clrscr(void);
  friend std::ostream &operator<<(std::ostream &os, const Clrscr &clr);
};

/**
 * Clear the BBS terminal.
 *
 */
extern Clrscr cls;

/**
 * @class NewLine
 * Carriage return + Newline
 * @brief CR+LF
 */
class NewLine {
public:
  NewLine(void);
  friend std::ostream &operator<<(std::ostream &os, const NewLine &nl);
};

/**
 * CRLF
 */
extern NewLine nl;

/**
 * This resets the colors to normal state.
 *
 * @brief reset colors to normal
 */
extern ANSIColor reset;

/// @deprecated Not used
enum class Justify { NONE, LEFT, RIGHT, CENTER };

/**
 * @class Goto
 * This handles outputting ANSI codes to position the cursor on the screen.
 *
 * @brief ANSI Goto X, Y position
 */
class Goto {
  /// X-Position
  int x;
  /// Y-Position
  int y;

public:
  Goto(int xpos, int ypos);
  /**
   * Default Goto constructor copier
   */
  Goto(const Goto &) = default;
  void set(int xpos, int ypos);
  friend std::ostream &operator<<(std::ostream &os, const Goto &g);
};

extern const char SaveCursor[];
extern const char RestoreCursor[];

/* should we try to derive a base class, so you can have multilines of
 * multilines? */

class LineBase {
public:
  virtual ~LineBase() = default;
  virtual bool update(void) = 0;
  // friend std::ostream &operator<<(std::ostream &os, const LineBase &lb) = 0;
};

class BasicLine {
private:
  std::string text;
  bool hasColor;
  ANSIColor color;
  /// renderFunction to use when rendering Line.
  renderFunction render;
  /// updateFunction to use when updating.
  updateFunction updater;

public:
  BasicLine(std::string txt);
  BasicLine(std::string txt, ANSIColor c);
  BasicLine(const BasicLine &rhs) = default;
  virtual ~BasicLine() = default;

  bool hasRender(void);
  void setText(std::string txt);
  void setColor(ANSIColor c);
  void setRender(renderFunction rf);
  void setUpdater(updateFunction uf);
  bool update(void);

  friend std::ostream &operator<<(std::ostream &os, const BasicLine &l);
};

class MultiLine {
private:
  std::vector<std::shared_ptr<BasicLine>> lines;

public:
  MultiLine();
  void append(std::shared_ptr<BasicLine> bl);

  bool update(void);
  friend std::ostream &operator<<(std::ostream &os, const MultiLine &l);
};

/**
 * @class Line
 * This holds text and ANSIColor information, and knows how to
 * send them out to the Door.
 * @brief Text and ANSIColor
 */
class Line {
private:
  /// Text of the line
  std::string text;

  /// Do we have color?
  bool hasColor;
  /// Line color
  ANSIColor color;
  /// Padding characters
  std::string padding;
  /// Padding color
  ANSIColor paddingColor;

  /// renderFunction to use when rendering Line.
  renderFunction render;
  /// updateFunction to use when updating.
  updateFunction updater;

public:
  Line(std::string &txt, int width = 0);
  Line(const char *txt, int width = 0);
  Line(std::string &txt, int width, ANSIColor c);
  Line(const char *txt, int width, ANSIColor c);
  Line(std::string &txt, int width, renderFunction rf);
  Line(const char *txt, int width, renderFunction rf);
  Line(const Line &rhs);
  // ~Line();

  bool hasRender(void);
  int length(void); //  const;
  /**
   * @param width int
   */
  void makeWidth(int width);
  /**
   * @param padstring std::string &
   * @param padColor ANSIColor
   */
  void setPadding(std::string &padstring, ANSIColor padColor);
  /**
   * @param padstring const char *
   * @param padColor ANSIColor
   */
  void setPadding(const char *padstring, ANSIColor padcolor);
  void setText(std::string &txt);
  void setText(const char *txt);

  void setColor(ANSIColor c);
  void setRender(renderFunction rf);
  void setUpdater(updateFunction uf);
  bool update(void);

  std::string debug(void);

  /**
   * @todo This might be a problem, because const Line wouldn't
   * allow me to track "updates".  I.E.  I send the line, I'd
   * need to change the line's State to "nothing changed".
   * Then, if something did change, the next update request would
   * be able to know that yes, this does indeed need to be sent.
   *
   * @bug This also might cause problems if I display a shared
   * BasicLine (in multiple places), and then update it.  It
   * would only update in the first place (the others wouldn't
   * show it needs an update).
   */
  friend std::ostream &operator<<(std::ostream &os, const Line &l);
};

/// Example BlueYellow renderFunction
extern renderFunction rBlueYellow;

/**
 * The different Borders supported by Panel.
 *
 */
enum class BorderStyle {
  /// NONE (0)
  NONE,
  /// SINGLE (1)
  SINGLE,
  /// DOUBLE (2)
  DOUBLE,
  /// SINGLE top DOUBLE side (3)
  SINGLE_DOUBLE,
  /// DOUBLE top SINGLE side (4)
  DOUBLE_SINGLE,
  /// BLANK (5)
  BLANK
};

class Panel {
protected:
  int x;
  int y;
  int width; // or padding ?
  BorderStyle border_style;
  ANSIColor border_color;
  /**
   * @todo Fix this to use shared_ptr.
   * I don't think unique_ptr is the right way to go with this.  I want to reuse
   * things, and that means shared_ptr!
   *
   */
  std::vector<std::unique_ptr<Line>> lines;
  bool hidden;
  // when you show panel, should it mark it as
  // redisplay everything??  maybe??
  bool shown_once; // ?? maybe  shown_once_already ?
  std::unique_ptr<Line> title;
  int offset;

public:
  Panel(int x, int y, int width);
  // Panel(const Panel &);
  Panel(Panel &) = delete; // default;
  Panel(Panel &&ref);

  void set(int x, int y);
  void setTitle(std::unique_ptr<Line> T, int off = 1);
  void setStyle(BorderStyle bs);
  void setColor(ANSIColor c);
  void hide(void);
  void show(void);
  void addLine(std::unique_ptr<Line> l);
  // bool delLine(std::shared_ptr<Line> l); // ?
  /*
  void display(void);
  void update(void);
  */

  /**
   * @brief Updates a panel.
   *
   * returns True if something was changed (and cursor has moved)
   * False, nothing to do, cursor is ok.
   *
   * @param d
   * @return true
   * @return false
   */
  bool update(Door &d);
  void update(Door &d, int line);
  door::Goto gotoEnd(void);

  friend std::ostream &operator<<(std::ostream &os, const Panel &p);
};

/*
Menu - defaults to double lines.
Has colorize for selected item / non-selected.
Arrow keys + ENTER, or keypress to select an item.
[O] Option Displayed Here

[ + ] = c1
O = c2
Remaining UC TEXT = c3
Remaining LC text = c4

// Colors for CS and CU (color selected, color unselected)
 */

class Menu : public Panel {
private:
  unsigned int chosen;
  std::vector<char> options;
  renderFunction selectedRender;
  renderFunction unselectedRender;
  /*
  std::function<void(Door &d, std::string &)> selectedColorizer;
  std::function<void(Door &d, std::string &)> unselectedColorizer;
  */

public:
  static renderFunction defaultSelectedRender;
  static renderFunction defaultUnselectedRender;
  /*
  static std::function<void(Door &d, std::string &)> defaultSelectedColorizer;
  static std::function<void(Door &d, std::string &)> defaultUnselectedColorizer;
  */

  Menu(int x, int y, int width);
  // Menu(const Menu &);
  Menu(const Menu &) = delete;
  Menu(Menu &&);

  void addSelection(char c, const char *line);
  void defaultSelection(int d);
  void setRender(bool selected, renderFunction render);
  /*
  void setColorizer(bool selected,
                    std::function<void(Door &d, std::string &)> colorizer);
  */

  int choose(Door &door);
  char which(int d);

  static renderFunction makeRender(ANSIColor c1, ANSIColor c2, ANSIColor c3,
                                   ANSIColor c4);

  // static std::function<void(Door &d, std::string &)>
  // makeColorizer(ANSIColor c1, ANSIColor c2, ANSIColor c3, ANSIColor c4);
};

renderFunction renderStatusValue(ANSIColor state, ANSIColor value);

class Screen {
private:
  bool hidden;
  std::vector<std::shared_ptr<Panel>> parts;

public:
  Screen(void);
  Screen(Screen &) = default;
  void addPanel(std::shared_ptr<Panel> p);
  bool delPanel(std::shared_ptr<Panel> p); // HMM.  Or ptr?
  void hide(void);
  void show(void);

  friend std::ostream &operator<<(std::ostream &os, const Screen &s);
};

/*
screen - contains panels.
  - default to 1,1 X 80,24
  - refresh(style) could redraw panels by order they were added,
  or could redraw panels from top to bottom, left to right.

crazy ideas:
  hide panels / z-order
  how to handle panel on top of other panels?
  Can I have you win + show animated final score calculations?

panel - has X,Y and width, optional length.  contains lines.
  length could be simply number of "lines".
  - has optional border.  double/single/Ds/Sd  TOPbottom
  - has optional title.
  - has optional footer.

  addLine()
  append() - Appends another line to current line.

  set(X,Y) - set a "line" at a given X,Y position.

menu - another type of panel, contains menu options/lines.

lightmenu - like above, but allows arrow keys to select menu options.

line - contains text.
  (Maybe a "dirty" flag is needed here?)
  - has optional (width)
  - has optional (justify - L, R, Center)
  - has optional padding (# of blank chars)
  - has color (of text)
  - has formatter/coloring function (to colorize the text)
  Example would be one that sets capital letters to one color, lower to another.
  Another example would be one that displays Score: XXX, where Score is one
  color, : is another, and XXX is yet another.  Properly padded, of course.
  - has "lambda" function to update the value? (Maybe?)
  Idea would be that I could update the score, and panel.update().  It would
  call all the line.update() functions and only update anything that has
  changed.

  Crazy ideas:
  Can I delete a line, and have it automatically removed from a panel?

lightline - text, changes format/coloring if focus/nofocus is set?

 */

} // namespace door
#endif