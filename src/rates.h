#pragma once

struct rates
{

  struct sr_t {
    double value;
    const char* name;
    int    valueInt;
  };

  // valid sample rates: 225001 - 300000 Hz, 900001 - 3200000 Hz
  static constexpr unsigned MIN = 900001;  // 900 kHz + 1 Hz
  static constexpr unsigned MAX = 3200000;  // 3200 kHz
  static constexpr unsigned N = 31;
  static const sr_t tab[N];
};
