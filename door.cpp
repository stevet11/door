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
#include <unistd.h> //

#include <iconv.h>

#include <algorithm>
#include <iostream>

#include <pwd.h> // getpwuid

/**
 * @file
 * @brief Door
 */

/*

My strategy here has failed.

If I set enigma to cp437, then it handles everything but the cp437
symbols (diamonds/hearts/spades/clubs) correctly on the unicode side.
[And my door thinks it's all cp437 always]

If I set enigma to utf8, then it works right on the ssh terminal side.
But cp437 turns to puke because it's trying to convert cp437 from
utf8 to cp437.  The symbols get '?'.

I can't detect unicode (when set to utf8), but I can detect cp437
(by sending the diamonds/hearts characters).

But I can't get through the enigma translation system.  If only iconv worked
correctly with hearts/clubs symbols!  Then I wouldn't need this broken
work-around code.
 */

namespace door {

/**
 * @brief pushback buffer for keys.
 *
 * This allows us to peek ahead and push back characters we're not interested
 * it.  It also allows us to test the \ref Door::getkey function.
 */
std::list<char> pushback;

/**
 * @brief convert string to lowercase
 *
 * @param text
 */
void to_lower(std::string &text) {
  transform(text.begin(), text.end(), text.begin(), ::tolower);
}

/**
 * @brief Replaces one string with another once
 *
 * Returns true if replaced.
 *
 * @param[in,out] str String to modify
 * @param[in] from String to search for
 * @param[in] to String to replace with
 * @return true
 * @return false
 */
bool replace(std::string &str, const std::string &from, const std::string &to) {
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos)
    return false;
  str.replace(start_pos, from.length(), to);
  return true;
}

/**
 * @brief Replace all instances of from with to.
 *
 * @param str
 * @param from
 * @param to
 * @return true
 * @return false
 */
bool replace(std::string &str, const char *from, const char *to) {
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos)
    return false;
  str.replace(start_pos, strlen(from), to);
  return true;
}

static bool hangup = false;

/**
 * @brief Signal handler for detecting hangup/broken pipe
 *
 * @param signal
 */
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

/**
 * @brief Converts from one encoding to another.
 * Uses iconv (international conversion) API.
 */
class IConv {
  iconv_t ic;

public:
  IConv(const char *to, const char *from);
  ~IConv();

  int convert(char *input, char *output, size_t outbufsize);
};

/**
 * @brief Construct a new IConv::IConv object
 *
 * Give the encodings that you want to convert to and from.
 * @param to
 * @param from
 */
IConv::IConv(const char *to, const char *from) : ic(iconv_open(to, from)) {}
IConv::~IConv() { iconv_close(ic); }

/**
 * @brief Calls iconv API to do the conversion.
 *
 * Buffers must be provided.
 * @param input
 * @param output
 * @param outbufsize
 * @return int
 */
int IConv::convert(char *input, char *output, size_t outbufsize) {
  size_t inbufsize = strlen(input);
  // size_t orig_size = outbufsize;
  // memset(output, 0, outbufsize);
  // https://www.gnu.org/savannah-checkouts/gnu/libiconv/documentation/libiconv-1.15/iconv.3.html
  int r = iconv(ic, &input, &inbufsize, &output, &outbufsize);
  *output = 0;
  return r;
}

/**
 * @brief converter that we provide to convert from CP437 to UTF-8.
 * \re cp437toUnicode
 */
static IConv converter("UTF-8", "CP437");

/**
 * @brief Convert from CP437 to unicode.
 *
 * @param input
 * @param out
 */
void cp437toUnicode(std::string input, std::string &out) {
  char buffer[10240];
  char output[16384];

  strcpy(buffer, input.c_str());
  converter.convert(buffer, output, sizeof(output));
  out.assign(output);
}

/**
 * @brief Convert from CP437 to unicode.
 *
 * @param input
 * @param out
 */
void cp437toUnicode(const char *input, std::string &out) {
  char buffer[10240];
  char output[16384];

  strcpy(buffer, input);
  converter.convert(buffer, output, sizeof(output));
  out.assign(output);
}

/**
 * @brief Was unicode detected?
 */
