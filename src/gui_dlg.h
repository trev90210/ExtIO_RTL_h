#pragma once

#include <Windows.h>
#include <atomic>

#define MAX_PPM   1000
#define MIN_PPM   -1000


extern char band_disp_text[255];
extern std::atomic_bool update_band_text;

bool is_gui_available();
void InitGUIControls();
void CreateGUI();
void DestroyGUI();
void gui_show();

void gui_show_missing_device(int from);  // 0 == OpenHW(), 1 == StartHW()
void gui_show_invalid_device();

void post_update_gui_init();
void post_update_gui_fields();

void gui_SetSrate(int srate_idx);
void gui_SetAttenuator(int atten_idx);
void gui_SetMGC(int mgc_idx);
void DisableGUIControlsAtStart();
void EnableGUIControlsAtStop();

INT_PTR CALLBACK MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
