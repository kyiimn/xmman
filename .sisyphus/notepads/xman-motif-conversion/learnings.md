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
