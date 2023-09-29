#pragma once

#include <rtl-sdr.h>

#include <stdint.h>
#include <atomic>

using CtrlFlagT = uint32_t;

struct CtrlFlags
{
  static constexpr CtrlFlagT freq = 1;
  static constexpr CtrlFlagT srate = 2;
  static constexpr CtrlFlagT rf_gain = 4;
  static constexpr CtrlFlagT rf_agc = 8;              // tuner rf agc
  static constexpr CtrlFlagT rtl_agc = 16;
  static constexpr CtrlFlagT sampling_mode = 32;      // (direct) sampling mode
  static constexpr CtrlFlagT offset_tuning = 64;      // for E4000
  static constexpr CtrlFlagT ppm_correction = 128;    // freq corr ppm
  static constexpr CtrlFlagT tuner_bandwidth = 256;
  static constexpr CtrlFlagT tuner_sideband = 1024;
  static constexpr CtrlFlagT tuner_band_center = 2048;
  static constexpr CtrlFlagT if_agc_gain = 4096;      // tuner if agc / if gain
  static constexpr CtrlFlagT gpio = 8192;             // gpio changed
  static constexpr CtrlFlagT band_changed = 16384;
  static constexpr CtrlFlagT rtl_impulse_nc = 32768;
  static constexpr CtrlFlagT everything = 65536;      // command everything
};


struct ControlVars
{
  static constexpr unsigned NUM_GPIO_BUTTONS = 5;

  ControlVars(bool init_next)
  {
    for (unsigned k = 0; k < NUM_GPIO_BUTTONS; ++k)
      GPIO[k] = (init_next) ? 0 : -9;
  }

  std::atomic_int64_t LO_freq = 100000000;
  std::atomic_int64_t tune_freq = 0;  // absolute (RF) frequency of tuned frequency
  std::atomic_int srate_idx = 21;     // default = 2.3 MSps
  std::atomic_int tuner_bw = 0;       // 0 == automatic, sonst in Hz
                          // n_bandwidths = bandwidths[]; nearestBwIdx()
  std::atomic_int decimation = 1;
  std::atomic_int rf_gain = 1;
  std::atomic_int if_gain_val = 1;
  std::atomic_int if_gain_idx = 1;
  std::atomic_int tuner_rf_agc = 1;   // 0 == off/manual, 1 == on/automatic
  std::atomic_int tuner_if_agc = 0;   // 0 == off/manual, 1 == on/automatic
  std::atomic_int rtl_agc = 1;
  std::atomic_int sampling_mode = 0;
  std::atomic_int band_center_sel = 0;
  std::atomic_int band_center_LO_delta = 0;
  std::atomic_int offset_tuning = 0;
  std::atomic_int USB_sideband = 0;
  std::atomic_int freq_corr_ppm = 0;
  std::atomic_int GPIO[NUM_GPIO_BUTTONS];

  std::atomic_int rtl_impulse_noise_cancellation = -1;

  std::atomic_int rtl_aagc_rf_en = -1;
  std::atomic_int rtl_aagc_rf_inv = -1;
  std::atomic_int rtl_aagc_rf_min = -1;
  std::atomic_int rtl_aagc_rf_max = -1;

  std::atomic_int rtl_aagc_if_en = -1;
  std::atomic_int rtl_aagc_if_inv = -1;
  std::atomic_int rtl_aagc_if_min = -1;
  std::atomic_int rtl_aagc_if_max = -1;

  std::atomic_int rtl_aagc_lg_lock = -1;  // lg: loop gain
  std::atomic_int rtl_aagc_lg_unlock = -1;
  std::atomic_int rtl_aagc_lg_ifr = -1;  // ifr: interference

  std::atomic_int rtl_aagc_vtop[3] = { -1, -1, -1 };
  std::atomic_int rtl_aagc_krf[4] = { -1, -1, -1, -1 };
};

extern std::atomic_int GPIO_pin[ControlVars::NUM_GPIO_BUTTONS];
extern std::atomic_int GPIO_inv[ControlVars::NUM_GPIO_BUTTONS];
extern std::atomic_int GPIO_en[ControlVars::NUM_GPIO_BUTTONS];
extern char GPIO_txt[ControlVars::NUM_GPIO_BUTTONS][16];

extern ControlVars last;
extern ControlVars nxt;
extern std::atomic<CtrlFlagT> somewhat_changed;
extern std::atomic_bool commandEverything;

extern const int* bandwidths;
extern const int* rf_gains;
extern const int* if_gains;
extern int n_bandwidths;
extern int n_rf_gains;
extern int n_if_gains;

struct RtlDeviceInfo
{
  RtlDeviceInfo() {
    clear();
  }

  void clear()
  {
    dev_idx = -1;
    memset(vendor, 0, sizeof(vendor));
    memset(product, 0, sizeof(product));
    memset(serial, 0, sizeof(serial));
    memset(name, 0, sizeof(name));
  }

  static bool is_same(const RtlDeviceInfo& A, const RtlDeviceInfo& B);

  uint32_t dev_idx;
  char vendor[256];
  char product[256];
  char serial[256];
  char name[256];
};

static constexpr unsigned MAX_RTL_DEVICES = 16;

extern RtlDeviceInfo RtlDeviceList[MAX_RTL_DEVICES];
extern RtlDeviceInfo RtlOpenDevice;
extern uint32_t RtlNumDevices;
extern uint32_t RtlSelectedDeviceIdx;  // index into RtlDeviceList[]

extern rtlsdr_dev_t* RtlSdrDev;

uint32_t retrieve_devices();
bool is_device_handle_valid();
void close_rtl_device();
bool open_selected_rtl_device();

bool Control_Changes();

extern std::atomic_uint32_t tunerNo;
extern std::atomic_bool GotTunerInfo;

int nearestBwIdx(int bw);
int nearestGainIdx(int gain, const int* gains, const int n_gains);

inline void trigger_control(CtrlFlagT f)
{
  somewhat_changed.fetch_or(f);
  Control_Changes();
}
