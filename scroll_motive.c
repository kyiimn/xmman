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
 * Nroff formatting types and text segment structure
 *
 ****************************************************************/

#define BACKSPACE 010   /* Same as original ScrollByL.c */

typedef enum {
    SEGMENT_NORMAL,
    SEGMENT_BOLD,
    SEGMENT_ITALIC,
    SEGMENT_SYMBOL
} SegmentType;

typedef struct {
    SegmentType type;
    char text[BUFSIZ];
    int len;
    int x_position;
} TextSegment;

#define MAX_SEGMENTS 256   /* Max segments per line */

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
static int _ScrollMotiveParseLine(const char *line, TextSegment *segments,
                                    int max_segments, int left_margin,
                                    int h_width);
static void _ScrollMotiveRenderSegment(Widget w, TextSegment *seg,
                                         XftRenderingContext *ctx, int y);
static void VerticalScroll(Widget w, XtPointer client_data, XtPointer call_data);
static void VerticalJump(Widget w, XtPointer client_data, XtPointer call_data);
static void _ScrollMotiveLoadFile(Widget w, FILE *file);
static void _ScrollMotiveFreeLines(Widget w);

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

    if (smw->scroll.back_pixmap != XmUNSPECIFIED_PIXMAP && smw->scroll.render_ctx.draw != NULL) {
        XftDrawRect(smw->scroll.render_ctx.draw, &smw->scroll.bg_color,
                     0, 0, ((Widget)smw)->core.width, ((Widget)smw)->core.height);
    }
    _ScrollMotiveDrawLines((Widget) smw,
                            smw->scroll.line_pointer,
                            smw->scroll.line_pointer +
                            smw->scroll.num_visible_lines);
    if (smw->scroll.back_pixmap != XmUNSPECIFIED_PIXMAP) {
        XCopyArea(XtDisplay((Widget) smw), smw->scroll.back_pixmap, XtWindow((Widget) smw),
                  DefaultGC(XtDisplay((Widget) smw), XScreenNumberOfScreen(XtScreen((Widget) smw))),
                  0, 0, ((Widget)smw)->core.width, ((Widget)smw)->core.height, 0, 0);
    }
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

    if (smw->scroll.back_pixmap != XmUNSPECIFIED_PIXMAP && smw->scroll.render_ctx.draw != NULL) {
        XftDrawRect(smw->scroll.render_ctx.draw, &smw->scroll.bg_color,
                     0, 0, ((Widget)smw)->core.width, ((Widget)smw)->core.height);
    }
    _ScrollMotiveDrawLines((Widget) smw,
                            smw->scroll.line_pointer,
                            smw->scroll.line_pointer +
                            smw->scroll.num_visible_lines);
    if (smw->scroll.back_pixmap != XmUNSPECIFIED_PIXMAP) {
        XCopyArea(XtDisplay((Widget) smw), smw->scroll.back_pixmap, XtWindow((Widget) smw),
                  DefaultGC(XtDisplay((Widget) smw), XScreenNumberOfScreen(XtScreen((Widget) smw))),
                  0, 0, ((Widget)smw)->core.width, ((Widget)smw)->core.height, 0, 0);
    }
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

    if (smw->scroll.render_ctx.draw == NULL) {
        XtAppError(XtWidgetToApplicationContext(w),
                   "xman: Failed to create Xft drawing context.\n");
        return;
    }

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

    if (smw->scroll.fonts != NULL && smw->scroll.fonts->normal != NULL) {
        smw->scroll.font_height = XftGetFontHeight(smw->scroll.fonts->normal);
        smw->scroll.h_width = XftGetFontWidth(smw->scroll.fonts->normal);
    }

    if (smw->scroll.font_height > 0) {
        smw->scroll.num_visible_lines =
            w->core.height / smw->scroll.font_height + 1;
    }

    smw->scroll.back_pixmap = XCreatePixmap(
        dpy, XtWindow(w),
        w->core.width, w->core.height,
        w->core.depth);

    /* Bind XftDraw to back_pixmap for double-buffered rendering */
    if (smw->scroll.render_ctx.draw != NULL && smw->scroll.back_pixmap != XmUNSPECIFIED_PIXMAP) {
        XftDrawChange(smw->scroll.render_ctx.draw, smw->scroll.back_pixmap);
    }

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

    if (smw->scroll.back_pixmap == XmUNSPECIFIED_PIXMAP)
        return;

    /*
     * Clear the back buffer before drawing to prevent
     * anti-aliased ghosting from previous content.
     */
    XftDrawRect(smw->scroll.render_ctx.draw, &smw->scroll.bg_color,
                 0, 0, w->core.width, w->core.height);

    /* Draw visible lines into back buffer */
    _ScrollMotiveDrawLines(w, smw->scroll.line_pointer,
                            smw->scroll.line_pointer +
                            smw->scroll.num_visible_lines);

    /* Blit back buffer to window */
    XCopyArea(XtDisplay(w), smw->scroll.back_pixmap, XtWindow(w),
              DefaultGC(XtDisplay(w), XScreenNumberOfScreen(XtScreen(w))),
              0, 0, w->core.width, w->core.height, 0, 0);
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

    /* Rebind XftDraw to the new back_pixmap */
    if (smw->scroll.render_ctx.draw != NULL && smw->scroll.back_pixmap != XmUNSPECIFIED_PIXMAP) {
        XftDrawChange(smw->scroll.render_ctx.draw, smw->scroll.back_pixmap);
    }

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
 * _ScrollMotiveParseLine — Parse nroff formatting in a man page line
 *
 ****************************************************************/

