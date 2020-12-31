/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef EXTIO_RTL_H
#define EXTIO_RTL_H

/* RTLSDR */
#define MIN_PPM			(-1000)
#define MAX_PPM			1000
#define MINRATE			900001
#define MAXRATE			3200000

/* ExtIO */
#define EXTIO_HWTYPE_16B	3
#define WINRAD_SRCHANGE		100
#define WINRAD_LOCHANGE		101
#define WINRAD_ATTCHANGE	125

extern HMODULE hInst;

#endif /* EXTIO_RTL_H */
