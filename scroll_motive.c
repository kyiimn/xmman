/*
 * scroll_motive.c — ScrollMotive widget implementation
 *
 * XmDrawingArea-based man page renderer using Xft for anti-aliased
 * text rendering. Replaces the original ScrollByLine Xaw widget.
 *
 * Copyright (c) 2026 xman Motif/Xft conversion project
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

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xos.h>

#include <Xm/XmP.h>
#include <Xm/ScrollBar.h>
#include <Xm/DrawingAP.h>

#include <X11/Xft/Xft.h>

#include "scroll_motiveP.h"
#include "xft_utils.h"
#include "xman_fonts.h"

#define XtCIndent "Indent"

/****************************************************************
 *
 * Resource offsets
 *
 ****************************************************************/

#define Offset(field) XtOffsetOf(ScrollMotiveRec, scroll.field)

static XtResource resources[] = {
    {XtNmanForceVert, XtCBoolean, XtRBoolean, sizeof(Boolean),
     Offset(force_vert), XtRImmediate, (XtPointer) True},
    {XtNmanIndent, XtCIndent, XtRDimension, sizeof(Dimension),
     Offset(indent), XtRImmediate, (XtPointer) 15},
};

#undef Offset

/****************************************************************
 *
 * Forward declarations
 *
 ****************************************************************/

static void _ScrollMotiveInitialize(Widget, Widget, ArgList, Cardinal *);
static void _ScrollMotiveRealize(Widget, XtValueMask *, XSetWindowAttributes *);
static void _ScrollMotiveExpose(Widget, XEvent *, Region);
static void _ScrollMotiveResize(Widget);
static void _ScrollMotiveDestroy(Widget);
static Boolean _ScrollMotiveSetValues(Widget, Widget, Widget, ArgList, Cardinal *);

static void _ScrollMotiveDrawLines(Widget w, int from_line, int to_line);
static void _ScrollMotiveCreateScrollbar(Widget w);
static void VerticalScroll(Widget w, XtPointer client_data, XtPointer call_data);
static void VerticalJump(Widget w, XtPointer client_data, XtPointer call_data);

/****************************************************************
 *
 * Class record
 *
 ****************************************************************/

ScrollMotiveClassRec scrollMotiveClassRec = {
    /* core_class */
    {
        (WidgetClass) &xmDrawingAreaClassRec,    /* superclass */
        "ScrollMotive",                            /* class_name */
        sizeof(ScrollMotiveRec),                  /* widget_size */
        NULL,                                     /* class_initialize */
        NULL,                                     /* class_part_init */
        FALSE,                                    /* class_inited */
        (XtInitProc) _ScrollMotiveInitialize,    /* initialize */
        NULL,                                     /* initialize_hook */
        _ScrollMotiveRealize,                     /* realize */
        NULL,                                     /* actions */
        0,                                        /* num_actions */
        resources,                                /* resources */
        XtNumber(resources),                      /* num_resources */
        NULLQUARK,                                /* xrm_class */
        TRUE,                                     /* compress_motion */
        XtExposeCompressMaximal,                  /* compress_exposure */
        TRUE,                                     /* compress_enterleave */
        TRUE,                                     /* visible_interest */
        _ScrollMotiveDestroy,                     /* destroy */
        _ScrollMotiveResize,                      /* resize */
        _ScrollMotiveExpose,                      /* expose */
        (XtSetValuesFunc) _ScrollMotiveSetValues, /* set_values */
        NULL,                                     /* set_values_hook */
        XtInheritSetValuesAlmost,                 /* set_values_almost */
        NULL,                                     /* get_values_hook */
        NULL,                                     /* accept_focus */
        XtVersion,                                /* version */
        NULL,                                     /* callback_offsets */
        NULL,                                     /* tm_table */
        XtInheritQueryGeometry,                   /* query_geometry */
        NULL,                                     /* display_accelerator */
        NULL                                      /* extension */
    },
    /* composite_class */
    {
        XtInheritGeometryManager,                  /* geometry_manager */
        XtInheritChangeManaged,                   /* change_managed */
        XtInheritInsertChild,                     /* insert_child */
        XtInheritDeleteChild,                     /* delete_child */
        NULL                                      /* extension */
    },
    /* constraint_class */
    {
        NULL,                                     /* subresources */
        0,                                        /* subresource_count */
        0,                                        /* constraint_size */
        NULL,                                     /* initialize */
        NULL,                                     /* destroy */
        NULL,                                     /* set_values */
        NULL                                      /* extension */
    },
    /* manager_class */
    {
        NULL,                                     /* translations */
        NULL,                                     /* syn_resources */
        0,                                        /* num_syn_resources */
        NULL,                                     /* syn_constraint_resources */
        0,                                        /* num_syn_constraint_resources */
        XmInheritParentProcess,                   /* parent_process */
        NULL                                      /* extension */
    },
    /* drawing_area_class */
    {
        NULL                                      /* extension */
    },
    /* scroll_motive_class */
    {
        0                                         /* mumble */
    }
};

