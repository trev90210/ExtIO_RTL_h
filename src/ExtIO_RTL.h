/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef EXTIO_RTL_H
#define EXTIO_RTL_H

#include <stdint.h>

#define ARRAY_SIZE(arr)		(sizeof(arr) / sizeof((arr)[0]))
#define ARRAY_SSIZE(arr)	((SSIZE_T)ARRAY_SIZE(arr))

#define EXTIO_RTL_MAXN		16
#define EXTIO_RTL_NAME		"ExtIO RTL-E4000"
#define EXTIO_RTL_ERROR(str)	MessageBox(NULL, TEXT(str), TEXT(EXTIO_RTL_NAME), \
					   MB_OK | MB_ICONERROR)

/* RTLSDR */
#define RTLSDR_MINPPM		(-999)
#define RTLSDR_MAXPPM		999
#define RTLSDR_MINLOSRATE	225001
#define RTLSDR_MAXLOSRATE	300000
#define RTLSDR_MINHISRATE	900001
#define RTLSDR_MAXHISRATE	3200000
#define RTLSDR_MAXTUNERGAINS	32

/* ExtIO HW Type Codes */
#define EXTIO_USBDATA_16	3
#define EXTIO_USBDATA_U8	9

/* ExtIO SDR Info Codes */
#define EXTIO_SUPPORTS_U8	7

/* ExtIO Status Codes */
#define EXTIO_CHANGED_SR	100
#define EXTIO_CHANGED_LO	101
#define EXTIO_CHANGED_ATT	125
#define EXTIO_CHANGED_RF_IF	136

#define EXTIO_SET_STATUS(EXTIO_CB, EXTIO_CMD)	EXTIO_CB(-1, EXTIO_CMD, 0, NULL)

// E4000
enum e4k_if_gain_mode {
	E4K_GAIN_MODE_DEFAULT		= 0,
	E4K_GAIN_MODE_LINEARITY		= 1,
	E4K_GAIN_MODE_SENSITIVITY	= 2,
};

extern HMODULE hInst;

extern "C" void __stdcall CloseHW(void);
extern "C" int __stdcall ExtIoGetActualMgcIdx(void);
extern "C" int __stdcall ExtIoGetActualSrateIdx(void);
extern "C" int __stdcall ExtIoGetMGCs(int idx, float *gain);
extern "C" int __stdcall ExtIoGetSetting(int idx, char *description, char *value);
extern "C" int __stdcall ExtIoGetSrates(int idx, double *samplerate);
extern "C" void __stdcall ExtIoSDRInfo(int SDRInfo, int /* additionalValue */, void * /* additionalPtr */);
extern "C" int __stdcall ExtIoSetMGC(int idx);
extern "C" void __stdcall ExtIoSetSetting(int idx, const char *value);
extern "C" int __stdcall ExtIoSetSrate(int idx);
extern "C" int __stdcall GetActualAttIdx(void);
extern "C" int __stdcall GetAttenuators(int idx, float *attenuation);
extern "C" int64_t __stdcall GetHWLO64(void);
extern "C" long __stdcall GetHWLO(void);
extern "C" long __stdcall GetHWSR(void);
extern "C" int __stdcall GetStatus(void);
extern "C" void __stdcall HideGUI(void);
extern "C" bool __stdcall InitHW(char *name, char *model, int &hwtype);
extern "C" bool __stdcall OpenHW(void);
extern "C" int __stdcall SetAttenuator(int idx);
extern "C" void __stdcall SetCallback(void (*ParentCallback)(int, int, float, void *));
extern "C" int64_t __stdcall SetHWLO64(int64_t LOfreq);
extern "C" long __stdcall SetHWLO(long LOfreq);
extern "C" void __stdcall ShowGUI(void);
extern "C" int __stdcall StartHW64(int64_t LOfreq);
extern "C" int __stdcall StartHW(long LOfreq);
extern "C" void __stdcall StopHW(void);
extern "C" void __stdcall SwitchGUI(void);

static inline int32_t ppm_validate(int32_t ppm)
{
	if (ppm < RTLSDR_MINPPM)
		return RTLSDR_MINPPM;
	if (ppm > RTLSDR_MAXPPM)
		return RTLSDR_MAXPPM;
	return ppm;
}

static inline uint32_t srate_validate(uint32_t srate)
{
	if (srate < RTLSDR_MINLOSRATE)
		return RTLSDR_MINLOSRATE;
	if (srate > RTLSDR_MAXLOSRATE && srate < RTLSDR_MINHISRATE)
		return RTLSDR_MINHISRATE;
	if (srate > RTLSDR_MAXHISRATE)
		return RTLSDR_MAXHISRATE;
	return srate;
}

#endif /* EXTIO_RTL_H */
