xmman (XmMan) is a graphical manual page browser using OpenMotif (Xm)
with Xft font rendering for the man page display area.

xmman is a fork of xman, the classic X Window System manual page browser
originally written for the Xaw (Athena Widget Set) toolkit. This fork ports
the entire UI from Xaw to OpenMotif, replacing all Xaw widgets with Motif
equivalents and adding direct Xft font rendering for the man page display
area.

The man page body text uses monospace font D2Coding (with fallback chain:
Noto Sans Mono CJK KR → DejaVu Sans Mono → monospace → fixed).
UI chrome uses proportional font NanumMyeongjo (with fallback chain:
Noto Serif CJK KR → DejaVu Serif → serif → fixed).

The custom ScrollMotive widget (XmDrawingArea-based) provides direct Xft
text rendering with double-buffering, embedded XmScrollBar, and nroff
formatting support (bold, italic, symbol).

All questions regarding this software should be directed at the
Xorg mailing list:

  https://lists.xorg/mailman/listinfo/xorg

The primary development code repository can be found at:

  https://gitlab.freedesktop.org/xorg/app/xman

Please submit bug reports and requests to merge patches there.

For patch submission instructions, see:

  https://www.x.org/wiki/Development/Documentation/SubmittingPatches