WidgetClass scrollMotiveWidgetClass = (WidgetClass) &scrollMotiveClassRec;

/****************************************************************
 *
 * Initialize
 *
 ****************************************************************/

/* ARGSUSED */
static void
_ScrollMotiveInitialize(Widget request, Widget new_w,
                        ArgList args, Cardinal *num_args)
{
    ScrollMotiveWidget smw = (ScrollMotiveWidget) new_w;

    smw->scroll.fonts = NULL;
    smw->scroll.line_pointer = 0;
    smw->scroll.lines = 0;
    smw->scroll.top_line = NULL;
    smw->scroll.indent = 15;
    smw->scroll.force_vert = True;
    smw->scroll.h_width = 0;
    smw->scroll.font_height = 0;
    smw->scroll.num_visible_lines = 0;
    smw->scroll.draw_width = 0;
    smw->scroll.draw_height = 0;

    /* Render context — zeroed, will be set up in Realize */
    memset(&smw->scroll.render_ctx, 0, sizeof(XftRenderingContext));

    smw->scroll.back_pixmap = XmUNSPECIFIED_PIXMAP;
    smw->scroll.scrollbar = NULL;
    smw->scroll.visual = NULL;
    smw->scroll.colormap = None;

    smw->scroll.fg_color.pixel = 0;
    smw->scroll.bg_color.pixel = 0;

    smw->scroll.file = NULL;
}

/****************************************************************
 *
 * VerticalScroll — XmScrollBar increment/decrement callback
 *
 ****************************************************************/

/* ARGSUSED */
static void
VerticalScroll(Widget w, XtPointer client_data, XtPointer call_data)
{
    XmScrollBarCallbackStruct *cbs = (XmScrollBarCallbackStruct *) call_data;
    ScrollMotiveWidget smw = (ScrollMotiveWidget) XtParent(w);
    int new_line;
    int max_line;

    if (smw->scroll.top_line == NULL)
        return;

    switch (cbs->reason) {
    case XmCR_INCREMENT:
        new_line = smw->scroll.line_pointer + 1;
        break;
    case XmCR_DECREMENT:
        new_line = smw->scroll.line_pointer - 1;
        break;
    case XmCR_PAGE_INCREMENT:
        new_line = smw->scroll.line_pointer + smw->scroll.num_visible_lines;
        break;
    case XmCR_PAGE_DECREMENT:
        new_line = smw->scroll.line_pointer - smw->scroll.num_visible_lines;
        break;
    default:
        return;
    }

    max_line = smw->scroll.lines - smw->scroll.num_visible_lines;
    if (max_line < 0)
        max_line = 0;

    if (new_line < 0)
        new_line = 0;
    else if (new_line > max_line)
        new_line = max_line;

    if (new_line == smw->scroll.line_pointer)
        return;

    smw->scroll.line_pointer = new_line;

    if (smw->scroll.scrollbar != NULL) {
        XmScrollBarSetValues(smw->scroll.scrollbar,
                             smw->scroll.line_pointer,
                             smw->scroll.num_visible_lines,
                             1, smw->scroll.num_visible_lines,
                             False);
    }

    XClearArea(XtDisplay((Widget) smw), XtWindow((Widget) smw),
               0, 0,
               ((Widget) smw)->core.width,
               ((Widget) smw)->core.height,
               False);
    _ScrollMotiveDrawLines((Widget) smw,
                           smw->scroll.line_pointer,
                           smw->scroll.line_pointer +
                           smw->scroll.num_visible_lines);
}

