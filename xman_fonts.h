/*
 * xman_fonts.h — Font configuration and fallback system for xman Motif/Xft conversion
 *
 * Defines font pattern constants with D2Coding monospace chain (man page body)
 * and NanumMyeongjo proportional chain (UI chrome), plus function prototypes
 * for font loading and resource management.
 *
 * Copyright (c) 2026 xman Motif/Xft conversion project
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.
 */

#ifndef XMAN_FONTS_H
#define XMAN_FONTS_H

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include "xft_utils.h"  /* for XmanFontSet */

struct _XmanFonts;

/*
 * Font configuration constants for xman Motif/Xft conversion
 *
 * Man page body (monospace): D2Coding family with fallback chain
 * UI chrome (proportional):  NanumMyeongjo family with fallback chain
 */

/* Man page body fonts (monospace) — D2Coding primary */
#define XMAN_MANPAGE_FONT        "D2Coding"
#define XMAN_MANPAGE_FALLBACK_1   "Noto Sans Mono CJK KR"
#define XMAN_MANPAGE_FALLBACK_2   "DejaVu Sans Mono"
#define XMAN_MANPAGE_FALLBACK_3   "monospace"
#define XMAN_MANPAGE_FALLBACK_4   "fixed"

/* UI chrome font (proportional) — NanumMyeongjo primary */
#define XMAN_UI_FONT              "NanumMyeongjo"
#define XMAN_UI_FALLBACK_1        "Noto Serif CJK KR"
#define XMAN_UI_FALLBACK_2        "DejaVu Serif"
#define XMAN_UI_FALLBACK_3        "serif"
#define XMAN_UI_FALLBACK_4        "fixed"

/* Default font sizes */
#define XMAN_MANPAGE_FONT_SIZE    12
#define XMAN_DIRECTORY_FONT_SIZE  14

/* Font pattern builder — constructs FcPattern-compatible fontconfig pattern string */
char *XftFontBuildPattern(const char *family, int size, int weight, int slant);

/* Man page font loading — uses D2Coding monospace fallback chain */
XmanFontSet *XmanLoadManpageFonts(Display *dpy, int screen);

/* Directory/UI font loading — uses NanumMyeongjo proportional fallback chain */
XftFont *XmanLoadDirectoryFont(Display *dpy, int screen);

/* Free all loaded fonts */
void XmanFreeFonts(Display *dpy, struct _XmanFonts *fonts);

#endif /* XMAN_FONTS_H */