bool unicode = false;
/**
 * @brief Was full CP437 detected?
 *
 * This is for full CP437 support, meaning it also supports hearts, diamonds,
 * spades, clubs char(3)..char(6).  These are sometimes ignored by CP437
 * translation programs as control codes.
 */
bool full_cp437 = false;
/**
 * @brief Capture the output for debugging.
 *
 * This is used by the tests.
 */
bool debug_capture = false;

/**
 * @brief Construct a new Door:: Door object
 *
 * @param[in] dname Door name used for logfile
 * @param[in] argc
 * @param[in] argv
 */
Door::Door(std::string dname, int argc, char *argv[])
    : std::ostream(this), doorname{dname},
      has_dropfile{false}, debugging{false}, seconds_elapsed{0},
      previous(COLOR::WHITE), track{true}, cx{1}, cy{1},
      inactivity{120}, node{1} {

  // Setup commandline options
  opt.addUsage("Door++ library by BUGZ (C) 2021");
  opt.addUsage("");
  opt.addUsage(" -h  --help                 Displays this help");
  opt.addUsage(" -l  --local                Local Mode");
  opt.addUsage(" -d  --dropfile [FILENAME]  Load Dropfile");
  opt.addUsage(" -n  --node N               Set node number");
  // opt.addUsage(" -c  --cp437                Force CP437");
  // opt.addUsage(" -b  --bbsname NAME         Set BBS Name");
  opt.addUsage(" -u  --username NAME        Set Username");
  opt.addUsage(" -t  --timeleft N           Set time left");
  opt.addUsage("     --maxtime N            Set max time");
  opt.addUsage("");
  opt.setFlag("help", 'h');
  opt.setFlag("local", 'l');
  opt.setFlag("cp437", 'c');
  opt.setFlag("unicode");
  opt.setFlag("debuggering");
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
    handle = username;
  }

  if (opt.getFlag("debuggering")) {
    debugging = true;
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

  time_used = 0;

  /*
  if (opt.getValue("bbsname") != nullptr) {
    bbsname = opt.getValue("bbsname");
  }
  */

  std::string logFileName = dname + ".log";
  logf.open(logFileName.c_str(), std::ofstream::out | std::ofstream::app);

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
    node = 0;
    if (username.empty()) {
      uid_t uid = geteuid();
      struct passwd *pw;
      pw = getpwuid(uid);
      if (pw) {
        // Do we have realname as well?
        std::string name = pw->pw_gecos;
        size_t pos = name.find(',');
        username = pw->pw_name;
        if (pos != std::string::npos) {
          name = name.substr(0, pos);
          if (!name.empty()) {
            username = name;
            handle = pw->pw_name;
          }
        }
        logf << "username: " << username << " handle: " << handle << std::endl;
      } else {

        std::cout << "Local mode requires username to be set." << std::endl;
        opt.printUsage();
        exit(1);
      };
    }

    // local mode, make sure we have a handle -- even if it's the same as
    // username.
    if (handle.empty())
      handle = username;
  } else {
    // we must have a dropfile, or else!
    if (!has_dropfile) {
      std::cout << "I require a dropfile.  And a shrubbery." << std::endl;
      opt.printUsage();
      exit(1);
    }
  }

  // Set program name

  log() << "Door init" << std::endl;
  init();

  // door.sys doesn't give BBS name. system_name
  if (!debugging) {
    detect_unicode_and_screen();
    logf << "Screen " << width << " X " << height << " unicode " << unicode
         << " full_cp437 " << full_cp437 << std::endl;
  }

  if (opt.getFlag("cp437")) {
    unicode = false;
  }
  if (opt.getFlag("unicode")) {
    unicode = true;
  }
}

Door::~Door() {
  // restore default mode
  // tcsetattr(STDIN_FILENO, TCSANOW, &tio_default);
  log() << "dtor" << std::endl;
  tcsetattr(STDIN_FILENO, TCOFLUSH, &tio_default);
  signal(SIGHUP, SIG_DFL);
  signal(SIGPIPE, SIG_DFL);

  // stop time thread
  stop_thread.set_value();
  time_thread.join();
  log() << "done" << std::endl;
  logf.close();
}