/****************************************************************
 *
 * VerticalJump — XmScrollBar drag/valueChanged callback
 *
 ****************************************************************/

/* ARGSUSED */
static void
VerticalJump(Widget w, XtPointer client_data, XtPointer call_data)
{
    XmScrollBarCallbackStruct *cbs = (XmScrollBarCallbackStruct *) call_data;
    ScrollMotiveWidget smw = (ScrollMotiveWidget) XtParent(w);
    int max_line;

    if (smw->scroll.top_line == NULL)
        return;

    smw->scroll.line_pointer = cbs->value;

    max_line = smw->scroll.lines - smw->scroll.num_visible_lines;
    if (max_line < 0)
        max_line = 0;

    if (smw->scroll.line_pointer < 0)
        smw->scroll.line_pointer = 0;
    else if (smw->scroll.line_pointer > max_line)
        smw->scroll.line_pointer = max_line;

    XClearArea(XtDisplay((Widget) smw), XtWindow((Widget) smw),
               0, 0,
               ((Widget) smw)->core.width,
               ((Widget) smw)->core.height,
               False);
    _ScrollMotiveDrawLines((Widget) smw,
                           smw->scroll.line_pointer,
                           smw->scroll.line_pointer +
                           smw->scroll.num_visible_lines);
}

/****************************************************************
 *
 * Realize — create Xft drawing context and back buffer
 *
 ****************************************************************/

static void
_ScrollMotiveRealize(Widget w, XtValueMask *mask,
                      XSetWindowAttributes *attrs)
{
    ScrollMotiveWidget smw = (ScrollMotiveWidget) w;
    Display *dpy;

    (*xmDrawingAreaClassRec.core_class.realize) (w, mask, attrs);

    dpy = XtDisplay(w);

    /*
     * Get the widget's actual visual and colormap from the realized
     * window. Do NOT use DefaultVisual() or DefaultColormap() — the
     * widget may be on a non-default visual (e.g. TrueColor on a
     * PseudoColor default screen, or in an XmVisualPolicy-driven
     * colormap).
     */
    {
        XWindowAttributes win_attrs;
        XGetWindowAttributes(dpy, XtWindow(w), &win_attrs);
        smw->scroll.visual = win_attrs.visual;
        smw->scroll.colormap = win_attrs.colormap;
    }

    smw->scroll.render_ctx.draw = XftDrawCreate(
        dpy, XtWindow(w),
        smw->scroll.visual, smw->scroll.colormap);

    /*
     * Allocate Xft colors from the widget's foreground/background
     * pixels. Convert pixel values to XRenderColor via XQueryColor
     * so Xft works correctly with any visual/colormap.
     */
    {
        XColor exact;
        XRenderColor xr_color;

        exact.pixel = smw->manager.foreground;
        XQueryColor(dpy, smw->scroll.colormap, &exact);
        xr_color.red = exact.red;
        xr_color.green = exact.green;
        xr_color.blue = exact.blue;
        xr_color.alpha = 0xffff;
        XftColorAllocValue(dpy, smw->scroll.visual,
                            smw->scroll.colormap, &xr_color,
                            &smw->scroll.fg_color);

        exact.pixel = w->core.background_pixel;
        XQueryColor(dpy, smw->scroll.colormap, &exact);
        xr_color.red = exact.red;
        xr_color.green = exact.green;
        xr_color.blue = exact.blue;
        xr_color.alpha = 0xffff;
        XftColorAllocValue(dpy, smw->scroll.visual,
                            smw->scroll.colormap, &xr_color,
                            &smw->scroll.bg_color);
    }

    smw->scroll.render_ctx.fg_color = smw->scroll.fg_color;
    smw->scroll.render_ctx.bg_color = smw->scroll.bg_color;
    smw->scroll.render_ctx.visual = smw->scroll.visual;
    smw->scroll.render_ctx.colormap = smw->scroll.colormap;

    smw->scroll.back_pixmap = XCreatePixmap(
        dpy, XtWindow(w),
        w->core.width, w->core.height,
        w->core.depth);

    smw->scroll.draw_width = w->core.width;
    smw->scroll.draw_height = w->core.height;

    _ScrollMotiveCreateScrollbar(w);
}

