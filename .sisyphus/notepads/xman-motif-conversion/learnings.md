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
