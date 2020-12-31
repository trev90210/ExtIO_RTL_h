// SPDX-FileCopyrightText: 2013 José Araújo <josemariaaraujo@gmail.com>
// SPDX-FileCopyrightText: 2020 Jorge Maidana <jorgem.seq@gmail.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// ExtIO_RTL.cpp - ExtIO wrapper for librtlsdr

#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <process.h>
#include <tchar.h>
#include <new>

#include "rtl-sdr.h"
#include "ExtIO_RTL.h"
#include "resource.h"

static HWND h_dialog;
static INT_PTR CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);

typedef struct {
	const double value;
	const TCHAR *name;
} RtlSdrSampleRate;

static RtlSdrSampleRate RtlSdrSampleRateArr[] = {
	{  250000.0, TEXT("0.25 Msps") },
	{  960000.0, TEXT("0.96 Msps") },
	{ 1028571.0, TEXT("1.02 Msps") },
	{ 1200000.0, TEXT("1.2 Msps")  },
	{ 1440000.0, TEXT("1.44 Msps") },
	{ 1800000.0, TEXT("1.8 Msps")  },
	{ 2400000.0, TEXT("2.4 Msps")  },
	{ 2880000.0, TEXT("2.88 Msps") },
	{ 3200000.0, TEXT("3.2 Msps")  }
};

static const TCHAR *RtlSdrDirSamplingArr[] = {
	TEXT("Disabled"),
	TEXT("I input"),
	TEXT("Q input")
};

// ExtIO Options
static int ExtIOSampleRate = 6;     // id: 00 default: 2.4 Msps
static int ExtIOTunerAGC = 1;       // id: 01 default: Enabled
static int ExtIORTLAGC = 0;         // id: 02 default: Disabled
static int ExtIOFreqCorrection = 0; // id: 03 default: 0
static int ExtIOTunerGain = 0;      // id: 04 default: 0
static int ExtIOBufferSize = 6;     // id: 05 default: 64 KiB
static int ExtIOOffsetTuning = 1;   // id: 06 default: Enabled
static int ExtIODirSampling = 0;    // id: 07 default: Disabled
static uint32_t ExtIODevIdx = 0;    // id: 08 default: 0

// Device
static rtlsdr_dev_t *RtlSdrDev;
static uint32_t RtlSdrDevCount;

// Tuner
static int *RtlSdrTunerGainsArr;
static int RtlSdrTunerGainsCount;
static int RtlSdrTunerGain;
static int RtlSdrPllLocked; // 0 = Locked

// Buffer
static const uint32_t RtlSdrBufSizeArr[] = { 1, 2, 4, 8, 16, 32, 64, 128, 256 }; // In KiB
static uint32_t RtlSdrBufSize;
static short *RtlSdrBufShort;

static HANDLE RtlSdrHandle = INVALID_HANDLE_VALUE;
static void RtlSdrThreadProc(void * /* param */);
static int RtlSdrThreadStart(void);
static int RtlSdrThreadStop(void);
static void RtlSdrCallback(unsigned char *buf, uint32_t len, void * /* ctx */); // RTL-SDR Callback

static void (*ExtIOCallback)(int, int, float, void *); // ExtIO Callback

extern "C" bool __stdcall InitHW(char *name, char *model, int &hwtype)
{
	RtlSdrDevCount = rtlsdr_get_device_count();
	if (!RtlSdrDevCount)
		goto fail;

	strcpy_s(name, 16, EXTIO_RTL_NAME);
	strcpy_s(model, 16, "USB");
	hwtype = EXTIO_USBDATA_16;
	return TRUE;

fail:
	MessageBox(NULL, TEXT("No RTL-SDR devices found"),
		   TEXT(EXTIO_RTL_NAME), MB_OK | MB_ICONERROR);
	return FALSE;
}

extern "C" int __stdcall GetStatus(void)
{
	return 0;
}