/****************************************************************
 *
 * CreateScrollbar — create the XmScrollBar child
 *
 ****************************************************************/

static void
_ScrollMotiveCreateScrollbar(Widget w)
{
    ScrollMotiveWidget smw = (ScrollMotiveWidget) w;
    Arg args[16];
    Cardinal n = 0;
    int slider_size;

    if (smw->scroll.scrollbar != NULL)
        return;

    XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
    XtSetArg(args[n], XmNprocessingDirection, XmMAX_ON_BOTTOM); n++;
    XtSetArg(args[n], XmNwidth, 20); n++;
    XtSetArg(args[n], XmNminimum, 0); n++;
    XtSetArg(args[n], XmNmaximum,
             (smw->scroll.lines > 0) ? smw->scroll.lines : 1); n++;
    XtSetArg(args[n], XmNincrement, 1); n++;
    XtSetArg(args[n], XmNpageIncrement,
             (smw->scroll.num_visible_lines > 0)
                 ? smw->scroll.num_visible_lines : 1); n++;

    slider_size = (smw->scroll.lines > 0 && smw->scroll.font_height > 0)
        ? (int)((float)w->core.height /
                (float)(smw->scroll.lines * smw->scroll.font_height))
        : 1;
    if (slider_size < 1) slider_size = 1;
    XtSetArg(args[n], XmNsliderSize, slider_size); n++;
    XtSetArg(args[n], XmNvalue, smw->scroll.line_pointer); n++;

    smw->scroll.scrollbar = XtCreateWidget(
        "scrollbar", xmScrollBarWidgetClass, w, args, n);

    XtAddCallback(smw->scroll.scrollbar,
                  XmNincrementCallback, VerticalScroll, NULL);
    XtAddCallback(smw->scroll.scrollbar,
                  XmNdecrementCallback, VerticalScroll, NULL);
    XtAddCallback(smw->scroll.scrollbar,
                  XmNpageIncrementCallback, VerticalScroll, NULL);
    XtAddCallback(smw->scroll.scrollbar,
                  XmNpageDecrementCallback, VerticalScroll, NULL);
    XtAddCallback(smw->scroll.scrollbar,
                  XmNvalueChangedCallback, VerticalJump, NULL);
    XtAddCallback(smw->scroll.scrollbar,
                  XmNdragCallback, VerticalJump, NULL);

    XtManageChild(smw->scroll.scrollbar);
}

/****************************************************************
 *
 * Expose — clear background and redraw visible lines
 *
 ****************************************************************/

/* ARGSUSED */
static void
_ScrollMotiveExpose(Widget w, XEvent *event, Region region)
{
    ScrollMotiveWidget smw = (ScrollMotiveWidget) w;

    if (!XtIsRealized(w))
        return;

    if (smw->scroll.top_line == NULL || smw->scroll.fonts == NULL)
        return;

    /*
     * Clear the damaged area first to prevent anti-aliased ghosting.
     * When switching fonts (normal→bold→italic), leftover anti-aliased
     * pixels from the previous font can leave visual artifacts if we
     * don't clear before drawing.
     */
    if (event != NULL && (event->type == Expose || event->type == GraphicsExpose)) {
        XClearArea(XtDisplay(w), XtWindow(w),
                    event->xexpose.x, event->xexpose.y,
                    event->xexpose.width, event->xexpose.height,
                    False);
    } else {
        /* Full redraw */
        XClearArea(XtDisplay(w), XtWindow(w),
                    0, 0, w->core.width, w->core.height, False);
    }

    /* Draw visible lines */
    _ScrollMotiveDrawLines(w, smw->scroll.line_pointer,
                            smw->scroll.line_pointer +
                            smw->scroll.num_visible_lines);
}