static int
_ScrollMotiveParseLine(const char *line, TextSegment *segments,
                        int max_segments, int left_margin, int h_width)
{
    const char *c = line;
    char buf[BUFSIZ];
    char *bufp = buf;
    int seg_count = 0;
    int h_col = 0;
    int x_pos = left_margin;
    Boolean italicflag = FALSE;
    SegmentType flush_type;

    if (line == NULL || *line == '\0' || *line == '\n') {
        segments[0].type = SEGMENT_NORMAL;
        segments[0].text[0] = '\0';
        segments[0].len = 0;
        segments[0].x_position = left_margin;
        return 1;
    }

#define FLUSH_BUF(seg_type) do {                                       \
    if (bufp > buf) {                                                   \
        int _len = (int)(bufp - buf);                                   \
        if (seg_count >= max_segments) goto done;                       \
        segments[seg_count].type = (seg_type);                          \
        memcpy(segments[seg_count].text, buf, _len);                    \
        segments[seg_count].text[_len] = '\0';                         \
        segments[seg_count].len = _len;                                 \
        segments[seg_count].x_position = x_pos;                         \
        h_col += _len;                                                  \
        seg_count++;                                                    \
        bufp = buf;                                                     \
        x_pos = left_margin + h_col * h_width;                         \
    }                                                                   \
} while(0)

    while (*c != '\0') {
        if (seg_count >= max_segments)
            goto done;

        if ((bufp - buf) > (BUFSIZ - 10)) {
            while (*c != '\n' && *c != '\0')
                c++;
            continue;
        }

        switch (*c) {
        case '\n':
            flush_type = italicflag ? SEGMENT_ITALIC : SEGMENT_NORMAL;
            FLUSH_BUF(flush_type);
            goto done;

        case '\t':
            flush_type = italicflag ? SEGMENT_ITALIC : SEGMENT_NORMAL;
            FLUSH_BUF(flush_type);
            italicflag = FALSE;
            h_col = h_col + 8 - (h_col % 8);
            x_pos = left_margin + h_col * h_width;
            c++;
            break;

        case BACKSPACE:
            if (c > line && c[-1] == c[1] && c[1] != BACKSPACE) {
                if (bufp > buf) {
                    bufp--;
                    flush_type = italicflag ? SEGMENT_ITALIC : SEGMENT_NORMAL;
                    FLUSH_BUF(flush_type);
                }
                bufp = buf;
                *bufp++ = c[1];
                FLUSH_BUF(SEGMENT_BOLD);
                italicflag = FALSE;
                while (*c == BACKSPACE && c > line && c[-1] == c[1])
                    c += 2;
                c--;
            }
            else if (c > line &&
                     ((c[-1] == 'o' && c[1] == '+') ||
                      (c[-1] == '+' && c[1] == 'o'))) {
                if (bufp > buf) {
                    bufp--;
                    flush_type = italicflag ? SEGMENT_ITALIC : SEGMENT_NORMAL;
                    FLUSH_BUF(flush_type);
                }
                bufp = buf;
                *bufp = (char) 183;
                FLUSH_BUF(SEGMENT_SYMBOL);
                c++;
            }
            else {
                if (bufp > buf)
                    bufp--;
                c++;
            }
            break;

        case '_':
            if (*(c + 1) == BACKSPACE) {
                if (!italicflag) {
                    FLUSH_BUF(SEGMENT_NORMAL);
                    italicflag = TRUE;
                }
                c += 2;
                *bufp++ = *c;
                break;
            }
            /* FALLTHROUGH */

        default:
            if (italicflag) {
                FLUSH_BUF(SEGMENT_ITALIC);
            }
            *bufp++ = *c;
            break;
        }
        c++;
    }

    flush_type = italicflag ? SEGMENT_ITALIC : SEGMENT_NORMAL;
    FLUSH_BUF(flush_type);

