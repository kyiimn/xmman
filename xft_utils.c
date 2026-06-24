/*
 * xft_utils.c — Xft font utility subsystem for xmman Motif/Xft conversion
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

#include "xft_utils.h"

/*
 * XftFontOpenWithFallback — Try font patterns in priority order.
 *
 * Attempts: primary → fallback1 → fallback2 → fallback3 → fallback4
 * If all named patterns fail, falls back to "fixed" as absolute last resort.
 * Returns NULL only if even "fixed" cannot be loaded.
 */
XftFont *
XftFontOpenWithFallback(Display *dpy, int screen, const char *primary,
                         const char *fallback1, const char *fallback2,
                         const char *fallback3, const char *fallback4)
{
    static const char *absolute_fallback = "fixed";
    const char *patterns[] = { primary, fallback1, fallback2,
                               fallback3, fallback4 };
    XftFont *font = NULL;

    for (int i = 0; i < (int)(sizeof(patterns) / sizeof(patterns[0])); i++) {
        if (patterns[i] == NULL || patterns[i][0] == '\0')
            continue;

        font = XftFontOpenName(dpy, screen, patterns[i]);
        if (font != NULL)
            return font;

        fprintf(stderr, "xmman: font pattern \"%s\" not found", patterns[i]);
        if (i < (int)(sizeof(patterns) / sizeof(patterns[0])) - 1)
            fprintf(stderr, ", trying fallback\n");
        else
            fprintf(stderr, "\n");
    }

    font = XftFontOpenName(dpy, screen, absolute_fallback);
    if (font != NULL) {
        fprintf(stderr, "xmman: using \"%s\" as absolute fallback\n",
                absolute_fallback);
        return font;
    }

    fprintf(stderr, "xmman: FATAL: could not load any font, not even \"%s\"\n",
            absolute_fallback);
    return NULL;
}

/*
 * XftLoadFontSet — Load all 4 font variants for man page rendering.
 *
 * Uses XftFontOpenWithFallback for each font slot with the project's
 * CJK fallback chain:
 *   Monospace: primary → Noto Sans Mono CJK KR → DejaVu Sans Mono → monospace → fixed
 *
 * If even the absolute fallback fails for the normal font, returns NULL.
 * Bold, italic, and symbol fonts that fail all fallbacks fall back to
 * the normal font so rendering never dereferences a NULL pointer.
 */
XmManFontSet *
XftLoadFontSet(Display *dpy, const char *normal_pattern,
               const char *bold_pattern, const char *italic_pattern,
               const char *symbol_pattern)
{
    int screen = DefaultScreen(dpy);
    XmManFontSet *fonts;

    fonts = (XmManFontSet *)malloc(sizeof(XmManFontSet));
    if (fonts == NULL) {
        fprintf(stderr, "xmman: out of memory allocating XmManFontSet\n");
        return NULL;
    }
    memset(fonts, 0, sizeof(XmManFontSet));

    fonts->normal = XftFontOpenWithFallback(dpy, screen, normal_pattern,
                                             "Noto Sans Mono CJK KR",
                                             "DejaVu Sans Mono",
                                             "monospace", "fixed");
    if (fonts->normal == NULL) {
        fprintf(stderr, "xmman: could not load normal font or any fallback\n");
        free(fonts);
        return NULL;
    }

    fonts->bold = XftFontOpenWithFallback(dpy, screen, bold_pattern,
                                           "Noto Sans Mono CJK KR:style=Bold",
                                           "DejaVu Sans Mono:style=Bold",
                                           "monospace:bold", "fixed");
    if (fonts->bold == NULL) {
        fprintf(stderr, "xmman: bold font fallback to normal\n");
        fonts->bold = fonts->normal;
    }

    fonts->italic = XftFontOpenWithFallback(dpy, screen, italic_pattern,
                                            "Noto Sans Mono CJK KR:style=Italic",
                                            "DejaVu Sans Mono:style=Oblique",
                                            "monospace:italic", "fixed");
    if (fonts->italic == NULL) {
        fprintf(stderr, "xmman: italic font fallback to normal\n");
        fonts->italic = fonts->normal;
    }

    fonts->symbol = XftFontOpenWithFallback(dpy, screen, symbol_pattern,
                                            "Noto Sans Mono CJK KR",
                                            "DejaVu Sans Mono",
                                            "monospace", "fixed");
    if (fonts->symbol == NULL) {
        fprintf(stderr, "xmman: symbol font fallback to normal\n");
        fonts->symbol = fonts->normal;
    }

    return fonts;
}

/*
 * XftFreeFontSet — Free all fonts in the set.
 *
 * Only calls XftFontClose for fonts that are distinct (non-NULL and
 * not aliased to the normal font) to avoid double-free.
 */
