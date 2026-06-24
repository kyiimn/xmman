# Decisions

## Architecture Decisions
- Hybrid Xft approach: Direct Xft for man page body, XmFONT_IS_XFT for UI chrome
- ScrollByLine → XmDrawingArea-based ScrollMotive widget with embedded XmScrollBar
- All XmForm attachments set programmatically (not via resource files)
- Keep existing nroff parser logic, port to Xft rendering
- Font fallback chains: D2Coding→Noto Sans Mono CJK KR→DejaVu Sans Mono→monospace→fixed (body), NanumMyeongjo→Noto Serif CJK KR→DejaVu Serif→serif→fixed (chrome)