#undef FLUSH_BUF

done:
    return seg_count;
}

/****************************************************************
 *
 * _ScrollMotiveRenderSegment — Render a single text segment with Xft
 *
 ****************************************************************/

static void
_ScrollMotiveRenderSegment(Widget w, TextSegment *seg,
                             XftRenderingContext *ctx, int y)
{
    ScrollMotiveWidget smw = (ScrollMotiveWidget) w;
    XftFont *font;
    int font_height, font_ascent;
    int width;

    if (seg->len == 0)
        return;

    switch (seg->type) {
    case SEGMENT_BOLD:
        font = smw->scroll.fonts->bold;
        break;
    case SEGMENT_ITALIC:
        font = smw->scroll.fonts->italic;
        break;
    case SEGMENT_SYMBOL:
        font = smw->scroll.fonts->symbol;
        break;
    default:
        font = smw->scroll.fonts->normal;
        break;
    }

    if (font == NULL)
        font = smw->scroll.fonts->normal;
    if (font == NULL)
        return;

    font_height = XftGetFontHeight(font);
    font_ascent = XftGetFontAscent(font);
    width = seg->len * smw->scroll.h_width;

    XftDrawRect(ctx->draw, &ctx->bg_color,
                 seg->x_position, y - font_ascent,
                 width, font_height);

    XftDrawStringUtf8(ctx->draw, &ctx->fg_color, font,
                       seg->x_position, y,
                       (const FcChar8 *) seg->text, seg->len);
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
    int h_width;
    int font_ascent;
    int font_height;
    TextSegment segments[MAX_SEGMENTS];
    int num_segs;
    int i;

    if (smw->scroll.top_line == NULL || smw->scroll.fonts == NULL)
        return;

    if (smw->scroll.fonts->normal == NULL)
        return;

    current_font = smw->scroll.fonts->normal;
    font_height = XftGetFontHeight(current_font);
    font_ascent = XftGetFontAscent(current_font);
    h_width = XftGetFontWidth(current_font);

    left_margin = (int) smw->scroll.indent;
    if (smw->scroll.scrollbar != NULL) {
        left_margin += smw->scroll.scrollbar->core.width +
                       smw->scroll.scrollbar->core.border_width;
    }

    y_pos = font_ascent;

    for (line = from_line;
         line < to_line && line < smw->scroll.lines;
         line++) {
        char *text = smw->scroll.top_line[line];

        if (text == NULL || *text == '\0' || *text == '\n') {
            y_pos += font_height;
            continue;
        }

        num_segs = _ScrollMotiveParseLine(text, segments, MAX_SEGMENTS,
                                           left_margin, h_width);

        for (i = 0; i < num_segs; i++) {
            if (segments[i].len > 0) {
                _ScrollMotiveRenderSegment(w, &segments[i],
                                            &smw->scroll.render_ctx,
                                            y_pos);
            }
        }

        y_pos += font_height;
    }
}

/****************************************************************
 *
 * _ScrollMotiveLoadFile — read file and build line pointer array
 *
 ****************************************************************/

#define ADD_MORE_MEM 100        /* increment for line pointer reallocs */
#define CHAR_PER_LINE 40        /* initial guess for lines in file */

static void
_ScrollMotiveLoadFile(Widget w, FILE *file)
{
    ScrollMotiveWidget smw = (ScrollMotiveWidget) w;
    char *page;
    char **line_pointer, **top_line;
    int nlines;
    struct stat fileinfo;

    /* Free any existing content first */
    if (smw->scroll.top_line != NULL) {
        _ScrollMotiveFreeLines(w);
    }

    smw->scroll.top_line = NULL;
    smw->scroll.lines = 0;
    smw->scroll.line_pointer = 0;

    if (file == NULL)
        return;

    /*
     * Get file size and allocate a chunk of memory for the file
     * to be copied into.
     */
    if (fstat(fileno(file), &fileinfo) != 0)
        return;

    if (fileinfo.st_size == 0)
        return;

    /* The XtMalloc below is limited to a size of int by the libXt API. */
    if (fileinfo.st_size >= INT_MAX)
        return;

    /* Initial guess for number of lines */
    if ((nlines = fileinfo.st_size / CHAR_PER_LINE) == 0)
        nlines = ADD_MORE_MEM;

    page = XtMalloc(fileinfo.st_size + 1);
    top_line = line_pointer = (char **) XtMalloc(nlines * sizeof(char *));

    /*
     * Copy the file into memory.
     */
    fseek(file, 0L, SEEK_SET);
    if (fread(page, sizeof(char), fileinfo.st_size, file) == 0) {
        XtFree(page);
        XtFree((char *) top_line);
        return;
    }

    /* Put NULL at end of buffer */
    *(page + fileinfo.st_size) = '\0';

    /*
     * Go through the file setting a line pointer to the character
     * after each newline.  If we run out of line pointer space then
     * realloc with space for more lines.
     */
    *line_pointer++ = page;     /* first line points to first char */
    while (*page != '\0') {
        if (*page == '\n') {
            *page = '\0';       /* terminate previous line */
            *line_pointer++ = page + 1;

            if (line_pointer >= top_line + nlines) {
                top_line = (char **) XtRealloc((char *) top_line,
                                               (nlines + ADD_MORE_MEM) * sizeof(char *));
                line_pointer = top_line + nlines;
                nlines += ADD_MORE_MEM;
            }
        }
        page++;
    }

    /*
     * Realloc the line pointer space to take only the minimum
     * amount of memory.
     */
    smw->scroll.lines = nlines = line_pointer - top_line;
    if (nlines > 0) {
        top_line = (char **) XtRealloc((char *) top_line,
                                       nlines * sizeof(char *));
        smw->scroll.top_line = top_line;
    } else {
        XtFree((char *) top_line);
        smw->scroll.top_line = NULL;
    }

    smw->scroll.line_pointer = 0;
}

