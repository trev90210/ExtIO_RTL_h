/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef EXTIO_RTL_H
#define EXTIO_RTL_H

#define EXTIO_RTL_NAME		"ExtIO RTL-SDR"

/* RTLSDR */
#define RTLSDR_MINPPM		(-999)
#define RTLSDR_MAXPPM		999
#define RTLSDR_MINHISRATE	900001
#define RTLSDR_MAXHISRATE	3200000

/* ExtIO HW Type Codes */
#define EXTIO_USBDATA_16	3

/* ExtIO Status Codes */
#define EXTIO_CHANGED_SR	100
#define EXTIO_CHANGED_LO	101
#define EXTIO_CHANGED_ATT	125

extern HMODULE hInst;

extern "C" void __stdcall CloseHW(void);
extern "C" int __stdcall ExtIoGetActualSrateIdx(void);
extern "C" int __stdcall ExtIoGetSetting(int idx, char *description, char *value);
extern "C" int __stdcall ExtIoGetSrates(int idx, double *samplerate);
extern "C" void __stdcall ExtIoSetSetting(int idx, const char *value);
extern "C" int __stdcall ExtIoSetSrate(int idx);
extern "C" int __stdcall GetActualAttIdx(void);
extern "C" int __stdcall GetAttenuators(int idx, float *attenuation);
extern "C" long __stdcall GetHWLO(void);
extern "C" long __stdcall GetHWSR(void);
extern "C" int __stdcall GetStatus(void);
extern "C" void __stdcall HideGUI(void);
extern "C" bool __stdcall InitHW(char *name, char *model, int &hwtype);
extern "C" bool __stdcall OpenHW(void);
extern "C" int __stdcall SetAttenuator(int idx);
extern "C" void __stdcall SetCallback(void (*ParentCallback)(int, int, float, void *));
extern "C" long __stdcall SetHWLO(long LOfreq);
extern "C" void __stdcall ShowGUI(void);
extern "C" int __stdcall StartHW(long LOfreq);
extern "C" void __stdcall StopHW(void);
extern "C" void __stdcall SwitchGUI(void);

#endif /* EXTIO_RTL_H */
