#include "door.h"
#include <algorithm>
#include <chrono>
#include <ctype.h>
#include <string.h>
#include <string>
#include <thread>

#include <libgen.h> // basename

// time/date output std::put_time()
// https://en.cppreference.com/w/cpp/io/manip/put_time
#include <ctime>
#include <iomanip>

// alarm signal
#include <signal.h>
#include <unistd.h>

#include <iconv.h>

#include <algorithm>
#include <iostream>

namespace door {

void to_lower(std::string &text) {
  transform(text.begin(), text.end(), text.begin(), ::tolower);
}

bool replace(std::string &str, const std::string &from, const std::string &to) {
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos)
    return false;
  str.replace(start_pos, from.length(), to);
  return true;
}

static bool hangup = false;

void sig_handler(int signal) {
  hangup = true;
  /*
  ofstream sigf;
  sigf.open("signal.log", std::ofstream::out | std::ofstream::app);

  sigf << "SNAP! GOT: " << signal << std::endl;
  sigf.close();
  */
  // 13 SIGPIPE -- ok, what do I do with this, eh?
}

class IConv {
  iconv_t ic;

public:
  IConv(const char *to, const char *from);
  ~IConv();

  int convert(char *input, char *output, size_t outbufsize);
};

IConv::IConv(const char *to, const char *from) : ic(iconv_open(to, from)) {}
IConv::~IConv() { iconv_close(ic); }

int IConv::convert(char *input, char *output, size_t outbufsize) {
  size_t inbufsize = strlen(input);
  // size_t orig_size = outbufsize;
  // memset(output, 0, outbufsize);
  // https://www.gnu.org/savannah-checkouts/gnu/libiconv/documentation/libiconv-1.15/iconv.3.html
  int r = iconv(ic, &input, &inbufsize, &output, &outbufsize);
  *output = 0;
  return r;
}

static IConv converter("UTF-8", "CP437");

void cp437toUnicode(std::string input, std::string &out) {
  char buffer[10240];
  char output[16384];

  strcpy(buffer, input.c_str());
  converter.convert(buffer, output, sizeof(output));
  out.assign(output);
}

void cp437toUnicode(const char *input, std::string &out) {
  char buffer[10240];
  char output[16384];

  strcpy(buffer, input);
  converter.convert(buffer, output, sizeof(output));
  out.assign(output);
}

bool unicode = false;
bool debug_capture = false;

/**
 * Construct a new Door object using the commandline parameters
 * given to the main function.
 *
 * @example door_example.cpp
 */
