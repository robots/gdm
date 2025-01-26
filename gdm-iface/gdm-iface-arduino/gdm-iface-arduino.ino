// Make sure you have cut the reset jumper so the mcu doesn't reset on every port connection.
// If you do not do this the interface python tools will not work.

#include "cmd.h"

void setup() {
  Serial.begin(921600);

  cmd_init();
}

void loop() {
  cmd_periodic();
}
