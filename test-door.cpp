#include "door.h"
#include "gtest/gtest.h"

namespace {

class DoorTest : public ::testing::Test {
protected:
  void SetUp() override {
    int argc = 5;
    char argv0[] = "./test";
    char argv1[] = "-l";
    char argv2[] = "-u";
    char argv3[] = "test";
    char argv4[] = "--debuggering";
    char *argv[] = {argv0, argv1, argv2, argv3, argv4};

    door::debug_capture = true;
    d = new door::Door("test", argc, argv);

    d->debug_buffer.clear();
  }

  void TearDown() override {
    delete d;
    d = nullptr;
  }

public:
  door::Door *d;
};

TEST_F(DoorTest, BasicColorOut1) {
  door::ANSIColor BonY(door::COLOR::BLUE, door::COLOR::YELLOW);
  char BLUE_ON_YELLOW[] = "\x1b[34;43m";
  *d << BonY;
  EXPECT_STREQ(d->debug_buffer.c_str(), BLUE_ON_YELLOW);
  d->debug_buffer.clear();

  door::ANSIColor YonB(door::COLOR::YELLOW, door::COLOR::BLUE,
                       door::ATTR::BOLD);
  char Y_ON_B[] = "\x1b[1;33;44m";
  *d << YonB;

  EXPECT_STREQ(d->debug_buffer.c_str(), Y_ON_B);

  *d << door::reset;
  d->debug_buffer.clear();
  // Without the reset, an extra 0; gets added to the output.

  door::ANSIColor GonR(door::COLOR::GREEN, door::COLOR::RED, door::ATTR::BLINK);
  char G_ON_R[] = "\x1b[5;32;41m";

  *d << GonR;
  EXPECT_STREQ(d->debug_buffer.c_str(), G_ON_R);
  d->debug_buffer.clear();
}

TEST_F(DoorTest, ResetOutput) {
  *d << door::reset;

  EXPECT_STREQ(d->debug_buffer.c_str(), "\x1b[0m");
  d->debug_buffer.clear();

  *d << door::nl;
  EXPECT_STREQ(d->debug_buffer.c_str(), "\r\n");
  d->debug_buffer.clear();

  *d << door::cls;
  // CLS + GOTO
  EXPECT_STREQ(d->debug_buffer.c_str(), "\x1b[2J\x1b[H");
  d->debug_buffer.clear();
}

TEST_F(DoorTest, ColorOptimizeOut1) {
  door::ANSIColor RonB(door::COLOR::RED, door::COLOR::BLUE);
  char R_ON_B[] = "\x1b[31;44m";
  *d << RonB;
  EXPECT_STREQ(d->debug_buffer.c_str(), R_ON_B);
  d->debug_buffer.clear();

  door::ANSIColor GonB(door::COLOR::GREEN, door::COLOR::BLUE);
  char G_ON_B[] = "\x1b[32m";
  *d << GonB;

  EXPECT_STREQ(d->debug_buffer.c_str(), G_ON_B);
  d->debug_buffer.clear();
}

TEST_F(DoorTest, GotoOutput) {
  door::Goto pos(1, 1);
  *d << pos;

  EXPECT_STREQ(d->debug_buffer.c_str(), "\x1b[H");
  d->debug_buffer.clear();

  pos.set(5, 10);
  *d << pos;

  EXPECT_STREQ(d->debug_buffer.c_str(), "\x1b[10;5H");
  d->debug_buffer.clear();

  pos.set(5, 1);
  *d << pos;

  EXPECT_STREQ(d->debug_buffer.c_str(), "\x1b[;5H");
  d->debug_buffer.clear();

  pos.set(1, 10);
  *d << pos;

  EXPECT_STREQ(d->debug_buffer.c_str(), "\x1b[10H");
  d->debug_buffer.clear();
}

TEST_F(DoorTest, LineOutput) {
  door::Line line("Meow");
  *d << line;

  EXPECT_STREQ(d->debug_buffer.c_str(), "Meow");
  d->debug_buffer.clear();

  door::Line color("Cat", 4,
                   door::ANSIColor(door::COLOR::BLACK, door::COLOR::WHITE));
  *d << color;
  EXPECT_STREQ(d->debug_buffer.c_str(), "\x1b[30;47mCat ");

  *d << door::reset;
  d->debug_buffer.clear();

  door::Line pad("Test", 4, door::ANSIColor(door::COLOR::RED));
  pad.setPadding("**", door::ANSIColor(door::COLOR::GREEN));
  *d << pad;

  EXPECT_STREQ(d->debug_buffer.c_str(), "\x1b[32m**\x1b[31mTest\x1b[32m**");
  d->debug_buffer.clear();
}

} // namespace