Door::Door(std::string dname, int argc, char *argv[])
    : std::ostream(this), doorname{dname}, has_dropfile{false},
      seconds_elapsed{0}, previous(COLOR::WHITE), track{true}, cx{1}, cy{1},
      inactivity{120}, node{1} {

  // Setup commandline options
  opt.addUsage("Door++ library by BUGZ (C) 2021");
  opt.addUsage("");
  opt.addUsage(" -h  --help                 Displays this help");
  opt.addUsage(" -l  --local                Local Mode");
  opt.addUsage(" -d  --dropfile [FILENAME]  Load Dropfile");
  opt.addUsage(" -n  --node N               Set node number");
  // opt.addUsage(" -b  --bbsname NAME         Set BBS Name");
  opt.addUsage(" -u  --username NAME        Set Username");
  opt.addUsage(" -t  --timeleft N           Set time left");
  opt.addUsage("     --maxtime N            Set max time");
  opt.addUsage("");
  opt.setFlag("help", 'h');
  opt.setFlag("local", 'l');
  opt.setOption("dropfile", 'd');
  // opt.setOption("bbsname", 'b');
  opt.setOption("username", 'u');
  opt.setOption("timeleft", 't');
  opt.setOption("maxtime");

  opt.processCommandArgs(argc, argv);

  if (!opt.hasOptions()) {
    opt.printUsage();
    exit(1);
  }

  if (opt.getFlag("help") || opt.getFlag('h')) {
    opt.printUsage();
    exit(1);
  }

  if (opt.getValue("username") != nullptr) {
    username = opt.getValue("username");
  }

  if (opt.getValue("node") != nullptr) {
    node = atoi(opt.getValue("node"));
  }

  if (opt.getValue("timeleft") != nullptr) {
    time_left = atoi(opt.getValue("timeleft"));
  } else {
    // sensible default
    time_left = 25;
  }

  /*
  if (opt.getValue("bbsname") != nullptr) {
    bbsname = opt.getValue("bbsname");
  }
  */

  parse_dropfile(opt.getValue("dropfile"));

  /*
  If the dropfile has time_left, we'll use it.

  Adjust time_left by maxtime value (if given).
   */

  if (opt.getValue("maxtime") != nullptr) {
    int maxtime = atoi(opt.getValue("maxtime"));
    if (time_left > maxtime) {
      logf << "Adjusting time from " << time_left << " to " << maxtime
           << std::endl;
      time_left = maxtime;
    }
  }

  if (opt.getFlag("local")) {
    if (username.empty()) {
      std::cout << "Local mode requires username to be set." << std::endl;
      opt.printUsage();
      exit(1);
    }
  } else {
    // we must have a dropfile, or else!
    if (!has_dropfile) {
      std::cout << "I require a dropfile.  And a shrubbery." << std::endl;
      opt.printUsage();
      exit(1);
    }
  }

  // Set program name

  std::string logFileName = dname + ".log";
  logf.open(logFileName.c_str(), std::ofstream::out | std::ofstream::app);

  log("Door init");
  init();

  // door.sys doesn't give BBS name. system_name
  detect_unicode_and_screen();
  logf << "Screen " << width << " X " << height << std::endl;
}

Door::~Door() {
  // restore default mode
  // tcsetattr(STDIN_FILENO, TCSANOW, &tio_default);
  log("dtor");
  tcsetattr(STDIN_FILENO, TCOFLUSH, &tio_default);
  signal(SIGHUP, SIG_DFL);
  signal(SIGPIPE, SIG_DFL);

  // time thread
  stop_thread.set_value();
  time_thread.join();
  log("done");
  logf.close();
}

// https://www.tutorialspoint.com/how-do-i-terminate-a-thread-in-cplusplus11

void Door::time_thread_run(std::future<void> future) {
  while (future.wait_for(std::chrono::milliseconds(1)) ==
         std::future_status::timeout) {
    // std::cout << "Executing the thread....." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    seconds_elapsed++;
    // log("TICK");
    // logf << "TICK " << seconds_elapsed << std::endl;
    if (seconds_elapsed % 60 == 0) {
      if (time_left > 0)
        time_left--;
    }
  }
}

void Door::init(void) {
  // https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
  // https://viewsourcecode.org/snaptoken/kilo/03.rawInputAndOutput.html
  // ok!  I didn't know about VTIME !  That's something different!

  // enable terminal RAW mode
  struct termios tio_raw;
  tcgetattr(STDIN_FILENO, &tio_default);
  tio_raw = tio_default;
  cfmakeraw(&tio_raw);
  // local terminal magic
  tio_raw.c_cc[VMIN] = 0;
  tio_raw.c_cc[VTIME] = 1;

  bpos = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &tio_raw);

  startup = std::time(nullptr);
  signal(SIGHUP, sig_handler);
  signal(SIGPIPE, sig_handler);

  // time thread
  std::future<void> stop_future = stop_thread.get_future();
  time_thread =
      std::thread(&Door::time_thread_run, this, std::move(stop_future));
}