/****************************************************************
 *
 * Resize — update back buffer and recalculate visible lines
 *
 ****************************************************************/

static void
_ScrollMotiveResize(Widget w)
{
    ScrollMotiveWidget smw = (ScrollMotiveWidget) w;
    Dimension scrollbar_width = 0;

    if (!XtIsRealized(w))
        return;

    /* Recreate back_pixmap to match new widget dimensions */
    if (smw->scroll.back_pixmap != XmUNSPECIFIED_PIXMAP) {
        XFreePixmap(XtDisplay(w), smw->scroll.back_pixmap);
    }
    smw->scroll.back_pixmap = XCreatePixmap(
        XtDisplay(w), XtWindow(w),
        w->core.width, w->core.height,
        w->core.depth);

    /* Recalculate number of visible lines */
    if (smw->scroll.font_height > 0) {
        smw->scroll.num_visible_lines =
            w->core.height / smw->scroll.font_height + 1;
    }

    if (smw->scroll.scrollbar != NULL) {
        scrollbar_width = smw->scroll.scrollbar->core.width +
                          smw->scroll.scrollbar->core.border_width;
    }
    smw->scroll.draw_width = w->core.width - scrollbar_width;
    smw->scroll.draw_height = w->core.height;

    if (smw->scroll.render_ctx.draw != NULL) {
        XftChangeDrawable(&smw->scroll.render_ctx, XtWindow(w));
    }

    if (smw->scroll.scrollbar != NULL) {
        int slider_size = (smw->scroll.lines > 0 && smw->scroll.font_height > 0)
            ? (int)((float)w->core.height /
                    (float)(smw->scroll.lines * smw->scroll.font_height))
            : 1;
        if (slider_size < 1) slider_size = 1;
        if (slider_size > smw->scroll.lines && smw->scroll.lines > 0)
            slider_size = smw->scroll.lines;

        XmScrollBarSetValues(smw->scroll.scrollbar,
                              smw->scroll.line_pointer,
                              slider_size, 1,
                              smw->scroll.num_visible_lines, True);
    }

    /* Trigger full redraw */
    XClearArea(XtDisplay(w), XtWindow(w), 0, 0,
               w->core.width, w->core.height, True);
}

/****************************************************************
 *
 * Destroy — free all Xft and X resources
 *
 ****************************************************************/

static void
_ScrollMotiveDestroy(Widget w)
{
    ScrollMotiveWidget smw = (ScrollMotiveWidget) w;
    Display *dpy = XtDisplay(w);

    /* Free XftDraw */
    if (smw->scroll.render_ctx.draw != NULL) {
        XftDrawDestroy(smw->scroll.render_ctx.draw);
        smw->scroll.render_ctx.draw = NULL;
    }

    /* Free XftColors */
    if (smw->scroll.visual != NULL) {
        XftColorFree(dpy, smw->scroll.visual,
                     smw->scroll.colormap, &smw->scroll.fg_color);
        XftColorFree(dpy, smw->scroll.visual,
                     smw->scroll.colormap, &smw->scroll.bg_color);
    }

    /* Free backing pixmap */
    if (smw->scroll.back_pixmap != XmUNSPECIFIED_PIXMAP) {
        XFreePixmap(dpy, smw->scroll.back_pixmap);
        smw->scroll.back_pixmap = XmUNSPECIFIED_PIXMAP;
    }

    /* Free line pointers */
    if (smw->scroll.top_line != NULL) {
        XtFree(*(smw->scroll.top_line));        /* Free the page buffer */
        XtFree((char *) smw->scroll.top_line);   /* Free the line pointer array */
        smw->scroll.top_line = NULL;
    }

    /* Free fonts */
    if (smw->scroll.fonts != NULL) {
        XftFreeFontSet(dpy, smw->scroll.fonts);
        smw->scroll.fonts = NULL;
    }

    /* Destroy scrollbar child */
    if (smw->scroll.scrollbar != NULL) {
        XtDestroyWidget(smw->scroll.scrollbar);
        smw->scroll.scrollbar = NULL;
    }

    /* Close file if open */
    if (smw->scroll.file != NULL) {
        fclose(smw->scroll.file);
        smw->scroll.file = NULL;
    }
}

