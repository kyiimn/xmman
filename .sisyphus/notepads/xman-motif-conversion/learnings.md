# Learnings

## 2026-06-24 Session Start

- Project: xman X11/Xaw → Motif/Xft conversion
- Key files: buttons.c, handler.c, misc.c, search.c, man.h, defs.h, globals.h, main.c, tkfuncs.c, help.c
- Reference project: ~/src/xcalc (xmcalc conversion)
- Key patterns from xcalc: All XmForm attachments set programmatically, Arm()/Disarm() required in button translations, no Xaw Dialog equivalent
- Font strategy: D2Coding (monospace) for man page body, NanumMyeongjo (proportional) for UI chrome
- Xft approach: Direct Xft rendering for man page content, XmFONT_IS_XFT for UI chrome widgets
- ScrollByLine → XmDrawingArea-based ScrollMotive with direct Xft rendering
## Build System Update (configure.ac + Makefile.am)

### Key Findings
- **Xft pkg-config name is `xft` (NOT `xft2`)** — on this Arch system, the package is `xft` not `xft2`. The task description said `xft2` but that doesn't exist here.
- **Motif pkg-config packages (`xm`, `motif`, `openmotif`) not available** — had to fall back to AC_CHECK_LIB for Motif. The AC_CHECK_LIB fallback pattern from xcalc works correctly.
- **xorg-util-macros not installed by default** — needed to build from source and install to ~/.local to get autoreconf working.
- **ACLOCAL_FLAGS=-I ~/.local/share/aclocal** needed for autoreconf to find xorg-macros.m4.
- **PKG_CONFIG_PATH needs ~/.local/share/pkgconfig** for the macros pkg file.

### configure.ac Changes
- Replaced `PKG_CHECK_MODULES(XMAN, [xproto >= 7.0.17 xaw7 xt])` with:
  1. Nested PKG_CHECK_MODULES for xm/motif/openmotif with AC_CHECK_LIB fallback
  2. `PKG_CHECK_MODULES([XFT], [xft fontconfig])`
  3. `PKG_CHECK_MODULES(XMAN, [xproto >= 7.0.17 xt])` (core deps only)
  4. XMAN_CFLAGS/LIBS merging from XM and XFT variables

### Makefile.am Changes
- Removed ScrollByL.c, ScrollByL.h, ScrollByLP.h from xman_SOURCES
- Added scroll_motive.c, scroll_motive.h, scroll_motiveP.h, xft_utils.c, xft_utils.h, xman_fonts.c, xman_fonts.h
- Added $(XM_CFLAGS) and $(XFT_CFLAGS) to AM_CFLAGS
- xman_LDADD unchanged (still uses $(XMAN_LIBS) which now includes XM and XFT)

## xft_utils Subsystem (Task 4)

