/*
 * xmman_fonts.c — Font loading and fallback system for xmman Motif/Xft conversion
 *
 * Copyright (c) 2026 xmman Motif/Xft conversion project
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <fontconfig/fontconfig.h>

#include "xmman_fonts.h"
#include "man.h"

/*
 * XftFontBuildPattern — Build a fontconfig pattern string from components.
 *
 * Returns a malloc'd string like "D2Coding-12" or "D2Coding-12:weight=bold"
 * or "D2Coding-12:slant=italic". Caller must free() the result.
 */
char *
XftFontBuildPattern(const char *family, int size, int weight, int slant)
{
    char buf[256];
    int len;

    len = snprintf(buf, sizeof(buf), "%s-%d", family, size);

    if (weight == FC_WEIGHT_BOLD) {
        len += snprintf(buf + len, sizeof(buf) - len, ":weight=bold");
    }

    if (slant == FC_SLANT_ITALIC) {
        len += snprintf(buf + len, sizeof(buf) - len, ":slant=italic");
    }

    return strdup(buf);
}

/*
 * XmManLoadManpageFonts — Load all 4 font variants for man page rendering.
 *
 * Uses the D2Coding monospace fallback chain via XftLoadFontSet.
 * Returns NULL if even the absolute fallback fails.
 */
XmManFontSet *
XmManLoadManpageFonts(Display *dpy, int screen)
{
    char *normal_pat, *bold_pat, *italic_pat, *symbol_pat;
    XmManFontSet *fonts;

    (void)screen;

    normal_pat = XftFontBuildPattern(XMMAN_MANPAGE_FONT,
                                     XMMAN_MANPAGE_FONT_SIZE,
                                     FC_WEIGHT_MEDIUM, FC_SLANT_ROMAN);
    bold_pat = XftFontBuildPattern(XMMAN_MANPAGE_FONT,
                                   XMMAN_MANPAGE_FONT_SIZE,
                                   FC_WEIGHT_BOLD, FC_SLANT_ROMAN);
    italic_pat = XftFontBuildPattern(XMMAN_MANPAGE_FONT,
                                     XMMAN_MANPAGE_FONT_SIZE,
                                     FC_WEIGHT_MEDIUM, FC_SLANT_ITALIC);
    symbol_pat = XftFontBuildPattern(XMMAN_MANPAGE_FONT,
                                     XMMAN_MANPAGE_FONT_SIZE,
                                     FC_WEIGHT_MEDIUM, FC_SLANT_ROMAN);

    fonts = XftLoadFontSet(dpy, normal_pat, bold_pat, italic_pat, symbol_pat);

    free(normal_pat);
    free(bold_pat);
    free(italic_pat);
    free(symbol_pat);

    return fonts;
}

/*
 * XmManLoadDirectoryFont — Load the proportional UI font for directory entries.
 *
 * Uses the NanumMyeongjo proportional fallback chain.
 * Returns NULL only if even "fixed" cannot be loaded.
 */
XftFont *
XmManLoadDirectoryFont(Display *dpy, int screen)
{
    char *pattern;
    XftFont *font;

    pattern = XftFontBuildPattern(XMMAN_UI_FONT, XMMAN_DIRECTORY_FONT_SIZE,
                                  FC_WEIGHT_MEDIUM, FC_SLANT_ROMAN);

    font = XftFontOpenWithFallback(dpy, screen, pattern,
                                   XMMAN_UI_FALLBACK_1, XMMAN_UI_FALLBACK_2,
                                   XMMAN_UI_FALLBACK_3, XMMAN_UI_FALLBACK_4);

    free(pattern);
    return font;
}

/*
 * XmManFreeFonts — Free all loaded fonts in the XmManFonts struct.
 *
 * Does NOT free the fonts struct itself (it's part of XmMan_Resources).
 */
void
XmManFreeFonts(Display *dpy, XmManFonts *fonts)
{
    if (fonts == NULL)
        return;

    if (fonts->manpage_fonts != NULL) {
        XftFreeFontSet(dpy, fonts->manpage_fonts);
        fonts->manpage_fonts = NULL;
    }

    if (fonts->directory_font != NULL) {
        XftFontClose(dpy, fonts->directory_font);
        fonts->directory_font = NULL;
    }
}