// https://www.tutorialspoint.com/how-do-i-terminate-a-thread-in-cplusplus11

void Door::time_thread_run(std::future<void> future) {
  while (future.wait_for(std::chrono::milliseconds(1)) ==
         std::future_status::timeout) {

    std::this_thread::sleep_for(std::chrono::seconds(1));
    ++seconds_elapsed;

    if (seconds_elapsed % 60 == 0) {
      if (time_left > 0)
        --time_left;
      ++time_used;
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

  tcsetattr(STDIN_FILENO, TCSANOW, &tio_raw);

  startup = std::time(nullptr);
  signal(SIGHUP, sig_handler);
  signal(SIGPIPE, sig_handler);

  // time thread
  std::future<void> stop_future = stop_thread.get_future();
  time_thread =
      std::thread(&Door::time_thread_run, this, std::move(stop_future));
}

/**
 * @brief Detect unicode/CP437, and screen size.
 *
 * This sets unicode and full_cp437 flags, width and height.
 *
 * This works by clearing the screen and homing the cursor.  We then output some
 * CP437 symbols and ask for the cursor position.  We send a newline, output a
 * unicode symbol, and query cursor position.
 *
 * The CP437 symbols we use are specific.  They are the hearts and diamonds
 * symbols, which are commonly mistaken for control codes.  If unicode == false
 * and full_cp437 == true, then we do fully support CP437 on this terminal.
 *
 * For the screensize, we move the cursor down 999 and move cursor right 999 and
 * query position.
 *
 * On failure to detect screensize, width and height are set to 0.
 */
void Door::detect_unicode_and_screen(void) {
  unicode = false;
  full_cp437 = false;
  width = 0;
  height = 0;

  if (!isatty(STDIN_FILENO)) {
    // https://stackoverflow.com/questions/273261/force-telnet-client-into-character-mode
    *this << "\377\375\042\377\373\001"; // fix telnet client
  }

  // maybe I need to be trying to detect cp437 instead of trying to detect
  // unicde!

  *this << "\x1b[0;30;40m\x1b[2J\x1b[H"; // black on black, clrscr, go home
  *this << "\x03\x04"                    // hearts and diamonds does CP437 work?
        << "\x1b[6n";                    // cursor pos
  *this << door::nl << "\u2615"
        << "\x1b[6n";                   // hot beverage + cursor pos
  *this << "\x1b[999C\x1b[999B\x1b[6n"; // goto end of screen + cursor pos
  *this << reset << "\x1b[2J\x1b[H";    // reset, cls, go home

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

#ifdef DEBUG_OUTPUT
      for (x = 0; x < len; x++) {
        if (buffer[x] < 0x20)
          logf << std::hex << (int)buffer[x] << " ";
        else
          logf << buffer[x] << " ";
      }
#endif

      for (x = 0; x < len; x++) {
        if (buffer[x] == 0)
          buffer[x] = ' ';
      }

#ifdef DEBUG_OUTPUT
      logf << std::endl;
      logf << "BUFFER [" << (char *)buffer << "]" << std::endl;
#endif

      // log detection results
      if (1) {
        std::string cleanbuffer = buffer;
        std::string esc = "\x1b";
        std::string esc_text = "^[";

        while (replace(cleanbuffer, esc, esc_text)) {
        };

        logf << "BUFFER [" << cleanbuffer << "]" << std::endl;
      }

      // 1;3R required on David's machine.  I'm not sure why.
      // 1;3R also happens under VSCodium.
      // 1;4R is what I get from syncterm.

      if (((strstr(buffer, "1;1R") != nullptr) or
           (strstr(buffer, "1;3R") != nullptr)) and
          ((strstr(buffer, "2;2R") != nullptr) or
           (strstr(buffer, "2;3R") != nullptr))) {

        unicode = true;
        log() << "unicode enabled \u2615" << std::endl; // "U0001f926");
      } else {
        if (strstr(buffer, "1;3R") != nullptr) {
          full_cp437 = true;
        }
      }

      // Get the terminal screen size

      char *cp;
      cp = strrchr(buffer, '\x1b');

      if (cp != nullptr) {
        cp++;
        if (*cp == '[') {
          cp++;
          height = atoi(cp);
          cp = strchr(cp, ';');
          if (cp != nullptr) {
            cp++;
            width = atoi(cp);
            if (width > 900) {
              // something went wrong
              width = 0;
              height = 0;
            }
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

/**
 * @brief Load dropfile into dropfilelines and parse.
 *
 * Load dropfile and parse door.sys and door32.sys.
 *
 * @param[in] filepath Dropfile path and filename
 */
void Door::parse_dropfile(const char *filepath) {
  if (filepath == nullptr)
    return;

  // Ok, parse file here...
  std::ifstream file(filepath);

  if (file.fail()) {
    log() << "Failed to open " << filepath << std::endl;
    return;
  }

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
    if (filename == "door32.sys") {
      // https://raw.githubusercontent.com/NuSkooler/ansi-bbs/master/docs/dropfile_formats/door32_sys.txt
      // dropfilelines[0] = Comm type (0=local, 1=serial, 2=telnet)
      // dropfilelines[1] = Comm or Socket handle
      // dropfilelines[2] = BaudRate
      // dropfilelines[3] = BBS Software Version
      username = dropfilelines[4];
      handle = dropfilelines[5];
      time_left = atoi(dropfilelines[6].c_str());
      // dropfilelines[7] = Emulation (0=Ascii, 1=ANSI, .. or above = ANSI)
      node = atoi(dropfilelines[8].c_str());
    } else {
      std::string msg = "Unknown dropfile: ";
      msg += filename;
      log() << msg << std::endl;
      *this << msg << std::endl;
      exit(2);
    }
  }
  log() << "node:" << node << " username: " << username << " handle: " << handle
        << " time: " << time_left << std::endl;
  has_dropfile = true;
}

/**
 * @brief Give ofstream handle for logging
 *
 * This appends the current date/time stamp into the logfile, and returns a
 * reference.
 *
 * Example:
 *
 * door.log() << "Something bad just happened." << std::endl;
 *
 * @return ofstream&
 */
ofstream &Door::log(void) {
  std::time_t t = std::time(nullptr);
  std::tm tm = *std::localtime(&t);
  logf << std::put_time(&tm, "%c ") << node << " " << username << " ";
  return logf;
}

/**
 * @brief Are there any keys in STDIN?
 *
 * This uses select to check if we have received any keys.  This does not use
 * pushback.
 *
 * If HANGUP, OUTOFTIME, return true
 *
 * @return true
 * @return false
 */
bool Door::haskey(void) {
  fd_set socket_set;
  struct timeval tv;
  int select_ret = -1;

  if (hangup)
    return true; // HANGUP;

  if (time_left < 2)
    return true; // OUTOFTIME;

  while (select_ret == -1) {
    FD_ZERO(&socket_set);
    FD_SET(STDIN_FILENO, &socket_set);

    tv.tv_sec = 0;
    tv.tv_usec = 1;

    select_ret = select(STDIN_FILENO + 1, &socket_set, NULL, NULL, &tv);
    if (select_ret == -1) {
      if (errno == EINTR)
        continue;
      log() << "hangup detected" << std::endl;
      hangup = true;
      return true; // HANGUP;
    }
    if (select_ret == 0)
      return false;
  }
  return true;
}

/**
 * @brief low level read key.
 *
 * Returns key, or
 * -1 no key available/timeout
 * -2 read error/hang up
 * -3 out of time
 *
 * @return signed int
 */
signed int Door::getch(void) {
  fd_set socket_set;
  struct timeval tv;
  int select_ret = -1;
  int recv_ret;
  char key;

  if (door::hangup)
    return HANGUP;

  if (time_left < 2)
    return OUTOFTIME;

  while (select_ret == -1) {
    FD_ZERO(&socket_set);
    FD_SET(STDIN_FILENO, &socket_set);

    // This delay isn't long enough for QModem in a DOSBOX.
    // doorway mode arrow keys aren't always caught.
    tv.tv_sec = 0;
    tv.tv_usec = 100;
    // tv.tv_usec = 500;

    select_ret = select(STDIN_FILENO + 1, &socket_set, NULL, NULL, &tv);
    // select(STDIN_FILENO + 1, &socket_set, NULL, NULL, bWait ? NULL : &tv);
    if (select_ret == -1) {
      if (errno == EINTR)
        continue;
      log() << "hangup detected" << std::endl;
      door::hangup = true;
      return HANGUP;
    }
    if (select_ret == 0)
      return TIMEOUT;
  }

  recv_ret = read(STDIN_FILENO, &key, 1);
  if (recv_ret != 1) {
    // possibly log this.
    log() << "hangup" << std::endl;
    hangup = true;
    return HANGUP;
  }
  // debug weird keys/layouts.
  // log() << "read " << std::hex << (int)key << std::endl;
  return key;
}

/**
 * Call low level getch if the pushback buffer is empty.
 *
 * This allows testing of the \ref Door::getkey function.
 *
 * @return signed int
 */
signed int Door::getkey_or_pushback(void) {
  signed int c;
  if (!door::pushback.empty()) {
    c = door::pushback.front();
    door::pushback.pop_front();
    return c;
  }
  return getch();
}

/**
 * @brief Get a key routine.
 *
 * This returns the key received, or XKEY_* values for function keys, etc.
 * If return < 0:
 * -1 timeout/no key
 * -2 hangup
 * -3 out of time
 *
 * @return signed int
 */
signed int Door::getkey(void) {
  signed int c, c2;

  c = getkey_or_pushback();

  if (c < 0)
    return c;

  /*
  We get 0x0d 0x00 for [Enter] (Syncterm)
  From David's syncterm, I'm getting 0x0d 0x0a.
  This strips out the null.
  */
  if (c == 0x0d) {
    c2 = getkey_or_pushback();
    if ((c2 != 0) and (c2 >= 0) and (c2 != 0x0a)) {
      log() << "got " << (int)c2 << " so stuffing it into unget buffer."
            << std::endl;
      door::pushback.push_front(c2);
    }
    return c;
  }

  if (c == 0) {
    // possibly "doorway mode"
    int tries = 0;

    c2 = getkey_or_pushback();
    while (c2 < 0) {
      if (tries > 7) {
        log() << "ok, got " << c2 << " and " << tries << " so returning 0x00!"
              << std::endl;

        return c;
      }
      c2 = getkey_or_pushback();
      ++tries;
    }

    if (tries > 0) {
      log() << "tries " << tries << std::endl;
    }

    switch (c2) {
    case 0x50:
      return XKEY_DOWN_ARROW;
    case 0x48:
      return XKEY_UP_ARROW;
    case 0x4b:
      return XKEY_LEFT_ARROW;
    case 0x4d:
      return XKEY_RIGHT_ARROW;
    case 0x47:
      return XKEY_HOME;
    case 0x4f:
      return XKEY_END;
    case 0x49:
      return XKEY_PGUP;
    case 0x51:
      return XKEY_PGDN;
    case 0x3b:
      return XKEY_F1;
    case 0x3c:
      return XKEY_F2;
    case 0x3d:
      return XKEY_F3;
    case 0x3e:
      return XKEY_F4;
    case 0x3f:
      return XKEY_F5;
    case 0x40:
      return XKEY_F6;
    case 0x41:
      return XKEY_F7;
    case 0x42:
      return XKEY_F8;
    case 0x43:
      return XKEY_F9;
    case 0x44:
      return XKEY_F10;
      /*
    case 0x45:
      return XKEY_F11;
    case 0x46:
      return XKEY_F12;
      */
    case 0x52:
      return XKEY_INSERT;
    case 0x53:
      return XKEY_DELETE;
    }
    logf << "\r\nDEBUG:\r\n0x00 + 0x" << std::hex << (int)c2;
    logf << "\r\n";
    logf.flush();
  }

  if (c == 0x1b) {
    // possible extended key
    c2 = getkey_or_pushback();
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
        door::pushback.push_front(c2);
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

/**
 * @brief Waits secs seconds for a keypress.
 *
 * returns key, or -1 on timeout (seconds passed).
 * -2 hangup
 * -3 out of time
 *
 * @param secs
 * @return signed int
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
    return HANGUP;

  if (time_left < 2)
    return OUTOFTIME;

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
      log() << "hangup detected" << std::endl;
      return HANGUP;
    }
    if (select_ret == 0)
      return TIMEOUT;
  }
  return getkey();
}

/**
 * @brief Waits miliseconds for a keypress.
 *
 * returns key, or -1 on timeout (seconds passed).
 * -2 hangup
 * -3 out of time
 *
 * @param msecs
 * @return signed int
 */
signed int Door::sleep_ms_key(int msecs) {
  fd_set socket_set;
  struct timeval tv;
  int select_ret = -1;
  /*
  int recv_ret;
  char key;
  */
  if (hangup)
    return HANGUP;

  if (time_left < 2)
    return OUTOFTIME;

  while (select_ret == -1) {
    FD_ZERO(&socket_set);
    FD_SET(STDIN_FILENO, &socket_set);

    tv.tv_sec = 0;
    tv.tv_usec = msecs * 1000;

    select_ret = select(STDIN_FILENO + 1, &socket_set, NULL, NULL, &tv);
    // select(STDIN_FILENO + 1, &socket_set, NULL, NULL, bWait ? NULL : &tv);
    if (select_ret == -1) {
      if (errno == EINTR)
        continue;
      hangup = true;
      log() << "hangup detected" << std::endl;
      return HANGUP;
    }
    if (select_ret == 0)
      return TIMEOUT;
  }
  return getkey();
}

/**
 * @brief Input a string of requested max length.
 *
 * This first sends out max number of spaces, and max number of backspaces. This
 * will setup the input area.  (If you set a background color of blue, this
 * would allow that to be seen by the user.)
 *
 * It handles input, backspaces / deleting the characters / enter input and
 * timeout/hangup/out of time.
 *
 * @param max
 * @return std::string
 */
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
 * @brief Get one of these keys
 *
 * returns char, or < 0 if timeout.
 *
 * @param keys
 * @return char or < 0
 */
int Door::get_one_of(const char *keys) {
  int c;
  while (true) {
    c = sleep_key(inactivity);
    if (c < 0)
      return c;

    if (c > 0x1000)
      continue;
    const char *key = strchr(keys, (char)toupper(c));
    if (key != nullptr) {
      return *key;
    }
    *this << '\x07';
  }
  return c;
}

/**
 * Take given buffer and output it.
 *
 * If debug_capture is enabled, we save everything to debug_buffer.
 * This is used by the tests.
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
int Door::overflow(int c) {
  if (debug_capture) {
    debug_buffer.append(1, (char)c);
  } else {
    if (!hangup) {
      std::cout << (char)c;
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
 * Create render output.
 *
 * Call this for each section you want to colorize.
 *
 * @param color
 * @param len
 */
void Render::append(ANSIColor color, int len) {
  if (outputs.empty()) {
    ColorOutput co;
    co.c = color;
    co.pos = 0;
    co.len = len;
    outputs.push_back(co);
    return;
  }
  ColorOutput &current = outputs.back();
  if (current.c == color) {
    current.len += len;
    return;
  }
  // Ok, new entry
  // beware the unicode text
  ColorOutput co;
  co.pos = current.pos + current.len;
  co.c = color;
  co.len = len;
  outputs.push_back(co);
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

void Goto::set(int xpos, int ypos) {
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

    if (g.x > 1) {
      os << ";";
      *d << std::to_string(g.x);
    }
    *d << "H";
    d->cx = g.x;
    d->cy = g.y;
    d->track = true;
  } else {
    os << "\x1b[" << std::to_string(g.y) << ";" << std::to_string(g.x) << "H";
  };
  return os;
}

/**
 * @brief ANSI Save Cursor position command.
 */
const char SaveCursor[] = "\x1b[s";
/**
 * @brief ANSI Restore Cursor position command.
 */
const char RestoreCursor[] = "\x1b[u";

// EXAMPLES

/// BlueYellow Render example function
renderFunction rBlueYellow = [](const std::string &txt) -> Render {
  Render r(txt);

  ANSIColor blue(COLOR::BLUE, ATTR::BOLD);
  ANSIColor cyan(COLOR::YELLOW, ATTR::BOLD);

  for (char const &c : txt) {
    if (isupper(c))
      r.append(blue);
    else
      r.append(cyan);
  }
  return r;
};

} // namespace door