extern "C" bool __stdcall OpenHW(void)
{
	int ret;

	if (ExtIODevIdx >= RtlSdrDevCount)
		ExtIODevIdx = 0;
	ret = rtlsdr_open(&RtlSdrDev, ExtIODevIdx);
	if (ret < 0)
		return FALSE;
	ret = rtlsdr_set_sample_rate(RtlSdrDev, (uint32_t)RtlSdrSampleRateArr[ExtIOSampleRate].value);
	if (ret < 0)
		return FALSE;

	h_dialog = CreateDialog(hInst, MAKEINTRESOURCE(DLG_MAIN),
				NULL, (DLGPROC)MainDlgProc);
	ShowWindow(h_dialog, SW_HIDE);
	return TRUE;
}

extern "C" int64_t __stdcall SetHWLO64(int64_t LOfreq)
{
	int ExtIOPllLocked = 1;
	int64_t ret = 0;

	if ((LOfreq < 1) || (LOfreq > (int64_t)0xffffffff)) {
		ret = -1;
	} else {
		ExtIOPllLocked = rtlsdr_set_center_freq(RtlSdrDev,
					     (uint32_t)(LOfreq & 0xffffffff));
		if (ExtIOPllLocked)
			ret = -1;
	}

	if (ExtIOPllLocked != RtlSdrPllLocked) {
		RtlSdrPllLocked = ExtIOPllLocked;
		if (!RtlSdrPllLocked)
			Static_SetText(GetDlgItem(h_dialog, IDC_TUNER_PLL),
				       TEXT("PLL LOCKED"));
		else
			Static_SetText(GetDlgItem(h_dialog, IDC_TUNER_PLL),
				       TEXT("PLL NOT LOCKED"));

		InvalidateRect(h_dialog, NULL, TRUE);
		UpdateWindow(h_dialog);
	}
	if (ret)
		return ret;
	if (LOfreq != GetHWLO64())
		ExtIOCallback(-1, EXTIO_CHANGED_LO, 0, NULL);
	return 0;
}

extern "C" long __stdcall SetHWLO(long LOfreq)
{
	int64_t ret = SetHWLO64((int64_t)(unsigned long)LOfreq);

	return (long)(ret & 0xffffffff);
}

extern "C" int __stdcall StartHW64(int64_t LOfreq)
{
	if (!RtlSdrDev)
		return -1;

	RtlSdrBufShort = new(std::nothrow) short[RtlSdrBufSize];
	if (!RtlSdrBufShort) {
		MessageBox(NULL, TEXT("Couldn't allocate buffer!"),
			   TEXT(EXTIO_RTL_NAME), MB_OK | MB_ICONERROR);
		return -1;
	}

	if (RtlSdrThreadStart() < 0) {
		delete[] RtlSdrBufShort;
		RtlSdrBufShort = NULL;
		return -1;
	}

	SetHWLO64(LOfreq);
	EnableWindow(GetDlgItem(h_dialog, IDC_RTL_BUFFER), FALSE);
	EnableWindow(GetDlgItem(h_dialog, IDC_RTL_DEVICE), FALSE);
	return (int)(RtlSdrBufSize / 2);
}

extern "C" int __stdcall StartHW(long LOfreq)
{
	return StartHW64((int64_t)(unsigned long)LOfreq);
}

extern "C" int64_t __stdcall GetHWLO64(void)
{
	int64_t LOfreq;
	static int64_t LastLOfreq = 100000000;

	LOfreq = (int64_t)rtlsdr_get_center_freq(RtlSdrDev);
	if (!LOfreq)
		return LastLOfreq;
	LastLOfreq = LOfreq;
	return LOfreq;
}

extern "C" long __stdcall GetHWLO(void)
{
	int64_t LOfreq = GetHWLO64();

	return (long)(LOfreq & 0x7fffffff);
}

extern "C" long __stdcall GetHWSR(void)
{
	return (long)rtlsdr_get_sample_rate(RtlSdrDev);
}

extern "C" int __stdcall ExtIoGetSrates(int idx, double *samplerate)
{
	if (idx < 0 || idx >= (sizeof(RtlSdrSampleRateArr) / sizeof(RtlSdrSampleRateArr[0])))
		return -1;

	*samplerate = RtlSdrSampleRateArr[idx].value;
	return 0;
}

