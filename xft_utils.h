/*
 * xft_utils.h — Xft font utility subsystem for xman Motif/Xft conversion
 *
 * Provides font loading with CJK fallback chains, rendering context management,
 * and convenience metric functions for the ScrollMotive man page renderer.
 *
 * Copyright (c) 2026 xman Motif/Xft conversion project
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.
 */

#ifndef XFT_UTILS_H
#define XFT_UTILS_H

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

/*
 * XmanFontSet — Holds 4 font variants for man page rendering
 * (normal, bold, italic, symbol) — all monospace for column alignment
 */
typedef struct {
    XftFont *normal;
    XftFont *bold;
    XftFont *italic;
    XftFont *symbol;
} XmanFontSet;

/*
 * XftRenderingContext — Drawing context for Xft text rendering
 */
typedef struct {
    XftDraw *draw;
    XftColor fg_color;
    XftColor bg_color;
    Visual *visual;
    Colormap colormap;
} XftRenderingContext;

/* Font loading with fallback chains */
XmanFontSet *XftLoadFontSet(Display *dpy, const char *normal_pattern,
                            const char *bold_pattern, const char *italic_pattern,
                            const char *symbol_pattern);
void XftFreeFontSet(Display *dpy, XmanFontSet *fonts);

/* Individual font loading with fallback */
XftFont *XftFontOpenWithFallback(Display *dpy, int screen, const char *primary,
                                   const char *fallback1, const char *fallback2,
                                   const char *fallback3, const char *fallback4);

/* Context management — ALWAYS use widget's actual visual/colormap, NOT DefaultVisual */
XftRenderingContext *XftCreateContext(Display *dpy, Drawable drawable, Visual *visual,
                                       Colormap colormap, unsigned long fg_pixel,
                                       unsigned long bg_pixel);
void XftDestroyContext(Display *dpy, XftRenderingContext *ctx);
void XftChangeDrawable(XftRenderingContext *ctx, Drawable new_drawable);

/* Font metrics */
int XftGetFontHeight(const XftFont *font);
int XftGetFontWidth(const XftFont *font);
int XftGetFontAscent(const XftFont *font);
int XftGetFontDescent(const XftFont *font);

/* String rendering */
void XftDrawStringUtf8Wrapped(XftRenderingContext *ctx, XftFont *font,
                                int x, int y, const char *text, int len);

#endif /* XFT_UTILS_H */