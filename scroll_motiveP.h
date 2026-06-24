/*
 * scroll_motiveP.h — ScrollMotive widget private header
 *
 * Instance and class record definitions for the ScrollMotive widget,
 * an XmDrawingArea subclass that renders man pages via Xft.
 *
 * Copyright (c) 2026 xman Motif/Xft conversion project
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.
 */

#ifndef _XtScrollMotivePrivate_h
#define _XtScrollMotivePrivate_h

#include <Xm/XmP.h>
#include <Xm/DrawingAP.h>
#include <Xm/ScrollBar.h>
#include <X11/Xft/Xft.h>
#include "scroll_motive.h"
#include "xft_utils.h"
#include "xman_fonts.h"

/***********************************************************************
 *
 * ScrollMotive Widget Class Record
 *
 ***********************************************************************/

typedef struct {
    int mumble;
} ScrollMotiveClassPart;

typedef struct _ScrollMotiveClassRec {
    CoreClassPart            core_class;
    XmDrawingAreaClassPart   drawing_area_class;
    ScrollMotiveClassPart    scroll_motive_class;
} ScrollMotiveClassRec;

extern ScrollMotiveClassRec scrollMotiveClassRec;

/***********************************************************************
 *
 * ScrollMotive Widget Instance Record
 *
 ***********************************************************************/

typedef struct _ScrollMotivePart {
    XmanFontSet    *fonts;
    XftRenderingContext render_ctx;
    int             h_width;
    int             font_height;
    int             line_pointer;
    int             lines;
    char          **top_line;
    Dimension       indent;
    Boolean         force_vert;
    FILE           *file;
    Widget          scrollbar;
    Pixmap          back_pixmap;
    XftColor        fg_color;
    XftColor        bg_color;
    Visual         *visual;
    Colormap        colormap;
    Dimension       draw_width;
    Dimension       draw_height;
    int             num_visible_lines;
} ScrollMotivePart;

typedef struct _ScrollMotiveRec {
    CorePart              core;
    XmDrawingAreaPart     drawing_area;
    ScrollMotivePart      scroll;
} ScrollMotiveRec;

#endif /* _XtScrollMotivePrivate_h */