extern "C" int __stdcall ExtIoGetActualSrateIdx(void)
{
	return ComboBox_GetCurSel(GetDlgItem(h_dialog, IDC_RTL_SAMPLE_RATE));
}

extern "C" int __stdcall ExtIoSetSrate(int idx)
{
	if (idx < 0 || idx >= (sizeof(RtlSdrSampleRateArr) / sizeof(RtlSdrSampleRateArr[0])))
		return -1;

	rtlsdr_set_sample_rate(RtlSdrDev, (uint32_t)RtlSdrSampleRateArr[idx].value);
	ComboBox_SetCurSel(GetDlgItem(h_dialog, IDC_RTL_SAMPLE_RATE), idx);
	ExtIOCallback(-1, EXTIO_CHANGED_SR, 0, NULL);
	return 0;
}

extern "C" int __stdcall GetAttenuators(int idx, float *attenuation)
{
	if (idx < 0 || idx >= RtlSdrTunerGainsCount)
		return -1;

	*attenuation = (float)(RtlSdrTunerGainsArr[idx] / 10.0);
	return 0;
}

extern "C" int __stdcall GetActualAttIdx(void)
{
	for (int i = 0; i < RtlSdrTunerGainsCount; i++) {
		if (RtlSdrTunerGain == RtlSdrTunerGainsArr[i])
			return i;
	}
	return -1;
}

extern "C" int __stdcall SetAttenuator(int idx)
{
	int pos;

	if (idx < 0 || idx >= RtlSdrTunerGainsCount)
		return -1;

	pos = RtlSdrTunerGainsArr[idx];
	SendMessage(GetDlgItem(h_dialog, IDC_TUNER_GAIN_CTL), TBM_SETPOS,
		   (WPARAM)TRUE, (LPARAM)-pos);
	if (Button_GetCheck(GetDlgItem(h_dialog, IDC_TUNER_AGC)) == BST_UNCHECKED) {
		TCHAR tunergain[256];

		_stprintf_s(tunergain, 256, TEXT("%2.1f dB"), (float)(pos / 10.0));
		Static_SetText(GetDlgItem(h_dialog, IDC_TUNER_GAIN), tunergain);
		if (pos != RtlSdrTunerGain)
			rtlsdr_set_tuner_gain(RtlSdrDev, pos);
	}
	RtlSdrTunerGain = pos;
	return 0;
}

extern "C" int __stdcall ExtIoGetSetting(int idx, char *description, char *value)
{
	switch (idx) {
	case 0:
		_snprintf(description, 1024, "%s", "SampleRate");
		_snprintf(value, 1024, "%d",
			  ComboBox_GetCurSel(GetDlgItem(h_dialog, IDC_RTL_SAMPLE_RATE)));
		return 0;
	case 1:
		_snprintf(description, 1024, "%s", "TunerAGC");
		_snprintf(value, 1024, "%d",
			  Button_GetCheck(GetDlgItem(h_dialog, IDC_TUNER_AGC)) ==
			  BST_CHECKED ? 1 : 0);
		return 0;
	case 2:
		_snprintf(description, 1024, "%s", "RTLAGC");
		_snprintf(value, 1024, "%d",
			  Button_GetCheck(GetDlgItem(h_dialog, IDC_RTL_AGC)) ==
			  BST_CHECKED ? 1 : 0);
		return 0;
	case 3: {
		TCHAR ppm[256];

		_snprintf(description, 1024, "%s", "FreqCorrection");
		Edit_GetText(GetDlgItem(h_dialog, IDC_RTL_PPM), ppm, 256);
		_snprintf(value, 1024, "%d", _ttoi(ppm));
		return 0;
	}
	case 4: {
		int pos = (int)-SendMessage(GetDlgItem(h_dialog, IDC_TUNER_GAIN_CTL),
					    TBM_GETPOS, (WPARAM)0, (LPARAM)0);

		_snprintf(description, 1024, "%s", "TunerGain");
		_snprintf(value, 1024, "%d", pos);
		return 0;
	}
	case 5:
		_snprintf(description, 1024, "%s", "BufferSize");
		_snprintf(value, 1024, "%d",
			  ComboBox_GetCurSel(GetDlgItem(h_dialog, IDC_RTL_BUFFER)));
		return 0;
	case 6:
		_snprintf(description, 1024, "%s", "OffsetTuning");
		_snprintf(value, 1024, "%d",
			  Button_GetCheck(GetDlgItem(h_dialog, IDC_TUNER_OFF_TUNING)) ==
			  BST_CHECKED ? 1 : 0);
		return 0;
	case 7:
		_snprintf(description, 1024, "%s", "DirectSampling");
		_snprintf(value, 1024, "%d",
			  ComboBox_GetCurSel(GetDlgItem(h_dialog, IDC_RTL_DIR_SAMPLING)));
		return 0;
	case 8:
		_snprintf(description, 1024, "%s", "Device");
		_snprintf(value, 1024, "%d",
			  ComboBox_GetCurSel(GetDlgItem(h_dialog, IDC_RTL_DEVICE)));
		return 0;
	}
	return -1;
}