void
XftFreeFontSet(Display *dpy, XmManFontSet *fonts)
{
    if (fonts == NULL)
        return;

    if (fonts->normal != NULL)
        XftFontClose(dpy, fonts->normal);

    if (fonts->bold != NULL && fonts->bold != fonts->normal)
        XftFontClose(dpy, fonts->bold);

    if (fonts->italic != NULL && fonts->italic != fonts->normal)
        XftFontClose(dpy, fonts->italic);

    if (fonts->symbol != NULL && fonts->symbol != fonts->normal)
        XftFontClose(dpy, fonts->symbol);

    free(fonts);
}

/*
 * XftCreateContext — Create an Xft rendering context.
 *
 * Uses the caller-provided visual and colormap (from the widget),
 * NOT DefaultVisual/DefaultColormap. This is critical for widgets
 * that may use a non-default visual.
 */
XftRenderingContext *
XftCreateContext(Display *dpy, Drawable drawable, Visual *visual,
                 Colormap colormap, unsigned long fg_pixel,
                 unsigned long bg_pixel)
{
    XftRenderingContext *ctx;
    XRenderColor render_color;

    ctx = (XftRenderingContext *)malloc(sizeof(XftRenderingContext));
    if (ctx == NULL) {
        fprintf(stderr, "xmman: out of memory allocating XftRenderingContext\n");
        return NULL;
    }
    memset(ctx, 0, sizeof(XftRenderingContext));

    ctx->draw = XftDrawCreate(dpy, drawable, visual, colormap);
    if (ctx->draw == NULL) {
        fprintf(stderr, "xmman: XftDrawCreate failed\n");
        free(ctx);
        return NULL;
    }

    ctx->visual = visual;
    ctx->colormap = colormap;

    render_color.red = 0;
    render_color.green = 0;
    render_color.blue = 0;
    render_color.alpha = 0xffff;

    if (fg_pixel != 0) {
        XColor xcolor;
        xcolor.pixel = fg_pixel;
        XQueryColor(dpy, colormap, &xcolor);
        render_color.red = xcolor.red;
        render_color.green = xcolor.green;
        render_color.blue = xcolor.blue;
    }

    if (!XftColorAllocValue(dpy, visual, colormap, &render_color, &ctx->fg_color)) {
        fprintf(stderr, "xmman: XftColorAllocValue failed for fg_color\n");
        XftDrawDestroy(ctx->draw);
        free(ctx);
        return NULL;
    }

    render_color.red = 0xffff;
    render_color.green = 0xffff;
    render_color.blue = 0xffff;
    render_color.alpha = 0xffff;

    if (bg_pixel != 0) {
        XColor xcolor;
        xcolor.pixel = bg_pixel;
        XQueryColor(dpy, colormap, &xcolor);
        render_color.red = xcolor.red;
        render_color.green = xcolor.green;
        render_color.blue = xcolor.blue;
    }

    if (!XftColorAllocValue(dpy, visual, colormap, &render_color, &ctx->bg_color)) {
        fprintf(stderr, "xmman: XftColorAllocValue failed for bg_color\n");
        XftColorFree(dpy, visual, colormap, &ctx->fg_color);
        XftDrawDestroy(ctx->draw);
        free(ctx);
        return NULL;
    }

    return ctx;
}

/*
 * XftDestroyContext — Free all Xft resources in the rendering context.
 */
void
XftDestroyContext(Display *dpy, XftRenderingContext *ctx)
{
    if (ctx == NULL)
        return;

    if (ctx->draw != NULL)
        XftDrawDestroy(ctx->draw);

    XftColorFree(dpy, ctx->visual, ctx->colormap, &ctx->fg_color);
    XftColorFree(dpy, ctx->visual, ctx->colormap, &ctx->bg_color);

    free(ctx);
}

/*
 * XftChangeDrawable — Update the drawable target for rendering.
 */
void
XftChangeDrawable(XftRenderingContext *ctx, Drawable new_drawable)
{
    if (ctx == NULL || ctx->draw == NULL)
        return;

    XftDrawChange(ctx->draw, new_drawable);
}

int
XftGetFontHeight(const XftFont *font)
{
    if (font == NULL)
        return 0;
    return font->height;
}

int
XftGetFontWidth(const XftFont *font)
{
    if (font == NULL)
        return 0;
    return font->max_advance_width;
}

int
XftGetFontAscent(const XftFont *font)
{
    if (font == NULL)
        return 0;
    return font->ascent;
}

int
XftGetFontDescent(const XftFont *font)
{
    if (font == NULL)
        return 0;
    return font->descent;
}

/*
 * XftDrawStringUtf8Wrapped — Render a UTF-8 string via Xft.
 *
 * Currently a straight wrapper around XftDrawStringUtf8.
 * The "Wrapped" name reserves the API for future word-wrap extension.
 */
void
XftDrawStringUtf8Wrapped(XftRenderingContext *ctx, XftFont *font,
                          int x, int y, const char *text, int len)
{
    if (ctx == NULL || font == NULL || text == NULL || len <= 0)
        return;

    XftDrawStringUtf8(ctx->draw, &ctx->fg_color, font,
                      x, y, (const FcChar8 *)text, len);
}