/*
 * scroll_motive.h — ScrollMotive widget public interface
 *
 * XmDrawingArea-based man page renderer using Xft for anti-aliased
 * text. Replaces the old ScrollByLine Xaw widget.
 *
 * Copyright (c) 2026 xman Motif/Xft conversion project
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.
 */

#ifndef _XtScrollMotive_h
#define _XtScrollMotive_h

#include <X11/Intrinsic.h>
#include <X11/Xfuncproto.h>
#include <stdio.h>

/***********************************************************************
 *
 * ScrollMotive Widget (subclass of XmDrawingArea)
 *
 ***********************************************************************/

/* Resource names */
#define XtNmanFontSet       "manFontSet"         /* XmanFontSet * */
#define XtNmanPageFile      "manPageFile"         /* FILE * */
#define XtNmanIndent        "manIndent"           /* Dimension */
#define XtNmanForceVert     "manForceVert"        /* Boolean */

/* Class record pointer */
extern WidgetClass scrollMotiveWidgetClass;

/* Type declarations */
typedef struct _ScrollMotiveClassRec *ScrollMotiveWidgetClass;
typedef struct _ScrollMotiveRec      *ScrollMotiveWidget;

/* Public API */
extern void ScrollMotiveSetFile(Widget w, FILE *file);
extern void ScrollMotiveScrollToLine(Widget w, int line);
extern int  ScrollMotiveGetTotalLines(Widget w);
extern int  ScrollMotiveGetVisibleLines(Widget w);

#endif /* _XtScrollMotive_h */