extern "C" void __stdcall ExtIoSetSetting(int idx, const char *value)
{
	int ExtIOVal;

	switch (idx) {
	case 0:
		ExtIOVal = atoi(value);
		if (ExtIOVal >= 0 && ExtIOVal < (sizeof(RtlSdrSampleRateArr) / sizeof(RtlSdrSampleRateArr[0])))
			ExtIOSampleRate = ExtIOVal;
		break;
	case 1:
		ExtIOVal = atoi(value);
		ExtIOTunerAGC = ExtIOVal ? 1 : 0;
		break;
	case 2:
		ExtIOVal = atoi(value);
		ExtIORTLAGC = ExtIOVal ? 1 : 0;
		break;
	case 3:
		ExtIOVal = atoi(value);
		if (ExtIOVal > RTLSDR_MINPPM && ExtIOVal < RTLSDR_MAXPPM)
			ExtIOFreqCorrection = ExtIOVal;
		break;
	case 4:
		ExtIOVal = atoi(value);
		ExtIOTunerGain = ExtIOVal;
		break;
	case 5:
		ExtIOVal = atoi(value);
		if (ExtIOVal >= 0 && ExtIOVal < (sizeof(RtlSdrBufSizeArr) / sizeof(RtlSdrBufSizeArr[0])))
			ExtIOBufferSize = ExtIOVal;
		break;
	case 6:
		ExtIOVal = atoi(value);
		ExtIOOffsetTuning = ExtIOVal ? 1 : 0;
		break;
	case 7:
		ExtIOVal = atoi(value);
		ExtIODirSampling = ExtIOVal;
		break;
	case 8:
		ExtIOVal = atoi(value);
		ExtIODevIdx = ExtIOVal;
		break;
	}
}

extern "C" void __stdcall StopHW(void)
{
	RtlSdrThreadStop();
	delete[] RtlSdrBufShort;
	RtlSdrBufShort = NULL;
	EnableWindow(GetDlgItem(h_dialog, IDC_RTL_BUFFER), TRUE);
	EnableWindow(GetDlgItem(h_dialog, IDC_RTL_DEVICE), TRUE);
}

extern "C" void __stdcall CloseHW(void)
{
	rtlsdr_close(RtlSdrDev);
	RtlSdrDev = NULL;
	if (h_dialog)
		DestroyWindow(h_dialog);
}

extern "C" void __stdcall ShowGUI(void)
{
	ShowWindow(h_dialog, SW_SHOW);
	SetForegroundWindow(h_dialog);
}

extern "C" void __stdcall HideGUI(void)
{
	ShowWindow(h_dialog, SW_HIDE);
}

extern "C" void __stdcall SwitchGUI(void)
{
	if (IsWindowVisible(h_dialog))
		ShowWindow(h_dialog, SW_HIDE);
	else
		ShowWindow(h_dialog, SW_SHOW);
}