void Door::detect_unicode_and_screen(void) {
  unicode = false;
  width = 0;
  height = 0;

  if (!isatty(STDIN_FILENO)) {
    // https://stackoverflow.com/questions/273261/force-telnet-client-into-character-mode
    *this << "\377\375\042\377\373\001"; // fix telnet client
  }

  *this << "\x1b[0;30;40m\x1b[2J\x1b[H"; // black on black, clrscr, go home
  *this << "\u2615"
        << "\x1b[6n";                   // hot beverage + cursor pos
  *this << "\x1b[999C\x1b[999B\x1b[6n"; // goto end of screen + cursor pos
  *this << "\x1b[H";                    // go home

  this->flush();
  usleep(1000 * 1000);

  if (haskey()) {
    char buffer[101];
    int len;
    len = read(STDIN_FILENO, &buffer, sizeof(buffer) - 1);
    // logf << "read " << len << std::endl;
    if (len > 0) {
      buffer[len] = 0;
      int x;
      /*
      for (x = 0; x < len; x++) {
        if (buffer[x] < 0x20)
          logf << std::hex << (int)buffer[x] << " ";
        else
          logf << buffer[x] << " ";
      }
      */
      for (x = 0; x < len; x++) {
        if (buffer[x] == 0)
          buffer[x] = ' ';
      }

      /*
      logf << std::endl;
      logf << "BUFFER [" << (char *)buffer << "]" << std::endl;
      */
      if (0) {
        std::string cleanbuffer = buffer;
        std::string esc = "\x1b";
        std::string esc_text = "^[";

        while (replace(cleanbuffer, esc, esc_text)) {
        };

        logf << "BUFFER [" << cleanbuffer << "]" << std::endl;
      }
      // this did not work -- because of the null characters in the buffer.

      // 1;3R required on David's machine.  I'm not sure why.
      // 1;3R also happens under VSCodium.
      // 1;4R is what I get from syncterm.

      if ((strstr(buffer, "1;2R") != nullptr) or
          (strstr(buffer, "1;3R") != nullptr)) {
        unicode = true;
        log("unicode enabled \u2615"); // "U0001f926");
      }
      // Get the terminal screen size
      // \x1b[1;2R\x1b[41;173R
      // log(buffer);

      char *cp;
      cp = strchr(buffer, '\x1b');
      if (cp != nullptr) {
        cp = strchr(cp + 1, '\x1b');
      } else {
        log("Failed terminal size detection.  See buffer:");
        log(buffer);
        return;
      }

      if (cp != nullptr) {
        cp++;
        if (*cp == '[') {
          cp++;
          height = atoi(cp);
          cp = strchr(cp, ';');
          if (cp != nullptr) {
            cp++;
            width = atoi(cp);
          } else {
            height = 0;
          }
        }
      }
    }
  } else {
    logf << "FAIL-WHALE, no response to terminal getposition." << std::endl;
  }
}

void Door::parse_dropfile(const char *filepath) {
  if (filepath == nullptr)
    return;

  // Ok, parse file here...
  std::ifstream file(filepath);
  std::string line;
  while (std::getline(file, line)) {
    // These are "DOS" files.  Remove trailing \r.
    if (!line.empty() && line[line.size() - 1] == '\r')
      line.erase(line.size() - 1);
    dropfilelines.push_back(line);
  }
  file.close();

  std::string filename;
  {
    // converting const char * to char * for basename.
    char *temp = strdup(filepath);
    filename = basename(temp);
    free(temp);
  }

  to_lower(filename);

  // for now, just door.sys.

  if (filename == "door.sys") {
    // Ok, parse away!
    node = atoi(dropfilelines[3].c_str());
    username = dropfilelines[9];
    location = dropfilelines[10];
    time_left = atoi(dropfilelines[18].c_str());
    sysop = dropfilelines[34];
    handle = dropfilelines[35];
  } else {
    std::string msg = "Unknown dropfile: ";
    msg += filename;
    log(msg);
    *this << msg << std::endl;
    exit(2);
  }
  has_dropfile = true;
}

void Door::log(std::string output) {
  // todo:  have logging

  std::time_t t = std::time(nullptr);
  std::tm tm = *std::localtime(&t);
  logf << std::put_time(&tm, "%c ") << output << std::endl;
}