#undef ADD_MORE_MEM
#undef CHAR_PER_LINE

/****************************************************************
 *
 * _ScrollMotiveFreeLines — free line pointer array and page buffer
 *
 ****************************************************************/

static void
_ScrollMotiveFreeLines(Widget w)
{
    ScrollMotiveWidget smw = (ScrollMotiveWidget) w;

    if (smw->scroll.top_line == NULL) {
        smw->scroll.lines = 0;
        smw->scroll.line_pointer = 0;
        return;
    }

    /* Free the page buffer (first pointer points to start of buffer) */
    XtFree(*(smw->scroll.top_line));

    /* Free the line pointer array itself */
    XtFree((char *) smw->scroll.top_line);

    smw->scroll.top_line = NULL;
    smw->scroll.lines = 0;
    smw->scroll.line_pointer = 0;
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
    ScrollMotiveWidget smw = (ScrollMotiveWidget) w;

    _ScrollMotiveFreeLines(w);

    smw->scroll.file = file;
    _ScrollMotiveLoadFile(w, file);

    smw->scroll.line_pointer = 0;

    if (smw->scroll.scrollbar != NULL && smw->scroll.num_visible_lines > 0) {
        XmScrollBarSetValues(smw->scroll.scrollbar, 0,
                              smw->scroll.num_visible_lines, 1,
                              smw->scroll.num_visible_lines, False);
    }

    if (XtIsRealized(w)) {
        if (smw->scroll.back_pixmap != XmUNSPECIFIED_PIXMAP && smw->scroll.render_ctx.draw != NULL) {
            XftDrawRect(smw->scroll.render_ctx.draw, &smw->scroll.bg_color,
                         0, 0, w->core.width, w->core.height);
        }
        XClearArea(XtDisplay(w), XtWindow(w), 0, 0,
                   smw->core.width, smw->core.height, False);
    }
}

/* ARGSUSED */
void
ScrollMotiveScrollToLine(Widget w, int line)
{
    ScrollMotiveWidget smw = (ScrollMotiveWidget) w;
    int max_line;

    if (line < 0)
        line = 0;

    max_line = smw->scroll.lines - smw->scroll.num_visible_lines;
    if (max_line < 0)
        max_line = 0;

    if (line > max_line)
        line = max_line;

    smw->scroll.line_pointer = line;

    if (smw->scroll.scrollbar != NULL) {
        XmScrollBarSetValues(smw->scroll.scrollbar,
                              smw->scroll.line_pointer,
                              smw->scroll.num_visible_lines,
                              1, smw->scroll.num_visible_lines,
                              False);
    }

    if (XtIsRealized(w)) {
        if (smw->scroll.back_pixmap != XmUNSPECIFIED_PIXMAP && smw->scroll.render_ctx.draw != NULL) {
            XftDrawRect(smw->scroll.render_ctx.draw, &smw->scroll.bg_color,
                         0, 0, w->core.width, w->core.height);
        }
        _ScrollMotiveDrawLines(w, smw->scroll.line_pointer,
                                smw->scroll.line_pointer + smw->scroll.num_visible_lines);
        if (smw->scroll.back_pixmap != XmUNSPECIFIED_PIXMAP) {
            XCopyArea(XtDisplay(w), smw->scroll.back_pixmap, XtWindow(w),
                      DefaultGC(XtDisplay(w), XScreenNumberOfScreen(XtScreen(w))),
                      0, 0, w->core.width, w->core.height, 0, 0);
        }
    }
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