extern "C" void __stdcall SetCallback(void (*ParentCallback)(int, int, float, void *))
{
	ExtIOCallback = ParentCallback;
}

static void RtlSdrCallback(unsigned char *buf, uint32_t len, void * /* ctx */)
{
	if (RtlSdrBufShort && buf && len == RtlSdrBufSize) {
		short *short_buf = RtlSdrBufShort;
		unsigned char *char_buf = buf;

		for (uint32_t i = 0; i < len; i++)
			*short_buf++ = ((short)(*char_buf++)) - 128;

		ExtIOCallback(RtlSdrBufSize, 0, 0, (void *)RtlSdrBufShort);
	}
}

static int RtlSdrThreadStart(void)
{
	// Exit if already running
	if (RtlSdrHandle != INVALID_HANDLE_VALUE)
		return -1;

	// Reset endpoint
	if (rtlsdr_reset_buffer(RtlSdrDev) < 0)
		return -1;

	RtlSdrHandle = (HANDLE)_beginthread(RtlSdrThreadProc, 0, NULL);
	if (RtlSdrHandle == INVALID_HANDLE_VALUE)
		return -1;

	SetThreadPriority(RtlSdrHandle, THREAD_PRIORITY_TIME_CRITICAL);
	return 0;
}

static int RtlSdrThreadStop(void)
{
	if (RtlSdrHandle == INVALID_HANDLE_VALUE)
		return -1;
	rtlsdr_cancel_async(RtlSdrDev);
	WaitForSingleObject(RtlSdrHandle, INFINITE);
	CloseHandle(RtlSdrHandle);
	RtlSdrHandle = INVALID_HANDLE_VALUE;
	return 0;
}

static void RtlSdrThreadProc(void * /* param */)
{
	// Blocks until rtlsdr_cancel_async() is called
	// Use default number of buffers
	rtlsdr_read_async(RtlSdrDev, (rtlsdr_read_async_cb_t)&RtlSdrCallback,
			  NULL, 0, RtlSdrBufSize);
	_endthread();
}