bool Door::haskey(void) {
  fd_set socket_set;
  struct timeval tv;
  int select_ret = -1;

  if (hangup)
    return -2;

  if (time_left < 2)
    return -3;

  while (select_ret == -1) {
    FD_ZERO(&socket_set);
    FD_SET(STDIN_FILENO, &socket_set);

    tv.tv_sec = 0;
    tv.tv_usec = 1;

    select_ret = select(STDIN_FILENO + 1, &socket_set, NULL, NULL, &tv);
    if (select_ret == -1) {
      if (errno == EINTR)
        continue;
      log("hangup detected");
      hangup = true;
      return (-2);
    }
    if (select_ret == 0)
      return false;
  }
  return true;
}

/*
  low-lever read a key from terminal or stdin.
  Returns key, or
  -1 (no key available)
  -2 (read error)
 */
signed int Door::getch(void) {
  fd_set socket_set;
  struct timeval tv;
  int select_ret = -1;
  int recv_ret;
  char key;

  if (door::hangup)
    return -2;

  if (time_left < 2)
    return -3;

  while (select_ret == -1) {
    FD_ZERO(&socket_set);
    FD_SET(STDIN_FILENO, &socket_set);

    tv.tv_sec = 0;
    tv.tv_usec = 100;

    select_ret = select(STDIN_FILENO + 1, &socket_set, NULL, NULL, &tv);
    // select(STDIN_FILENO + 1, &socket_set, NULL, NULL, bWait ? NULL : &tv);
    if (select_ret == -1) {
      if (errno == EINTR)
        continue;
      log("hangup detected");
      door::hangup = true;
      return (-2);
    }
    if (select_ret == 0)
      return (-1);
  }

  recv_ret = read(STDIN_FILENO, &key, 1);
  if (recv_ret != 1) {
    // possibly log this.
    log("hangup");
    hangup = true;
    return -2;
  }
  return key;
}

void Door::unget(char c) {
  if (bpos < sizeof(buffer) - 1) {
    buffer[bpos] = c;
    bpos++;
  }
}

char Door::get(void) {
  if (bpos == 0)
    return 0;
  bpos--;
  return buffer[bpos];
}

signed int Door::getkey(void) {
  signed int c, c2;

  if (bpos != 0) {
    c = get();
  } else {
    c = getch();
  }

  if (c < 0)
    return c;

  /*
  We get 0x0d 0x00 for [Enter] (Syncterm)
  This strips out the null.
  */
  if (c == 0x0d) {
    c2 = getch();
    if ((c2 != 0) and (c2 >= 0))
      unget(c2);
    return c;
  }

  if (c == 0x1b) {
    // possible extended key
    c2 = getch();
    if (c2 < 0) {
      // nope, just plain ESC key
      return c;
    }

    // consume extended key values int extended buffer
    char extended[16];
    unsigned int pos = 0;
    extended[pos] = (char)c2;
    extended[pos + 1] = 0;
    pos++;
    while ((pos < sizeof(extended) - 1) and ((c2 = getch()) >= 0)) {
      // special case here where I'm sending out cursor location requests
      // and the \x1b[X;YR strings are getting buffered.
      if (c2 == 0x1b) {
        unget(c2);
        break;
      }
      extended[pos] = (char)c2;
      extended[pos + 1] = 0;
      pos++;
    }

    // convert extended buffer to special key
    if (extended[0] == '[') {
      switch (extended[1]) {
      case 'A':
        return XKEY_UP_ARROW;
      case 'B':
        return XKEY_DOWN_ARROW;
      case 'C':
        return XKEY_RIGHT_ARROW;
      case 'D':
        return XKEY_LEFT_ARROW;
      case 'H':
        return XKEY_HOME;
      case 'F':
        return XKEY_END; // terminal
      case 'K':
        return XKEY_END;
      case 'U':
        return XKEY_PGUP;
      case 'V':
        return XKEY_PGDN;
      case '@':
        return XKEY_INSERT;
      }

      if (extended[pos - 1] == '~') {
        // \x1b[digits~
        int number = atoi(extended + 1);
        switch (number) {
        case 2:
          return XKEY_INSERT; // terminal
        case 3:
          return XKEY_DELETE; // terminal
        case 5:
          return XKEY_PGUP; // terminal
        case 6:
          return XKEY_PGDN; // terminal
        case 15:
          return XKEY_F5; // terminal
        case 17:
          return XKEY_F6; // terminal
        case 18:
          return XKEY_F7; // terminal
        case 19:
          return XKEY_F8; // terminal
        case 20:
          return XKEY_F9; // terminal
        case 21:
          return XKEY_F10; // terminal
        case 23:
          return XKEY_F11;
        case 24:
          return XKEY_F12; // terminal
        }
      }
    }

    if (extended[0] == 'O') {
      switch (extended[1]) {
      case 'P':
        return XKEY_F1;
      case 'Q':
        return XKEY_F2;
      case 'R':
        return XKEY_F3;
      case 'S':
        return XKEY_F4;
      case 't':
        return XKEY_F5; // syncterm
      }
    }

    // unknown -- This needs to be logged
    logf << "\r\nDEBUG:\r\nESC + ";
    for (unsigned int x = 0; x < pos; x++) {
      char z = extended[x];
      if (iscntrl(z)) {
        logf << (int)z << " ";
      } else {
        logf << "'" << (char)z << "'"
             << " ";
      };
    }
    logf << "\r\n";
    logf.flush();

    return XKEY_UNKNOWN;
  }
  return c;
}

