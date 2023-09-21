## ExtIO_RTL

ExtIO wrapper for librtlsdr for use with [**HDSDR**](https://hdsdr.de/) and other [**Winrad**](https://www.i2phd.org/winrad/) compatible programs

* Original code from [**jorgem-seq/ExtIO_RTL**](https://github.com/jorgem-seq/ExtIO_RTL), which was derived from [**josemariaaraujo/ExtIO_RTL**](https://github.com/josemariaaraujo/ExtIO_RTL)
* removed makefiles and added cmake support to compile with Visual Studio 2019
* added all required libraries as submodules
* added github action for automatic build
* merged 'Add Bias Tee option, add branch auto switch' from Jorge's testing branch
* static build: doesn't need any special DLLs

Additional features with v2023 and later releases:

* enhanced features for R860 or R820T/2 and also R828D tuners:
  - Narrow tuner bandwidths - downto ~ 300 kHz - improving dynamic and 'sensivity' by removing unnecessary signals
  - Band Center configuration to get rid of the received DC from the 'signal of interest'
  - Tune USB band: mirroring the spectrum - switches the band corner, where the steeper low pass filter is applied
  - RF gain: controls LNA and mixer
  - IF gain: controls VGA - with feedback from RTL2832U in case of enabled AGC
  - supports the RTL-SDR blog V4 dongle - based on the R828D tuner and including an HF upconverter. see  https://www.rtl-sdr.com/rtl-sdr-blog-v4-dongle-initial-release/
  - note: keep the IF-AGC / Auto-VGA off for R828D based tuners: it often produces 'pumping'
* explicit GPIO controls in the GUI
* band configurations enabled by editing rtl_sdr_extio.cfg in your %USERPROFILE% directory
  - default/demo configuration is generated at first start
  - it's disabled by default
  - should be self-explanatory with comment lines

Setup: simply put/copy the ExtIO_RTL.dll alongside to the HDSDR executable.


Clone with submodules to automatically retrieve libusb, pthread-win32 and librtlsdr from git bash:

```
git clone -b cmake_github_action --recursive https://github.com/hayguen/ExtIO_RTL.git
```

Manual compilation from Visual Studio 2019 Command Prompt:
```
cmake -G "Visual Studio 16 2019" -A Win32 -S ExtIO_RTL -B build_ExtIO_RTL
cmake --build build_ExtIO_RTL --config Release --target ExtIO_RTL
```

A pre-built binary should be available in github Actions.

### LICENSE

GPL-2.0, see [**LICENSE file**](COPYING)