### Key Findings
- **`XftFontSet` is already a typedef** in `<X11/Xft/Xft.h>` (it's `FcFontSet`). Our struct was renamed to `XmanFontSet` to avoid conflict.
- **`XftCreateContext` needs `Display *dpy`** — not in original task spec signature but required by `XftDrawCreate`, `XftColorAllocValue`, `XftColorFree`. Added as first parameter.
- **`XftFreeFontSet` needs `Display *dpy`** — required by `XftFontClose`. Added parameter.
- **`XftDestroyContext` needs `Display *dpy`** — required by `XftColorFree`. Added parameter.
- **man.h now includes `xft_utils.h`** — needed so the `XmanFontSet` type is visible where `XmanFonts` struct uses it.
- **Font color allocation**: `XftCreateContext` uses `XQueryColor` to convert pixel values to `XRenderColor`, with black (0,0,0,0xffff) and white (0xffff,0xffff,0xffff,0xffff) as defaults when pixel=0.
- **CJK fallback chains hardcoded in `XftLoadFontSet`**: Monospace chain for body, with style variants for bold/italic.
- Clean compile with `gcc -c -I. -Wall -Wextra -pedantic xft_utils.c $(pkg-config --cflags xft fontconfig)` — zero warnings.
- LSP clangd errors about `ft2build.h not found` are false positives from clangd not finding FreeType includes (gcc compiles fine).

## xman_fonts Subsystem (Task 4b)

### Key Decisions
- **Forward declaration for XmanFonts**: xman_fonts.h uses `struct _XmanFonts;` forward declaration instead of including man.h, avoiding circular dependency. xman_fonts.c includes man.h for the full type definition.
- **scroll_motive.h stub**: Created minimal stub header so man.h can compile before the full ScrollMotive widget implementation. Just declares WidgetClass and widget type pointers.
- **XftFontBuildPattern uses fontconfig FC_WEIGHT_BOLD/FC_SLANT_ITALIC constants**: These are from `<fontconfig/fontconfig.h>`, already available via xft dependency.
- **XtResource names for new font patterns**: Used "FontPattern" as the resource class (not XtCFont which is for XFontStruct). Kept old "directoryFontNormal" XtRFontStruct resource as-is (for backward compat) and added new "directoryFontPattern" XtRString resource.
- **XmanFreeFonts doesn't free the struct itself**: Because XmanFonts is embedded in Xman_Resources, not heap-allocated separately.

### File Changes
- **xman_fonts.h** (new): Font constant defines, XftFontBuildPattern/XmanLoadManpageFonts/XmanLoadDirectoryFont/XmanFreeFonts prototypes
- **xman_fonts.c** (new): Implementation of all 4 functions, uses XftLoadFontSet and XftFontOpenWithFallback from xft_utils
- **man.h**: Added `#include "xman_fonts.h"`, added 5 `char *` font pattern fields to Xman_Resources
- **main.c**: Added `#include "xman_fonts.h"`, added 5 new XtResource entries for font patterns
- **scroll_motive.h** (new stub): Minimal header to unblock compilation until Task 5-10

## Task T5: scroll_motive.h and scroll_motiveP.h headers

### Key Findings
- `scroll_motive.h` needed `#include <stdio.h>` for `FILE *` in `ScrollMotiveSetFile` prototype — `X11/Intrinsic.h` does not pull it in
- `scroll_motiveP.h` must include `<Xm/XmP.h>` and `<Xm/DrawingAP.h>` (not just `<Xm/Xm.h>` and `<Xm/DrawingA.h>`) to get `CoreClassPart`, `CorePart`, `XmDrawingAreaClassPart`, `XmDrawingAreaPart`
- XmDrawingArea class hierarchy: Core → Composite → Constraint → XmManager → XmDrawingArea, so instance record has 5 parts: CorePart, CompositePart, ConstraintPart, XmManagerPart, XmDrawingAreaPart
- Xft headers require freetype2 include path (`-I/usr/include/freetype2`) for `ft2build.h`
- Resource names use `XtNman*` prefix (not `XtNscrollMotive*`) to match original ScrollByLine naming convention where resource names refer to the content, not the widget

### Compilation flags for headers with Xft+Motif
```
gcc -c -I. -I/usr/include -I/usr/include/X11 -I/usr/include/Xft -I/usr/include/freetype2 -I/usr/include/libpng16
```

## Task T6: scroll_motive.c Implementation

### Key Findings
- **CoreClassPart field order**: `superclass, class_name, widget_size, class_initialize, class_part_init, class_inited, initialize, initialize_hook, realize, ...` — missing `class_name` causes all subsequent fields to shift, producing cascading type errors.
- **XmCompositeClassPart uses `XtInherit*` macros** (not `_XmInherit*`): `XtInheritGeometryManager`, `XtInheritChangeManaged`, `XtInheritInsertChild`, `XtInheritDeleteChild`. The `_XmInherit*` macros don't exist for these fields.
- **No `XftDrawVisualOfScreen()` function exists**. Use `XGetWindowAttributes()` after realize to get the widget's actual visual from its window attributes. This is the correct way to get the visual for Xft rendering.
- **No `w->core.foreground` field** in XmManager widgets — foreground is `smw->manager.foreground` (from XmManagerPart). Background is still `w->core.background_pixel`.
- **XtCIndent is not a standard Xt resource class string** — must be defined locally as `"Indent"`.
- **`XmUNSPECIFIED_PIXMAP`** is the correct sentinel for uninitialized Pixmap in Motif widgets.
- **Clean compile** with: `gcc -c -I. -Wall -Wextra -Wno-unused-parameter scroll_motive.c -I/usr/include/freetype2 -I/usr/include/libpng16 $(pkg-config --cflags xft fontconfig) -I/usr/include/Xm`
- **LSP false positives**: clangd may report errors about `ft2build.h` not found — this is a clangd include path issue, not a real compilation error.
- **Anti-aliased ghosting prevention**: Clear background with `XClearArea` before drawing each text line to prevent leftover sub-pixel artifacts from previous font renderings.
- **Xft color allocation pattern**: pixel → XQueryColor → XRenderColor → XftColorAllocValue. This ensures correct colors on any visual/colormap.

## Task T7: Scrollbar Integration

### Key Findings
- **XmScrollBar callbacks differ from Xaw**: Xaw uses `XtNscrollProc` (pixel-based) and `XtNjumpProc` (float 0-1). Xm uses `XmNincrementCallback`, `XmNdecrementCallback`, `XmNpageIncrementCallback`, `XmNpageDecrementCallback` (all via `XmScrollBarCallbackStruct`), plus `XmNvalueChangedCallback` and `XmNdragCallback` for thumb positioning.
- **XmScrollBarCallbackStruct fields**: `reason`, `value`, `pixel_increment`, `slider_size`, `increment`, `page_increment`. For increment/decrement callbacks, use `cbs->reason` to distinguish direction. For drag/valueChanged, use `cbs->value` as the new line_pointer.
- **Callback widget is the scrollbar, not the parent**: VerticalScroll/VerticalJump receive the XmScrollBar widget as `w`, so `XtParent(w)` gets the ScrollMotive parent. This matches Xaw's pattern where the scrollbar widget is the callback's first argument.
- **XmScrollBarSetValues(widget, value, slider_size, increment, page_increment, notify)**: The `notify` parameter (True/False) controls whether callbacks fire. Use False in VerticalScroll to avoid recursive callback loops.
- **scrollbar_width offset**: `scrollbar->core.width + scrollbar->core.border_width` gives the total space consumed by the scrollbar. This is added to `indent` in _ScrollMotiveDrawLines to offset text left margin.
- **draw_width accounts for scrollbar**: In _ScrollMotiveResize, `draw_width = core.width - scrollbar_width` reserves space for the scrollbar.
- **No Xaw scrollbarWidgetClass references**: Verified `nm scroll_motive.o | grep scrollbarWidgetClass` returns 0 matches — all scrollbar functionality uses XmScrollBar.

## T8: Nroff Formatting Parser (scroll_motive.c)

### Key Design Decisions
- **TextSegment stores text inline** (`char text[BUFSIZ]`) rather than pointing into the raw line buffer. This is necessary because nroff formatting strips backspace sequences — the clean text has no direct pointer relationship to the original line.
- **FLUSH_BUF macro defined inside function body** since it references local variables (`bufp`, `buf`, `seg_count`, `h_col`, `x_pos`). `#undef` at function end to prevent scope pollution.
- **WHICH macro replaced with inline ternary** — the original `WHICH(italic, bold)` macro used commas inside ternary, which breaks when passed as a macro argument to FLUSH_BUF. Pre-compute the SegmentType into a local variable instead.
- **Font metrics cached in Realize** — `font_height` and `h_width` set from `XftGetFontHeight/Width(fonts->normal)` during `_ScrollMotiveRealize()` so they're available for the parser's column calculations.

### Nroff Format Parsing Rules (ported from ScrollByL.c)
- **Bold**: `c\bc` (character + backspace + same character) — overstrike pattern
- **Italic**: `_\bc` (underscore + backspace + character)
- **Symbol/bullet**: `o\b+` or `+\bo` — renders as Unicode middle dot (char 183)
- **Tabs**: expanded to next 8-column boundary using `h_col + 8 - (h_col % 8)`
- **Multi-overstrike bold**: `c\bc\bc\bc...` — the while loop skips all repeated backspace pairs

### Pitfalls
- The original `DumpText()` returns the x position for chaining. Our `_ScrollMotiveRenderSegment()` doesn't need this because segments carry their own `x_position`.
- `TextSegment.text[BUFSIZ]` makes the struct ~8KB per segment — 256 segments would be ~2MB on stack. This is fine for a single-line parse but the `MAX_SEGMENTS` limit prevents stack overflow.
- When `bufp > buf` check fails in FLUSH_BUF (empty buffer), the segment is simply skipped — no empty segments are emitted.

## Task T9: File Loading and Line Management

### Key Findings
- **_ScrollMotiveLoadFile pattern**: Direct port of ScrollByL.c's `LoadFile()` — uses `fstat()` + `fread()` to load entire file into a single XtMalloc'd buffer, then builds a `char **top_line` array pointing into the buffer. Each `\n` is replaced with `\0` to terminate lines.
- **Single-buffer approach**: All line pointers point into one contiguous `page` buffer. `_ScrollMotiveFreeLines` frees `*top_line` (the buffer) then `top_line` (the pointer array) — exactly matching the original's `XtFree(*(top_line))` + `XtFree((char *) top_line)` pattern.
- **`nlines` calculation**: `line_pointer - top_line` gives the final line count after parsing, same as original. The realloc to shrink the array is also preserved.
- **XmScrollBarSetValues in SetFile**: Uses `XmMAX_ON_BOTTOM` as the final parameter (processing direction), with initial value 0 and slider_size = num_visible_lines.
- **Realize adds num_visible_lines init**: After font metrics are set, `num_visible_lines = core.height / font_height + 1` is computed before scrollbar creation, matching the original's `Layout()` pattern.
- **`INT_MAX` guard**: Preserved from original ScrollByL.c — files larger than INT_MAX bytes are rejected since XtMalloc takes an int.
- **`#define ADD_MORE_MEM` / `CHAR_PER_LINE`**: These are `#undef`'d after `_ScrollMotiveLoadFile` to avoid polluting the namespace, matching the original's scoping convention.

## Task T10: ScrollMotive Integration (buttons.c, misc.c)

### Changes Made
- **buttons.c `CreateManpageWidget()`**: Replaced `scrollByLineWidgetClass` with `scrollMotiveWidgetClass` in the `XtCreateWidget` call for the manpage widget
- **buttons.c**: Added `#include "scroll_motiveP.h"` for `ScrollMotiveWidget` cast and `scroll` part accessor
- **buttons.c**: Added Xft font loading block after widget creation — calls `XmanLoadManpageFonts()` to get the `XmanFontSet*`, then sets `smw->scroll.fonts`, `font_height`, and `h_width`
- **misc.c `OpenFile()`**: Replaced `XtSetValues(XtNfile)` with direct `ScrollMotiveSetFile()` call, removing the ArgList/num_args pattern entirely
- **handler.c**: No changes needed — no ScrollByLine or XtNfile references
- **help.c**: No changes needed — `OpenHelpfile()` calls `OpenFile()` which is in misc.c (already changed)

### Key Findings
- **Font loading must happen after widget creation but before realize**: `CreateManpageWidget()` creates the widget, then `StartManpage()` calls `XtRealizeWidget()`. Font loading in between means `_ScrollMotiveRealize()` finds `fonts != NULL` and computes metrics.
- **`XScreenNumberOfScreen(XtScreen(w))`**: Returns the screen number (int) for `XmanLoadManpageFonts(Display*, int)`. This is the correct way to get the screen number from a widget.
- **`ScrollMotiveSetFile()` handles all the internal work**: It frees old lines, loads new file, resets line_pointer, updates scrollbar values, and clears the window. No need to set `XtNfile` via resources.
- **`scroll_motiveP.h` required for buttons.c**: The private header is needed to cast the widget to `ScrollMotiveWidget` and access `smw->scroll.fonts`, `font_height`, `h_width`. The public header (`scroll_motive.h`) only exposes the class pointer and API functions.
- **Pre-existing Xaw errors in buttons.c**: The file still has many Xaw widget classes (`formWidgetClass`, `labelWidgetClass`, etc.) that need Motif conversion. These are NOT part of this task.

## Task T11: MakeTopBox Motif Conversion

### Changes Made
- **MakeTopBox()** fully converted from Xaw to Motif widgets:
  - `formWidgetClass` → `xmFormWidgetClass`
  - `labelWidgetClass` → `xmLabelWidgetClass`
  - `commandWidgetClass` → `xmPushButtonWidgetClass`
  - `XtNfromVert` → `XmNtopAttachment/XmNtopWidget` (XmATTACH_WIDGET)
  - `XtNfromHoriz` → `XmNleftAttachment/XmNleftWidget` (XmATTACH_WIDGET)
  - All labels set via `XmStringCreateLocalized()` + `XmNlabelString` + `XmStringFree()`
  - All push buttons have `Arm()/Disarm()` translation overrides for visual feedback
  - Layout: topLabel spans full width, Help+Quit side-by-side below label, Manpage button full width below Help
  - `FormUpWidgets()` call removed from MakeTopBox (XmForm attachments handle layout now)
  - `#define TOPARGS 5` removed (no longer used)
  - Local variables changed: `arglist[TOPARGS]` → `args[8]`, `num_args` → `n`, `command` → `help_cmd/quit_cmd/manpage_cmd`

### Key Findings
- **FormUpWidgets() and ConvertNamesToWidgets() MUST remain**: search.c still calls `FormUpWidgets()` with Xaw dialog widgets. These functions are not exclusive to MakeTopBox.
- **Widget name constants vs labels**: HELP_BUTTON="helpButton", QUIT_BUTTON="quitButton", MANPAGE_BUTTON="manpageButton" are widget NAMES (for XtNameToWidget), not visible labels. Visible labels are "Help", "Quit", "Manual Page".
- **XmForm attachment pattern**: Every widget needs explicit attachments. No attachment = widget collapses to zero size. Full-width widgets need both `XmNleftAttachment=XmATTACH_FORM` and `XmNrightAttachment=XmATTACH_FORM`.
- **Quit button needs XmNrightAttachment=XmATTACH_FORM** to fill the row properly alongside Help.
- **Compilation**: Zero errors from MakeTopBox(). All 16 remaining errors are pre-existing Xaw references in other functions.

## Task T12: Option Menu and Manpage Widget Menu Bar Conversion

### Changes Made
- **CreateOptionMenu()**: Replaced `simpleMenuWidgetClass` → `XmCreatePulldownMenu()`, `smeBSBObjectClass` → `xmPushButtonWidgetClass`, `XtNcallback` → `XmNactivateCallback`, labels via `XmStringCreateLocalized()` + `XmNlabelString` + `XmStringFree()`
- **CreateSectionMenu()**: Same conversion pattern. Also added `man_globals->section_menu = menu` to store the pulldown menu widget. Changed `XtNcallback` → `XmNactivateCallback`, kept `XtNdestroyCallback` for MenuDestroy.
- **CreateManpageWidget()**: Restructured call order — pulldown menus created BEFORE cascade buttons that reference them. `panedWidgetClass` → `xmPanedWindowWidgetClass`, `menuButtonWidgetClass` → `xmCascadeButtonWidgetClass`, `labelWidgetClass` → `xmLabelWidgetClass`. Options cascade uses `XmNsubMenuId` + `XmNlabelString`. Sections cascade similarly. `XtNmenuName` eliminated entirely.
- **StartManpage()**: `XtNlabel` on both_screens_entry → `XmNlabelString` with `XmStringCreateLocalized()`. `XtNpreferredPaneSize` → `XmNpreferredPaneSize`.
- **ChangeLabel() in misc.c**: Replaced `XtNlabel` with `XmNlabelString` + `XmStringCreateLocalized()` + `XmStringFree()`. Removed the old `XtNwidth/XtNheight` workaround.
- **man.h**: Added `section_menu` field to `ManpageGlobals` struct.

### Key Findings
- **Chicken-and-egg problem with XmCascadeButton**: The cascade button needs `XmNsubMenuId` pointing to a pulldown menu widget that must already exist. Solution: create pulldown menus first (via `CreateOptionMenu`/`CreateSectionMenu`), then create cascade buttons referencing `man_globals->option_menu`/`man_globals->section_menu`.
- **Non-full-instance sections button**: When `full_instance` is FALSE, the sections cascade button is created without `XmNsubMenuId` and then `XtSetSensitive(mysections, FALSE)`. This is valid — Motif allows cascade buttons without submenus.
- **`XtNpreferredPaneSize` → `XmNpreferredPaneSize`**: When converting `panedWidgetClass` to `xmPanedWindowWidgetClass`, the resource name also changes prefix.
- **`XmCreatePulldownMenu(parent, name, args, n)`**: This is the Motif convenience function replacing `XtCreatePopupShell(name, simpleMenuWidgetClass, parent, ...)`. The parent should be the shell widget (`mytop`), not the hpane.
- **Pre-existing Xaw errors remain**: `viewportWidgetClass`, `listWidgetClass`, `labelWidgetClass` (in MakeSaveWidgets), `dialogWidgetClass`, `XawDialogAddButton`, `XtNdefaultDistance`, `XtNlist` — all in functions not part of this task.
- **`XmStringCreateLocalized((String) option_names[i])`**: Cast to `(String)` needed because `option_names[i]` is `const char *` but `XmStringCreateLocalized` takes `char *`.

## Task T13: Directory List Conversion (Xaw → XmList)

### Changes Made

- **buttons.c `MakeDirectoryBox()`**: Complete rewrite:
  - Removed `CreateList()` function (was only called from old MakeDirectoryBox)
  - `listWidgetClass` → `xmListWidgetClass` (as child of ScrolledWindow)
  - `XtNlist` with `char**` → `XmNitems` with `XmStringTable` + `XmNitemCount`
  - `XtNfont` removed (XmList uses XmString rendering)
  - `XtNcallback` → `XmNdefaultActionCallback` + `XmNsingleSelectionCallback`
  - XmStringTable items freed with `XmStringFree()` + `XtFree()` after widget creation
  - `XmBROWSE_SELECT` selection policy for single-item selection
  - Widget stored in `*dir_disp` (becomes `manpagewidgets.box[section]`) is the XmList itself
  - Parent is `xmScrolledWindowWidgetClass` (already created in CreateManpageWidget)

- **buttons.c `CreateManpageWidget()`**: 
  - `viewportWidgetClass` → `xmScrolledWindowWidgetClass`
  - `XtNallowVert, TRUE` → `XmNscrollingPolicy, XmAUTOMATIC`

- **handler.c `DirectoryHandler()`**: 
  - `XawListReturnStruct *ret_struct` → `XmListCallbackStruct *cbs`
  - `ret_struct->list_index` → `cbs->item_position - 1` (1-based → 0-based)

- **handler.c `DirPopupCallback()`**: 
  - `XawListUnhighlight(box[current_box])` → `XmListDeselectAllItems(box[current_box])`

- **search.c `DoManualSearch()`**: 
  - `XawListUnhighlight(box[current_directory])` → `XmListDeselectAllItems(box[current_directory])`
  - `XawListHighlight(box[i], e_num)` → `XmListSelectPos(box[i], e_num + 1, True)` (0-based → 1-based)

### Key Findings
- **XmList items must be freed after widget creation**: `XmNitems` copies the strings, so the XmStringTable must be freed with `XmStringFree()` per item + `XtFree()` for the table itself.
- **XmList uses 1-based positions**: `item_position` is 1-based, while Xaw's `list_index` was 0-based. All callers must adjust with `-1` (reading) or `+1` (writing).
- **ScrolledWindow with XmAUTOMATIC**: No need to create a separate scrollbar — XmScrolledWindow handles it automatically when `XmNscrollingPolicy` is set to `XmAUTOMATIC`.
- **No need for `XmNfont` on XmList**: XmList uses XmString which carries its own font info.
- **Pre-existing errors unchanged**: All compilation errors in buttons.c, handler.c, and search.c are pre-existing Xaw references in other functions (MakeSaveWidgets, FormUpWidgets, MakeSearchWidget, SearchString, ToggleBothShownState).

## Task T14: Search Dialog Conversion (Xaw → Motif)

### Changes Made

- **search.c `MakeSearchWidget()`**: Complete rewrite from Xaw to Motif:
  - `dialogWidgetClass` → `xmFormWidgetClass` (form container)
  - Xaw dialog's built-in text field → `xmTextFieldWidgetClass` (explicit "value" widget)
  - `XawDialogAddButton(dialog, ...)` × 3 → `xmPushButtonWidgetClass` with `XmNlabelString` + `XmStringCreateLocalized()` + `XmStringFree()`
  - `XtSetKeyboardFocus(dialog, text)` removed (not needed with XmTextField)
  - All 4 widgets have explicit XmForm attachments (`XmNleftAttachment`, `XmNrightAttachment`, `XmNtopAttachment`, `XmNtopWidget`)
  - All 3 buttons have `Arm()/Disarm()` translation overrides
  - `text_widget` stored in `man_globals->text_widget` for `SearchString()` access
  - `FormUpWidgets()` call removed (XmForm handles layout)
  - `XtNtransientFor` arg on transientShell removed (not needed; transientShellWidgetClass works without it)

- **search.c `SearchString()`**: `XawDialogGetValueString(dialog)` → `XmTextFieldGetString(man_globals->text_widget)`. Uses stored `text_widget` instead of `XtNameToWidget`.

- **search.c `DoSearch()` clear_search_string section**: `XtNameToWidget(search_widget, DIALOG)` + `XtSetArg(XtNvalue, "")` + `XtSetValues()` → `XmTextFieldSetString(man_globals->text_widget, "")`

### Key Findings
- **`text_widget` field already existed** in `ManpageGlobals` struct (man.h line 164) — no struct change needed
- **`XmTextFieldGetString()` returns malloc'd string**: Caller must `XtFree()` the result. The existing `DoSearch()` code doesn't free it, which matches the original `XawDialogGetValueString()` behavior (also malloc'd). This is a potential leak in both old and new code, but out of scope.
- **`handler.c Search()` action unchanged**: `XtPopdown(XtParent(XtParent(w)))` still works because widget hierarchy is pushbutton → form → shell (same 2-level depth as Xaw's pushbutton → dialog → shell)
- **No changes to handler.c, buttons.c, or misc.c**: All Xaw references in those files are in other functions (MakeSaveWidgets, FormUpWidgets, PopWarning) and outside scope
- **LSP clangd error about ft2build.h**: Pre-existing false positive, gcc compiles clean
- **XmNeditMode = XmSINGLE_LINE_EDIT**: Required for XmTextField to behave as single-line input (matches Xaw dialog's text widget behavior)