int Door::get_input(void) {
  signed int c;
  c = getkey();
  if (c < 0)
    return 0;
  return c;

  /*
  tODInputEvent event;
  od_get_input(&event, OD_NO_TIMEOUT, GETIN_NORMAL);
  */
}

/*
The following code will wait for 1.5 second:

#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>`

int main() {
    struct timeval t;
    t.tv_sec = 1;
    t.tv_usec = 500000;
    select(0, NULL, NULL, NULL, &t);
}

 */
signed int Door::sleep_key(int secs) {
  fd_set socket_set;
  struct timeval tv;
  int select_ret = -1;
  /*
  int recv_ret;
  char key;
  */
  if (hangup)
    return -2;

  if (time_left < 2)
    return -3;

  while (select_ret == -1) {
    FD_ZERO(&socket_set);
    FD_SET(STDIN_FILENO, &socket_set);

    tv.tv_sec = secs;
    tv.tv_usec = 0;

    select_ret = select(STDIN_FILENO + 1, &socket_set, NULL, NULL, &tv);
    // select(STDIN_FILENO + 1, &socket_set, NULL, NULL, bWait ? NULL : &tv);
    if (select_ret == -1) {
      if (errno == EINTR)
        continue;
      hangup = true;
      log("hangup detected");
      return (-2);
    }
    if (select_ret == 0)
      return (-1);
  }
  return getkey();
}

std::string Door::input_string(int max) {
  std::string input;

  // draw the input area.
  *this << std::string(max, ' ');
  *this << std::string(max, '\x08');

  int c;

  while (true) {
    c = sleep_key(inactivity);
    if (c < 0) {
      input.clear();
      return input;
    }
    if (c > 0x1000)
      continue;

    if (isprint(c)) {
      if (int(input.length()) < max) {
        *this << char(c);
        input.append(1, c);
      } else {
        // bell
        *this << '\x07';
      }
    } else {
      switch (c) {
      case 0x08:
      case 0x7f:
        if (input.length() > 0) {
          *this << "\x08 \x08";
          // this->flush();
          input.erase(input.length() - 1);
        };
        break;
      case 0x0d:
        return input;
      }
    }
  }
}

/**
 * Take given buffer and output it.
 *
 * This originally stored output in the buffer.  We now directly output
 * via the OpenDoor od_disp_emu.
 *
 * @param s const char *
 * @param n std::streamsize
 * @return std::streamsize
 */
std::streamsize Door::xsputn(const char *s, std::streamsize n) {
  if (debug_capture) {
    debug_buffer.append(s, n);
  } else {
    static std::string buffer;
    buffer.append(s, n);
    // setp(&(*buffer.begin()), &(*buffer.end()));
    if (!hangup) {
      std::cout << buffer;
      std::cout.flush();
    }
    // od_disp_emu(buffer.c_str(), TRUE);
    // Tracking character position could be a problem / local terminal unicode.
    if (track)
      cx += n;
    buffer.clear();
  }
  return n;
}