/****************************************************************
 *
 * SetValues — handle resource changes
 *
 ****************************************************************/

/* ARGSUSED */
static Boolean
_ScrollMotiveSetValues(Widget old, Widget request, Widget new_w,
                       ArgList args, Cardinal *num_args)
{
    ScrollMotiveWidget oldsmw = (ScrollMotiveWidget) old;
    ScrollMotiveWidget newsmw = (ScrollMotiveWidget) new_w;
    Boolean redraw = False;

    /* If force_vert changed, need to reconfigure scrollbar visibility */
    if (oldsmw->scroll.force_vert != newsmw->scroll.force_vert) {
        redraw = True;
    }

    /* If indent changed, need full redraw */
    if (oldsmw->scroll.indent != newsmw->scroll.indent) {
        redraw = True;
    }

    /* File change handling — will be expanded in Task 9 */
    if (oldsmw->scroll.file != newsmw->scroll.file) {
        redraw = True;
    }

    return redraw;
}

/****************************************************************
 *
 * _ScrollMotiveDrawLines — render visible lines with Xft
 *
 ****************************************************************/

static void
_ScrollMotiveDrawLines(Widget w, int from_line, int to_line)
{
    ScrollMotiveWidget smw = (ScrollMotiveWidget) w;
    XftFont *current_font;
    int y_pos;
    int line;
    int left_margin;

    if (smw->scroll.top_line == NULL || smw->scroll.fonts == NULL)
        return;

    if (smw->scroll.fonts->normal == NULL)
        return;

    current_font = smw->scroll.fonts->normal;

    left_margin = (int) smw->scroll.indent;
    if (smw->scroll.scrollbar != NULL) {
        left_margin += smw->scroll.scrollbar->core.width +
                       smw->scroll.scrollbar->core.border_width;
    }

    y_pos = XftGetFontAscent(current_font);

    for (line = from_line;
         line < to_line && line < smw->scroll.lines;
         line++) {
        char *text = smw->scroll.top_line[line];

        if (text == NULL || *text == '\0' || *text == '\n') {
            y_pos += XftGetFontHeight(current_font);
            continue;
        }

        XClearArea(XtDisplay(w), XtWindow(w),
                    left_margin, y_pos - XftGetFontAscent(current_font),
                    w->core.width - left_margin,
                    XftGetFontHeight(current_font),
                    False);

        XftDrawStringUtf8(smw->scroll.render_ctx.draw,
                          &smw->scroll.fg_color,
                          current_font,
                          left_margin, y_pos,
                          (const FcChar8 *) text,
                          (int) strlen(text));

        y_pos += XftGetFontHeight(current_font);
    }
}

/****************************************************************
 *
 * Public API
 *
 ****************************************************************/

/* ARGSUSED */
void
ScrollMotiveSetFile(Widget w, FILE *file)
{
    /* Stub — file loading will be implemented in Task 9 */
    ScrollMotiveWidget smw = (ScrollMotiveWidget) w;
    smw->scroll.file = file;
}

/* ARGSUSED */
void
ScrollMotiveScrollToLine(Widget w, int line)
{
    /* Stub — scrollbar callback integration in Task 7 */
    (void) w;
    (void) line;
}

int
ScrollMotiveGetTotalLines(Widget w)
{
    return ((ScrollMotiveWidget) w)->scroll.lines;
}

int
ScrollMotiveGetVisibleLines(Widget w)
{
    return ((ScrollMotiveWidget) w)->scroll.num_visible_lines;
}