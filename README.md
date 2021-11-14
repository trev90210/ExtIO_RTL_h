## ExtIO_RTL

ExtIO wrapper for librtlsdr for use with [**HDSDR**](https://hdsdr.de/) and other [**Winrad**](https://www.i2phd.org/winrad/) compatible programs

* Original code from [**jorgem-seq/ExtIO_RTL**](https://github.com/jorgem-seq/ExtIO_RTL), which was derived from [**josemariaaraujo/ExtIO_RTL**](https://github.com/josemariaaraujo/ExtIO_RTL)
* removed makefiles and added cmake support to compile with Visual Studio 2019
* added all required libraries as submodules
* added github action for automatic build
* merged 'Add Bias Tee option, add branch auto switch' from Jorge's testing branch

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