/**
 * Stores a character into the buffer.
 * This does still use the buffer.
 * @todo Replace this also with a direct call to od_disp_emu.
 *
 * @param c char
 * @return int
 */
int Door::overflow(char c) {
  /*
  char temp[2];
  temp[0] = c;
  temp[1] = 0;
  */

  // buffer.push_back(c);
  // od_disp_emu(temp, TRUE);
  if (debug_capture) {
    debug_buffer.append(1, c);
  } else {
    if (!hangup) {
      std::cout << c;
      std::cout.flush();
    }
  }
  if (track)
    cx++;
  // setp(&(*buffer.begin()), &(*buffer.end()));
  return c;
}

/**
 * Construct a new Color Output:: Color Output object
 * We default to BLACK/BLACK (not really a valid color),
 * pos=0 and len=0.
 */
ColorOutput::ColorOutput() : c(COLOR::BLACK, COLOR::BLACK) {
  pos = 0;
  len = 0;
}

/**
 * Reset pos and len to 0.
 */
void ColorOutput::reset(void) {
  pos = 0;
  len = 0;
}

/**
 * Construct a new Render:: Render object
 *
 * Render consists of constant text,
 * and vector of ColorOutput
 *
 * @param txt Text
 */
Render::Render(const std::string txt) : text{txt} {}

/**
 * Output the Render.
 *
 * This consists of going through the vector, getting
 * the fragment (from pos and len), and outputting the
 * color and fragment.
 *
 * @param os
 */
void Render::output(std::ostream &os) {
  for (auto out : outputs) {
    std::string fragment = text.substr(out.pos, out.len);
    os << out.c << fragment;
  }
}

/**
 * Construct a new Clrscr:: Clrscr object
 *
 * This is used to clear the screen.
 */
Clrscr::Clrscr() {}

/**
 * Clear the screen using ANSI codes.
 *
 * Not all systems home the cursor after clearing the screen.
 * We automatically home the cursor as well.
 *
 * @param os std::ostream&
 * @param clr const Clrscr&
 * @return std::ostream&
 */
std::ostream &operator<<(std::ostream &os, const Clrscr &clr) {
  Door *d = dynamic_cast<Door *>(&os);
  if (d != nullptr) {
    d->track = false;
    *d << "\x1b[2J"
          "\x1b[H";
    d->cx = 1;
    d->cy = 1;
    d->track = true;
  } else {
    os << "\x1b[2J"
          "\x1b[H";
  }
  return os;
}

Clrscr cls;

/**
 * This is used to issue NL+CR
 *
 */
NewLine::NewLine() {}

/**
 * Output Newline + CarriageReturn
 * @param os std::ostream
 * @param nl const NewLine
 * @return std::ostream&
 */
std::ostream &operator<<(std::ostream &os, const NewLine &nl) {
  Door *d = dynamic_cast<Door *>(&os);
  if (d != nullptr) {
    d->track = false;
    *d << "\r\n";
    d->cx = 1;
    d->cy++;
    d->track = true;
  } else {
    os << "\r\n";
  };
  return os;
}

NewLine nl;

/**
 * Construct a new Goto:: Goto object
 *
 * @param xpos
 * @param ypos
 */
Goto::Goto(int xpos, int ypos) {
  x = xpos;
  y = ypos;
}

/**
 * Output the ANSI codes to position the cursor to the given y,x position.
 *
 * @todo Optimize the ANSI goto string output.
 * @todo Update the Door object so it know where the cursor
 * is positioned.
 *
 * @param os std::ostream
 * @param g const Goto
 * @return std::ostream&
 */
std::ostream &operator<<(std::ostream &os, const Goto &g) {
  Door *d = dynamic_cast<Door *>(&os);
  if (d != nullptr) {
    d->track = false;
    *d << "\x1b[";
    if (g.y > 1)
      *d << std::to_string(g.y);
    os << ";";
    if (g.x > 1)
      *d << std::to_string(g.x);
    *d << "H";
    d->cx = g.x;
    d->cy = g.y;
    d->track = true;
  } else {
    os << "\x1b[" << std::to_string(g.y) << ";" << std::to_string(g.x) << "H";
  };
  return os;
}

