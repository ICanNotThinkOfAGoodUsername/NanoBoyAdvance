/*
 * Copyright (C) 2021 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <nba/rom/gpio/gpio.hpp>

#include "hw/irq/irq.hpp"

namespace nba {

struct RTC : GPIO {
  enum class Port {
    SCK,
    SIO,
    CS
  };

  enum class State {
    Command,
    Sending,
    Receiving,
    Complete
  };

  enum class Register {
    ForceReset = 0,
    DateTime = 2,
    ForceIRQ = 3,
    Control = 4,
    Time = 6,
    Free = 7
  };

  RTC(core::IRQ& irq) : irq(irq) {
    Reset();
  }

  void Reset();

protected:
  auto ReadPort() -> u8 final;
  void WritePort(u8 value) final;

private:
  bool ReadSIO();
  void ReceiveCommandSIO();
  void ReceiveBufferSIO();
  void TransmitBufferSIO();
  void ReadRegister();
  void WriteRegister();

  static auto ConvertDecimalToBCD(u8 x) -> u8 {
    u8 y = 0;
    u8 e = 1;

    while (x > 0) {
      y += (x % 10) * e;
      e *= 16;
      x /= 10; 
    }

    return y;
  }

  int current_bit;
  int current_byte;

  Register reg;
  u8 data;
  u8 buffer[7];

  struct PortData {
    int sck;
    int sio;
    int cs;
  } port;

  State state;

  struct ControlRegister {
    bool unknown;
    bool per_minute_irq;
    bool mode_24h;
    bool poweroff;

    void Reset() {
      unknown = false;
      per_minute_irq = false;
      mode_24h = false;
      poweroff = false;
    }
  } control;

  core::IRQ& irq;

  static constexpr int s_argument_count[8] = {
    0, // ForceReset
    0, // Unused?
    7, // DateTime
    0, // ForceIRQ
    1, // Control
    0, // Unused?
    3, // Time
    0  // FreeRegister
  };
};

} // namespace nba