static INT_PTR CALLBACK MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND hGain;
	static HBRUSH BRUSH_RED = CreateSolidBrush(RGB(255, 0, 0));
	static HBRUSH BRUSH_GREEN = CreateSolidBrush(RGB(0, 255, 0));

	switch (uMsg) {
	case WM_INITDIALOG: {
		int TunerGainIdx = 0;
		TCHAR ppm[256];

		for (int i = 0; i < (sizeof(RtlSdrDirSamplingArr) / sizeof(RtlSdrDirSamplingArr[0])); i++)
			ComboBox_AddString(GetDlgItem(hwndDlg, IDC_RTL_DIR_SAMPLING), RtlSdrDirSamplingArr[i]);

		ComboBox_SetCurSel(GetDlgItem(hwndDlg, IDC_RTL_DIR_SAMPLING), ExtIODirSampling);
		rtlsdr_set_direct_sampling(RtlSdrDev, ExtIODirSampling);

		Button_SetCheck(GetDlgItem(hwndDlg, IDC_TUNER_AGC),
				ExtIOTunerAGC ? BST_CHECKED : BST_UNCHECKED);
		rtlsdr_set_tuner_gain_mode(RtlSdrDev, ExtIOTunerAGC ? 0 : 1);

		Button_SetCheck(GetDlgItem(hwndDlg, IDC_RTL_AGC),
				ExtIORTLAGC ? BST_CHECKED : BST_UNCHECKED);
		rtlsdr_set_agc_mode(RtlSdrDev, ExtIORTLAGC ? 1 : 0);

		Button_SetCheck(GetDlgItem(hwndDlg, IDC_TUNER_OFF_TUNING),
				ExtIOOffsetTuning ? BST_CHECKED : BST_UNCHECKED);
		rtlsdr_set_offset_tuning(RtlSdrDev, ExtIOOffsetTuning ? 1 : 0);

		SendMessage(GetDlgItem(hwndDlg, IDC_RTL_PPM_CTL),
			    UDM_SETRANGE, (WPARAM)TRUE,
			    MAKELPARAM(RTLSDR_MAXPPM, RTLSDR_MINPPM));

		_stprintf_s(ppm, 256, TEXT("%d"), ExtIOFreqCorrection);
		Edit_SetText(GetDlgItem(hwndDlg, IDC_RTL_PPM), ppm);
		rtlsdr_set_freq_correction(RtlSdrDev, ExtIOFreqCorrection);

		for (uint32_t i = 0; i < RtlSdrDevCount; i++) {
			TCHAR devid[256];

			_stprintf_s(devid, 256, TEXT("%d: %S"), i,
				    rtlsdr_get_device_name(i));
			ComboBox_AddString(GetDlgItem(hwndDlg, IDC_RTL_DEVICE), devid);
		}
		ComboBox_SetCurSel(GetDlgItem(hwndDlg, IDC_RTL_DEVICE), ExtIODevIdx);

		for (int i = 0; i < (sizeof(RtlSdrSampleRateArr) / sizeof(RtlSdrSampleRateArr[0])); i++)
			ComboBox_AddString(GetDlgItem(hwndDlg, IDC_RTL_SAMPLE_RATE),
					   RtlSdrSampleRateArr[i].name);
		ComboBox_SetCurSel(GetDlgItem(hwndDlg, IDC_RTL_SAMPLE_RATE), ExtIOSampleRate);

		for (int i = 0; i < (sizeof(RtlSdrBufSizeArr) / sizeof(RtlSdrBufSizeArr[0])); i++) {
			TCHAR bufsize[256];

			_stprintf_s(bufsize, 256, TEXT("%d "), RtlSdrBufSizeArr[i]);
			ComboBox_AddString(GetDlgItem(hwndDlg, IDC_RTL_BUFFER), bufsize);
		}
		ComboBox_SetCurSel(GetDlgItem(hwndDlg, IDC_RTL_BUFFER), ExtIOBufferSize);
		RtlSdrBufSize = RtlSdrBufSizeArr[ExtIOBufferSize] * 1024;

		RtlSdrTunerGainsCount = rtlsdr_get_tuner_gains(RtlSdrDev, NULL);
		RtlSdrTunerGainsArr = new int[RtlSdrTunerGainsCount];
		hGain = GetDlgItem(hwndDlg, IDC_TUNER_GAIN_CTL);

		rtlsdr_get_tuner_gains(RtlSdrDev, RtlSdrTunerGainsArr);
		SendMessage(hGain, TBM_SETRANGEMIN, (WPARAM)TRUE, (LPARAM)-RtlSdrTunerGainsArr[RtlSdrTunerGainsCount - 1]);
		SendMessage(hGain, TBM_SETRANGEMAX, (WPARAM)TRUE, (LPARAM)-RtlSdrTunerGainsArr[0]);
		for (int i = 0; i < RtlSdrTunerGainsCount; i++) {
			SendMessage(hGain, TBM_SETTIC, (WPARAM)0, (LPARAM)-RtlSdrTunerGainsArr[i]);
			if (ExtIOTunerGain == RtlSdrTunerGainsArr[i])
				TunerGainIdx = i;
		}
		SendMessage(hGain, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)-RtlSdrTunerGainsArr[TunerGainIdx]);
		RtlSdrTunerGain = RtlSdrTunerGainsArr[TunerGainIdx];

		if (ExtIOTunerAGC) {
			EnableWindow(hGain, FALSE);
			Static_SetText(GetDlgItem(hwndDlg, IDC_TUNER_GAIN), TEXT("AGC"));
		} else {
			int pos = (int)-SendMessage(hGain, TBM_GETPOS, (WPARAM)0, (LPARAM)0);
			TCHAR tunergain[256];

			_stprintf_s(tunergain, 256, TEXT("%2.1f dB"), (float)(pos / 10.0));
			Static_SetText(GetDlgItem(hwndDlg, IDC_TUNER_GAIN), tunergain);
			rtlsdr_set_tuner_gain(RtlSdrDev, pos);
		}
		return TRUE;
	}
	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDC_RTL_PPM:
			if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE) {
				TCHAR ppm[256];

				Edit_GetText((HWND)lParam, ppm, 256);
				if (!rtlsdr_set_freq_correction(RtlSdrDev, _ttoi(ppm)))
					ExtIOCallback(-1, EXTIO_CHANGED_LO, 0, NULL);
			}
			return TRUE;
		case IDC_RTL_AGC:
			if (Button_GetCheck
				(GET_WM_COMMAND_HWND(wParam, lParam)) == BST_CHECKED)
				rtlsdr_set_agc_mode(RtlSdrDev, 1);
			else
				rtlsdr_set_agc_mode(RtlSdrDev, 0);
			return TRUE;
		case IDC_TUNER_OFF_TUNING:
			if (Button_GetCheck
				(GET_WM_COMMAND_HWND(wParam, lParam)) == BST_CHECKED)
				rtlsdr_set_offset_tuning(RtlSdrDev, 1);
			else
				rtlsdr_set_offset_tuning(RtlSdrDev, 0);
			return TRUE;
		case IDC_TUNER_AGC:
			if (Button_GetCheck
				(GET_WM_COMMAND_HWND(wParam, lParam)) == BST_CHECKED) {
				rtlsdr_set_tuner_gain_mode(RtlSdrDev, 0);
				EnableWindow(hGain, FALSE);
				Static_SetText(GetDlgItem
					      (hwndDlg, IDC_TUNER_GAIN), TEXT("AGC"));
			} else {
				int pos = (int)-SendMessage(hGain, TBM_GETPOS,
							   (WPARAM)0, (LPARAM)0);
				TCHAR tunergain[256];

				rtlsdr_set_tuner_gain_mode(RtlSdrDev, 1);
				EnableWindow(hGain, TRUE);
				_stprintf_s(tunergain, 256, TEXT("%2.1f dB"),
					   (float)(pos / 10.0));
				Static_SetText(GetDlgItem
					      (hwndDlg, IDC_TUNER_GAIN), tunergain);
				rtlsdr_set_tuner_gain(RtlSdrDev, pos);
			}
			return TRUE;
		case IDC_RTL_SAMPLE_RATE:
			if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE) {
				rtlsdr_set_sample_rate(RtlSdrDev,
					(uint32_t)RtlSdrSampleRateArr[ComboBox_GetCurSel(GET_WM_COMMAND_HWND(wParam, lParam))].value);
				ExtIOCallback(-1, EXTIO_CHANGED_SR, 0, NULL);
			}
			if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_EDITUPDATE) {
				double coeff;
				uint32_t newsrate;
				int exp = 1;
				TCHAR srate[256];
				TCHAR *endptr;

				ComboBox_GetText((HWND)lParam, srate, 256);
				coeff = _tcstod(srate, &endptr);
				while (_istspace(*endptr))
					++endptr;

				switch (_totupper(*endptr)) {
				case 'K':
					exp = 1024;
					break;
				case 'M':
					exp = 1024 * 1024;
					break;
				}

				newsrate = (uint32_t)(coeff * exp);
				if (newsrate >= RTLSDR_MINHISRATE && newsrate <= RTLSDR_MAXHISRATE) {
					rtlsdr_set_sample_rate(RtlSdrDev, newsrate);
					ExtIOCallback(-1, EXTIO_CHANGED_SR, 0, NULL);
				}
			}
			return TRUE;
		case IDC_RTL_BUFFER:
			if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE) {
				RtlSdrBufSize = RtlSdrBufSizeArr[ComboBox_GetCurSel
						(GET_WM_COMMAND_HWND(wParam, lParam))] * 1024;
				ExtIOCallback(-1, EXTIO_CHANGED_SR, 0, NULL);
			}
			return TRUE;
		case IDC_RTL_DIR_SAMPLING:
			if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE) {
				rtlsdr_set_direct_sampling(RtlSdrDev,
				    ComboBox_GetCurSel(GET_WM_COMMAND_HWND(wParam, lParam)));
				if (ComboBox_GetCurSel(GET_WM_COMMAND_HWND(wParam, lParam)) == 0) {
					if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_TUNER_OFF_TUNING)) == BST_CHECKED)
						rtlsdr_set_offset_tuning(RtlSdrDev, 1);
					else
						rtlsdr_set_offset_tuning(RtlSdrDev, 0);

					if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_TUNER_AGC)) == BST_CHECKED) {
						rtlsdr_set_tuner_gain_mode(RtlSdrDev, 0);
					} else {
						int pos = (int)-SendMessage(hGain, TBM_GETPOS, (WPARAM)0, (LPARAM)0);

						rtlsdr_set_tuner_gain_mode(RtlSdrDev, 1);
						rtlsdr_set_tuner_gain(RtlSdrDev, pos);
					}
				}
				ExtIOCallback(-1, EXTIO_CHANGED_LO, 0, NULL);
			}
			return TRUE;
		case IDC_RTL_DEVICE:
			if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE) {
				uint32_t currsrate = rtlsdr_get_sample_rate(RtlSdrDev);

				rtlsdr_close(RtlSdrDev);
				RtlSdrDev = NULL;
				if (rtlsdr_open(&RtlSdrDev, (uint32_t)ComboBox_GetCurSel(GET_WM_COMMAND_HWND(wParam, lParam))) < 0) {
					MessageBox(NULL, TEXT("Couldn't open device!"),
						   TEXT(EXTIO_RTL_NAME), MB_OK | MB_ICONERROR);
					return TRUE;
				}
				rtlsdr_set_sample_rate(RtlSdrDev, currsrate);
			}
			return TRUE;
		}
		break;
	case WM_VSCROLL:
		if ((HWND)lParam == hGain) {
			int pos = (int)-SendMessage(hGain, TBM_GETPOS,
						   (WPARAM)0, (LPARAM)0);
			TCHAR tunergain[256];

			for (int i = 0; i < RtlSdrTunerGainsCount - 1; i++) {
				if (pos > RtlSdrTunerGainsArr[i] &&
				    pos < RtlSdrTunerGainsArr[i + 1]) {
					if (((pos - RtlSdrTunerGainsArr[i]) <
					     (RtlSdrTunerGainsArr[i + 1] - pos) &&
					     (LOWORD(wParam) != TB_LINEUP)) ||
					    (LOWORD(wParam) == TB_LINEDOWN))
						pos = RtlSdrTunerGainsArr[i];
					else
						pos = RtlSdrTunerGainsArr[i + 1];
				}
			}

			SendMessage(hGain, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)-pos);
			_stprintf_s(tunergain, 256, TEXT("%2.1f dB"), (float)(pos / 10.0));
			Static_SetText(GetDlgItem(hwndDlg, IDC_TUNER_GAIN), tunergain);

			if (pos != RtlSdrTunerGain) {
				RtlSdrTunerGain = pos;
				rtlsdr_set_tuner_gain(RtlSdrDev, pos);
				ExtIOCallback(-1, EXTIO_CHANGED_ATT, 0, NULL);
			}
			return TRUE;
		}
		if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_RTL_PPM_CTL))
			return TRUE;
		break;
	case WM_CLOSE:
		ShowWindow(h_dialog, SW_HIDE);
		return TRUE;
	case WM_DESTROY:
		delete[] RtlSdrTunerGainsArr;
		RtlSdrTunerGainsArr = NULL;
		h_dialog = NULL;
		return TRUE;
	case WM_CTLCOLORSTATIC:
		if (IDC_TUNER_PLL == GetDlgCtrlID((HWND)lParam)) {
			HDC hdc = (HDC)wParam;

			if (!RtlSdrPllLocked) {
				SetBkColor(hdc, RGB(0, 255, 0));
				return (INT_PTR)BRUSH_GREEN;
			} else {
				SetBkColor(hdc, RGB(255, 0, 0));
				return (INT_PTR)BRUSH_RED;
			}
		}
		break;
	}
	return FALSE;
}
