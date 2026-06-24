/*
 * xmman_fonts.h — Font configuration and fallback system for xmman Motif/Xft conversion
 *
 * Defines font pattern constants with D2Coding monospace chain (man page body)
 * and NanumMyeongjo proportional chain (UI chrome), plus function prototypes
 * for font loading and resource management.
 *
 * Copyright (c) 2026 xmman Motif/Xft conversion project
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.
 */

#ifndef XMMAN_FONTS_H
#define XMMAN_FONTS_H

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include "xft_utils.h"  /* for XmManFontSet */

struct _XmManFonts;

/*
 * Font configuration constants for xmman Motif/Xft conversion
 *
 * Man page body (monospace): D2Coding family with fallback chain
 * UI chrome (proportional):  NanumMyeongjo family with fallback chain
 */

/* Man page body fonts (monospace) — D2Coding primary */
#define XMMAN_MANPAGE_FONT        "D2Coding"
#define XMMAN_MANPAGE_FALLBACK_1   "Noto Sans Mono CJK KR"
#define XMMAN_MANPAGE_FALLBACK_2   "DejaVu Sans Mono"
#define XMMAN_MANPAGE_FALLBACK_3   "monospace"
#define XMMAN_MANPAGE_FALLBACK_4   "fixed"

/* UI chrome font (proportional) — NanumMyeongjo primary */
#define XMMAN_UI_FONT              "NanumMyeongjo"
#define XMMAN_UI_FALLBACK_1        "Noto Serif CJK KR"
#define XMMAN_UI_FALLBACK_2        "DejaVu Serif"
#define XMMAN_UI_FALLBACK_3        "serif"
#define XMMAN_UI_FALLBACK_4        "fixed"

/* Default font sizes */
#define XMMAN_MANPAGE_FONT_SIZE    12
#define XMMAN_DIRECTORY_FONT_SIZE  14

/* Font pattern builder — constructs FcPattern-compatible fontconfig pattern string */
char *XftFontBuildPattern(const char *family, int size, int weight, int slant);

/* Man page font loading — uses D2Coding monospace fallback chain */
XmManFontSet *XmManLoadManpageFonts(Display *dpy, int screen);

/* Directory/UI font loading — uses NanumMyeongjo proportional fallback chain */
XftFont *XmManLoadDirectoryFont(Display *dpy, int screen);

/* Free all loaded fonts */
void XmManFreeFonts(Display *dpy, struct _XmManFonts *fonts);

#endif /* XMMAN_FONTS_H */