// EXAMPLES

/// BlueYellow Render example function
renderFunction rBlueYellow = [](const std::string &txt) -> Render {
  Render r(txt);

  ColorOutput co;

  bool uc = true;
  ANSIColor blue(COLOR::BLUE, ATTR::BOLD);
  ANSIColor cyan(COLOR::YELLOW, ATTR::BOLD);

  co.pos = 0;
  co.len = 0;
  co.c = blue;
  // d << blue;

  int tpos = 0;
  for (char const &c : txt) {
    if (uc) {
      if (!isupper(c)) {
        // possible color change
        if (co.len != 0) {
          r.outputs.push_back(co);
          co.reset();
          co.pos = tpos;
        }

        co.c = cyan;
        // d << cyan;
        uc = false;
      }
    } else {
      if (isupper(c)) {
        if (co.len != 0) {
          r.outputs.push_back(co);
          co.reset();
          co.pos = tpos;
        }
        co.c = blue;

        // d << blue;
        uc = true;
      }
    }
    co.len++;
    tpos++;
    // d << c;
  }
  if (co.len != 0) {
    r.outputs.push_back(co);
  }
  return r;
};

door::renderFunction renderStatusValue(door::ANSIColor status,
                                       door::ANSIColor value) {
  door::renderFunction rf = [status,
                             value](const std::string &txt) -> door::Render {
    door::Render r(txt);
    door::ColorOutput co;

    co.pos = 0;
    co.len = 0;
    co.c = status;

    size_t pos = txt.find(':');
    if (pos == std::string::npos) {
      // failed to find - use entire string as status color.
      co.len = txt.length();
      r.outputs.push_back(co);
    } else {
      pos++; // Have : in status color
      co.len = pos;
      r.outputs.push_back(co);
      co.reset();
      co.pos = pos;
      co.c = value;
      co.len = txt.length() - pos;
      r.outputs.push_back(co);
    }

    return r;
  };
  return rf;
}

door::renderFunction rStatusValue = [](const std::string &txt) -> door::Render {
  door::Render r(txt);
  door::ColorOutput co;

  // default colors STATUS: value
  door::ANSIColor status(door::COLOR::BLUE, door::ATTR::BOLD);
  door::ANSIColor value(door::COLOR::YELLOW, door::ATTR::BOLD);

  co.pos = 0;
  co.len = 0;
  co.c = status;

  size_t pos = txt.find(':');
  if (pos == std::string::npos) {
    // failed to find - use entire string as status color.
    co.len = txt.length();
    r.outputs.push_back(co);
  } else {
    pos++; // Have : in status color
    co.len = pos;
    r.outputs.push_back(co);
    co.reset();
    co.pos = pos;
    co.c = value;
    co.len = txt.length() - pos;
    r.outputs.push_back(co);
  }

  return r;
};

/*
std::function<void(Door &d, std::string &txt)> BlueYellow2 =
    [](Door &d, std::string &txt) -> void {
  bool uc = true;
  ANSIColor blue(COLOR::BLACK, COLOR::CYAN);
  ANSIColor cyan(COLOR::YELLOW, COLOR::BLUE, ATTR::BOLD);

  d << blue;
  for (char const &c : txt) {
    if (uc) {
      if (c == ':') {
        d << cyan;
        uc = false;
      }
    }
    d << c;
  }
};

std::function<void(Door &d, std::string &txt)> Aweful =
    [](Door &d, std::string &txt) -> void {
  for (char const &c : txt) {
    // Color clr((Colors)((c % 14) + 1), Colors::BLACK, 0);
    // Use only BRIGHT/LIGHT colors.
    ANSIColor clr((COLOR)(c % 8), ATTR::BOLD);
    d << clr << c;
  }
};
*/

} // namespace door
