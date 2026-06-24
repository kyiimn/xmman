/*
 * scroll_motive.h — ScrollMotive widget public interface (stub)
 *
 * This is a minimal stub header that allows compilation of other modules
 * before the full ScrollMotive widget implementation is complete.
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

#define XtNscrollMotiveFontSet    "scrollMotiveFontSet"
#define XtNscrollMotiveDirFont    "scrollMotiveDirFont"

extern WidgetClass scrollMotiveWidgetClass;

typedef struct _ScrollMotiveClassRec *ScrollMotiveWidgetClass;
typedef struct _ScrollMotiveRec      *ScrollMotiveWidget;

#endif /* _XtScrollMotive_h */