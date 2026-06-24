# xman Xaw → Motif/Xft Conversion

## TL;DR

> **Quick Summary**: Convert xman from X11/Xaw (Athena Widget Set) to OpenMotif (Xm), replacing all Xaw widgets with Motif equivalents and adding direct Xft font rendering for the man page display area. Man page body text uses monospace font **D2Coding** (fallback: Noto Sans Mono CJK KR → DejaVu Sans Mono → monospace → fixed). UI chrome (buttons, labels, menus, directory list) uses proportional font **NanumMyeongjo** via XmRenderTable (fallback: Noto Serif CJK KR → DejaVu Serif → serif → fixed). The custom ScrollByLine widget is rewritten as XmDrawingArea-based with direct Xft text rendering.
> 
> **Deliverables**:
> - Fully functional xmman binary with Motif UI and Xft text rendering
> - Xft font utility subsystem (xft_utils.c/h)
> - XmDrawingArea-based ScrollByLine replacement (scroll_motive.c/h)
> - Converted TopBox, Manpage, Menu, Search, Help, Save, Warning dialogs
> - Rewritten app-defaults/Xmman for Motif resources
> - Updated build system (configure.ac, Makefile.am) for Xm + Xft
> - Build smoke test and headless render sanity test
> 
> **Estimated Effort**: XL (20+ tasks across 5 waves)
> **Parallel Execution**: YES - 5 waves
> **Critical Path**: Task 1-3 (build/headers) → Task 4-5 (Xft utils + font subsystem) → Task 6-10 (ScrollByLine rewrite) → Task 11-17 (UI conversion) → Task 18-20 (resources/tests) → Task F1-F4 (final verification)

---

## Context

### Original Request
xman X11/Xaw 프로그램을 Motif로 컨버팅. 각 컴포넌트 크기, 배치, 이벤트 콜백 연결에 특히 신경. 작업 단위 매우 세분화. ~/src/xcalc의 xcalc→xmcalc 컨버전 사례 참고. Xft 지원 필수 — 맨페이지 본문은 monospace(D2Coding), UI 크롬은 NanumMyeongjo. openmotif 자체 Xft 구현은 불안정하므로 직접 컨트롤.

### Interview Summary
**Key Discussions**:
- ScrollByLine widget: XmDrawingArea 기반 재작성 with direct Xft rendering
- Xft scope: Hybrid — man page content uses direct Xft, UI chrome uses XmFONT_IS_XFT
- Menu system: XmMenuBar + XmCascadeButton (standard Motif menu bar)
- Search dialog: Custom XmForm + XmTextField + XmPushButton
- Test strategy: Build + manual QA scenarios
- Font fallback (man page body): D2Coding → Noto Sans Mono CJK KR → DejaVu Sans Mono → monospace → fixed
- Font fallback (UI chrome): NanumMyeongjo → Noto Serif CJK KR → DejaVu Serif → serif → fixed
- Paned widget: XmPanedWindow with approximate parity
- Resource file: Full rewrite for Motif

**Research Findings**:
- xcalc conversion: All XmForm attachments, button labels, and translations MUST be set programmatically
- xcalc conversion: Arm()/Disarm() required in Motif button translations
- xcalc conversion: Xaw Dialog has no Motif equivalent — needs full restructure
- xcalc conversion: No Xft in reference project — Xft is new work
- Metis identified 12 edge cases including XftDraw visual/colormap, anti-aliased ghosting, scrollbar thumb sizing, font fallback

### Metis Review
**Identified Gaps** (all addressed):
- Xft scope ambiguity → resolved: hybrid approach
- ScrollByLine scrollbar strategy → resolved: embedded XmScrollBar
- Search dialog approach → resolved: custom XmForm
- Font fallback chain → resolved: separate monospace (D2Coding) and proportional (NanumMyeongjo) fallback chains
- Internationalization scope → resolved: preserve existing nroff parser only
- Paned behavior → resolved: approximate parity acceptable
- XftDraw visual/colormap → guardrail: use widget's actual visual/colormap
- Anti-aliased ghosting → guardrail: clear-before-redraw mandatory
- Memory management → guardrail: explicit XftColorFree/XftFontClose/XftDrawDestroy
- Build dependency detection → guardrail: try xm, motif, openmotif

---

## Work Objectives

### Core Objective
Convert xman from X11/Xaw to OpenMotif with direct Xft font rendering for man page display, preserving all existing functionality while matching Motif UI conventions. Man page body uses monospace font D2Coding; UI chrome uses proportional font NanumMyeongjo.

### Concrete Deliverables
- Working `xmman` binary linked against `-lXm -lXt -lX11 -lXft -lfontconfig`
- No Xaw references remaining in source code
- Xft rendering of man page content with D2Coding monospace default (body) and NanumMyeongjo for UI chrome
- XmFONT_IS_XFT rendering for all UI chrome widgets
- Standard Motif menu bar replacing Xaw popup menus
- All 9 option menu entries functional
- Dynamic section menu functional
- Directory listing with XmList
- Search dialog with XmForm + XmTextField
- Help window, Save dialog, Warning dialog all functional
- Build smoke test and headless render sanity test

### Definition of Done
- [ ] `./configure && make` produces `xmman` binary with zero warnings
- [ ] `ldd xmman | grep -c Xaw` returns 0 (no Xaw dependency)
- [ ] `ldd xmman | grep -E 'Xm|Xft|fontconfig'` shows all three linked
- [ ] `grep -r '<X11/Xaw/' src/` returns empty (no Xaw includes)
- [ ] xmman opens and displays a man page with Xft-rendered text
- [ ] All menu items (9 options + dynamic sections) work
- [ ] Search dialog opens, accepts input, returns results
- [ ] Font fallback works when D2Coding or NanumMyeongjo is unavailable

### Must Have
- Complete Xaw → Xm widget conversion for all widgets
- Direct Xft rendering in ScrollByLine replacement
- XmFONT_IS_XFT for UI chrome widgets
- D2Coding monospace as default man page body font with fallback chain
- NanumMyeongjo as default UI chrome font with fallback chain
- All existing xman functionality preserved
- Programmatic XmForm attachments (no resource-file widget names)
- Arm()/Disarm() in all button translations
- XmStringCreateLocalized() for all programmatic label changes

### Must NOT Have (Guardrails)
- NO Xaw include headers remaining in source
- NO Xaw library dependency
- NO resource-file widget-name references for XmForm attachments
- NO behavior changes to man.c, vendor.c, vendor.h
- NO new features beyond existing xman functionality
- NO UTF-8 nroff parser (preserve existing ASCII backspace parser only)
- NO XmLabel/XmPushButton subclassing for Xft (use XmFONT_IS_XFT for chrome)
- NO XmFONT_IS_XFT for the man page display area (use direct Xft with D2Coding monospace there)
- NO DefaultVisual/DefaultColormap for XftDrawCreate (use widget's actual visual)
- NO uncleared background before anti-aliased text redraw
- NO leaked Xft resources (XftFont, XftDraw, XftColor must be freed)

---

## Verification Strategy

> **ZERO HUMAN INTERVENTION** - ALL verification is agent-executed. No exceptions.

### Test Decision
- **Infrastructure exists**: NO (no existing test framework)
- **Automated tests**: YES (build smoke test + headless render sanity test)
- **Framework**: Custom shell scripts using xvfb-run + xdotool + xwd
- **Agent-Executed QA**: ALWAYS (mandatory for all tasks)

### QA Policy
Every task MUST include agent-executed QA scenarios.
Evidence saved to `.sisyphus/evidence/task-{N}-{scenario-slug}.{ext}`.

- **Build**: Bash (`make`, `ldd`, `grep`)
- **Render**: Bash with xvfb-run (Xvfb + xwd + file comparison)
- **UI**: Bash with xdotool (key/mouse simulation + xwd screenshot)
- **API/Function**: Bash with curl-equivalent for man page rendering

---

## Execution Strategy

### Parallel Execution Waves

```
Wave 1 (Foundation - build, headers, Xft utility):
├── Task 1: Update build system for Motif + Xft [quick]
├── Task 2: Replace Xaw includes with Motif includes in headers [quick]
├── Task 3: Create Xft font utility subsystem (xft_utils.c/h) [deep]
└── Task 4: Create font configuration and fallback system [unspecified-high]

Wave 2 (Core Widget - ScrollByLine replacement):
├── Task 5: Create ScrollMotive data structures and header (scroll_motive.h) [quick]
├── Task 6: Implement ScrollMotive Xft text renderer [deep]
├── Task 7: Implement ScrollMotive scrollbar integration [unspecified-high]
├── Task 8: Implement ScrollMotive nroff parser (bold/italic/symbol) [deep]
├── Task 9: Implement ScrollMotive file loading and line management [unspecified-high]
└── Task 10: Integrate ScrollMotive with man page display (OpenFile, expose) [deep]

Wave 3 (UI Conversion - dialogs and menus):
├── Task 11: Convert TopBox (form/label/commands) to XmForm/XmLabel/XmPushButton [unspecified-high]
├── Task 12: Convert option menu to XmMenuBar + XmCascadeButton [unspecified-high]
├── Task 13: Convert section menu to XmPulldownMenu [unspecified-high]
├── Task 14: Convert manpage widget (paned/label) to XmPanedWindow [unspecified-high]
├── Task 15: Convert directory list (viewport+list) to XmScrolledWindow+XmList [unspecified-high]
├── Task 16: Convert search dialog (XawDialog) to custom XmForm dialog [deep]
├── Task 17: Convert save/standby/warning dialogs to XmDialogShell + XmMessageBox [quick]
└── Task 18: Convert help window to Motif [unspecified-high]

Wave 4 (Handler/callback conversion + resources):
├── Task 19: Convert handler.c callbacks and action routines for Motif [deep]
├── Task 20: Convert misc.c utilities (ChangeLabel, PopupWarning, etc.) for XmString [unspecified-high]
├── Task 21: Convert search.c DoSearch/MakeSearchWidget for Motif [unspecified-high]
├── Task 22: Convert tkfuncs.c for Motif widget access [quick]
├── Task 23: Rewrite app-defaults/Xmman for Motif resources [quick]
└── Task 24: Convert main.c resource definitions and initialization [unspecified-high]

Wave 5 (Integration and polish):
├── Task 25: Wire all callbacks and verify menu/directory/search functionality [deep]
├── Task 26: Add keyboard translations for Motif action system [unspecified-high]
├── Task 27: Implement ICCCM delete window and cursor management for Motif [quick]
└── Task 28: Final integration build and link verification [quick]

Wave FINAL (After ALL tasks — 4 parallel reviews):
├── Task F1: Plan compliance audit (oracle)
├── Task F2: Code quality review (unspecified-high)
├── Task F3: Real manual QA (unspecified-high)
└── Task F4: Scope fidelity check (deep)
-> Present results -> Get explicit user okay

Critical Path: Task 1-2 → Task 3-4 → Task 5-10 → Task 11-18 → Task 19-24 → Task 25-28 → F1-F4
Parallel Speedup: ~60% faster than sequential
Max Concurrent: 4 (Wave 1)
```

### Dependency Matrix

| Task | Depends On | Blocks |
|------|-----------|--------|
| 1 | - | 2, 3, 4, all subsequent |
| 2 | 1 | 11-24 |
| 3 | 1 | 6, 7 |
| 4 | 1, 3 | 6, 7 |
| 5 | 2 | 6, 7, 8, 9, 10 |
| 6 | 3, 4, 5 | 10 |
| 7 | 3, 4, 5 | 10 |
| 8 | 5 | 10 |
| 9 | 5 | 10 |
| 10 | 6, 7, 8, 9 | 25 |
| 11 | 2 | 25 |
| 12 | 2 | 25 |
| 13 | 2 | 25 |
| 14 | 2 | 25 |
| 15 | 2 | 25 |
| 16 | 2 | 25 |
| 17 | 2 | 25 |
| 18 | 2 | 25 |
| 19 | 11-18 | 25 |
| 20 | 11-18 | 25 |
| 21 | 16 | 25 |
| 22 | 2 | 25 |
| 23 | 19-22 | - |
| 24 | 2 | 25 |
| 25 | 10, 11-18, 19-21, 24 | 28 |
| 26 | 25 | 28 |
| 27 | 25 | 28 |
| 28 | 25, 26, 27 | F1-F4 |

### Agent Dispatch Summary

- **Wave 1**: 4 tasks — T1 `quick`, T2 `quick`, T3 `deep`, T4 `unspecified-high`
- **Wave 2**: 6 tasks — T5 `quick`, T6 `deep`, T7 `unspecified-high`, T8 `deep`, T9 `unspecified-high`, T10 `deep`
- **Wave 3**: 8 tasks — T11-15 `unspecified-high`, T16 `deep`, T17 `quick`, T18 `unspecified-high`
- **Wave 4**: 6 tasks — T19 `deep`, T20-21 `unspecified-high`, T22 `quick`, T23 `quick`, T24 `unspecified-high`
- **Wave 5**: 4 tasks — T25 `deep`, T26 `unspecified-high`, T27 `quick`, T28 `quick`
- **FINAL**: 4 reviews — F1 `oracle`, F2 `unspecified-high`, F3 `unspecified-high`, F4 `deep`

---

## TODOs

- [x] 1. Update build system for Motif + Xft

  **What to do**:
  - Edit `configure.ac`: Replace `xaw7` dependency with `xm` (try `xm`, then `motif`, then `openmotif`), add `xft` and `fontconfig` via `PKG_CHECK_MODULES`
  - Add `XFT_LIBS` and `XFT_CFLAGS` variables from pkg-config
  - Edit `Makefile.am`: Change `XMAN_LIBS` to include `-lXm -lXt -lX11 -lXft -lfontconfig`, remove `-lXaw7`
  - Add `xft_utils.c`, `xft_utils.h`, `scroll_motive.c`, `scroll_motive.h` to source list
  - Remove `ScrollByL.c`, `ScrollByL.h`, `ScrollByLP.h` from source list
  - Update `AM_CPPFLAGS` to include Xft headers
  - Verify `./autogen.sh && ./configure && make` compiles (with expected errors from unchanged Xaw source — just verify build system changes are correct)

  **Must NOT do**:
  - Do not modify any .c/.h source files yet (only build files)
  - Do not remove ScrollByL files yet (they'll still compile even if unused)
  - Do not change the binary name yet

  **Recommended Agent Profile**:
  - **Category**: `quick`
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Task 2)
  - **Parallel Group**: Wave 1
  - **Blocks**: Tasks 3, 4, and all subsequent
  - **Blocked By**: None

  **References**:
  - `configure.ac` (lines 1-EOF): Current xaw7 dependency and build configuration
  - `Makefile.am` (lines 1-EOF): Current source file list and link flags
  - `~/src/xcalc/configure.ac`: Reference Motif build configuration pattern
  - `~/src/xcalc/Makefile.am`: Reference source file organization

  **Why Each Reference Matters**:
  - `configure.ac`: Must see exact PKG_CHECK_MODULES syntax to replace xaw7 with motif+xft
  - `Makefile.am`: Must see current source list to know what to add/remove
  - xcalc `configure.ac`: Shows how motif dependency is declared (xm package)
  - xcalc `Makefile.am`: Shows working motif+xt link configuration

  **Acceptance Criteria**:
  - [ ] `configure.ac` contains `PKG_CHECK_MODULES([XM], [xm])` or equivalent fallback chain
  - [ ] `configure.ac` contains `PKG_CHECK_MODULES([XFT], [xft2 fontconfig])`
  - [ ] `Makefile.am` lists `xft_utils.c`, `xft_utils.h`, `scroll_motive.c`, `scroll_motive.h` in sources
  - [ ] `Makefile.am` does NOT list `ScrollByL.c` in sources (removed from build)
  - [ ] `autogen.sh && ./configure` runs without errors
  - [ ] `ldd xmman` would show `-lXm -lXft` (after full conversion)

  **QA Scenarios**:

  ```
  Scenario: Build system configuration succeeds
    Tool: Bash
    Preconditions: autotools installed, motif-dev and libxft-dev packages available
    Steps:
      1. Run `./autogen.sh` in project root
      2. Run `./configure`
      3. Check exit code is 0
      4. Run `grep -c 'PKG_CHECK_MODULES.*XM' configure.ac` — expect ≥ 1
      5. Run `grep -c 'PKG_CHECK_MODULES.*XFT' configure.ac` — expect ≥ 1
      6. Run `grep 'xft_utils' Makefile.am | wc -l` — expect ≥ 1
      7. Run `grep 'ScrollByL.c' Makefile.am | wc -l` — expect 0
    Expected Result: configure.ac has XM and XFT pkg-config, Makefile.am has new sources, no ScrollByL.c
    Failure Indicators: configure fails, PKG_CHECK_MODULES not found, old sources still listed
    Evidence: .sisyphus/evidence/task-1-build-config.log
  ```

  **Commit**: YES
  - Message: `build: switch configure.ac/Makefile.am from xaw7 to motif+xft`
  - Files: `configure.ac`, `Makefile.am`
  - Pre-commit: `./autogen.sh && ./configure`

- [x] 2. Replace Xaw includes with Motif includes in headers

  **What to do**:
  - Edit `man.h`: Replace all `#include <X11/Xaw/*.h>` lines with corresponding Motif includes:
    - `Xaw/Cardinals.h` → (remove, define ZERO=0, ONE=1 locally)
    - `Xaw/AsciiText.h` → (remove, not needed)
    - `Xaw/SmeBSB.h` → `Xm/PushB.h`
    - `Xaw/Box.h` → (remove, not needed)
    - `Xaw/Command.h` → `Xm/PushB.h`
    - `Xaw/Dialog.h` → `Xm/SelectioB.h` and `Xm/TextF.h`
    - `Xaw/Label.h` → `Xm/Label.h`
    - `Xaw/List.h` → `Xm/List.h`
    - `Xaw/MenuButton.h` → `Xm/CascadeB.h` and `Xm/RowColumn.h`
    - `Xaw/SimpleMenu.h` → `Xm/RowColumn.h`
    - `Xaw/Paned.h` → `Xm/PanedW.h`
    - `Xaw/Viewport.h` → `Xm/ScrolledW.h`
    - `Xaw/Scrollbar.h` → `Xm/ScrollBar.h`
  - Add `#include <Xm/Xm.h>` as the primary Motif include
  - Add `#include <Xm/Form.h>`, `#include <Xm/Frame.h>`, `#include <Xm/DialogS.h>`, `#include <Xm/MessageB.h>`
  - Add `#include <X11/Xft/Xft.h>` and `#include <X11/Xft/XftCompat.h>` if needed
  - Add forward declarations for `XftFont`, `XftDraw`, `XftColor` in man.h or new header
  - Update `XmanFonts` struct in man.h: Change `XFontStruct *directory` to include Xft font fields
  - Define `ZERO` and `ONE` macros locally since `Xaw/Cardinals.h` is removed
  - Verify all .c files that include "man.h" or "globals.h" still compile (they will have linker errors, but should compile)

  **Must NOT do**:
  - Do not modify .c file logic yet — only header changes
  - Do not remove `ScrollByL.h` include yet (it's still referenced by code)
  - Do not change struct field types that affect memory layout across .c files

  **Recommended Agent Profile**:
  - **Category**: `quick`
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Task 1)
  - **Parallel Group**: Wave 1
  - **Blocks**: Tasks 5-28
  - **Blocked By**: None

  **References**:
  - `man.h` (lines 36-68): Current Xaw include block — every include must be mapped
  - `man.h` (lines 87-99): XmanFonts, XmanCursors structs — directory font field to change
  - `man.h` (lines 101-106): ManPageWidgets struct — widget types will change
  - `~/src/xcalc/xmcalc.h` (lines 44-55): Reference Motif include pattern

  **Why Each Reference Matters**:
  - `man.h` includes: Every Xaw include must have a corresponding Motif replacement
  - XmanFonts: The `XFontStruct *directory` field needs to become Xft-aware
  - ManPageWidgets: Widget types change from Xaw to Motif widget classes
  - xcalc header: Shows which Motif includes are actually needed in practice

  **Acceptance Criteria**:
  - [ ] `grep -c '<X11/Xaw/' man.h` returns 0
  - [ ] `grep -c '<Xm/' man.h` returns ≥ 8
  - [ ] `grep -c '<X11/Xft/' man.h` returns ≥ 1
  - [ ] ZERO and ONE macros defined locally
  - [ ] All .c files that include man.h compile (may have type errors, but no missing-header errors)

  **QA Scenarios**:

  ```
  Scenario: Header compilation succeeds
    Tool: Bash
    Preconditions: Task 1 build system changes are in place
    Steps:
      1. Run `grep -c '<X11/Xaw/' man.h` — expect 0
      2. Run `grep -c '<Xm/' man.h` — expect ≥ 8
      3. Run `grep -c 'ZERO' man.h` — expect ≥ 1 (local definition)
      4. Run `grep -c '<X11/Xft/' man.h` — expect ≥ 1
      5. Run `make 2>&1 | grep -c 'No such file' | head` — expect 0 (no missing headers)
    Expected Result: No Xaw includes, Motif includes present, ZERO/ONE defined, Xft included
    Failure Indicators: Missing header errors, Xaw includes remaining
    Evidence: .sisyphus/evidence/task-2-headers.log
  ```

  **Commit**: YES
  - Message: `headers: replace Xaw includes with Xm/Xft includes in man.h`
  - Files: `man.h`

- [x] 3. Create Xft font utility subsystem (xft_utils.c/h)

  **What to do**:
  - Create `xft_utils.h`: Public API for Xft font management
    - `typedef struct { XftFont *normal; XftFont *bold; XftFont *italic; XftFont *symbol; } XftFontSet;`
    - `typedef struct { XftDraw *draw; XftColor fg_color; XftColor bg_color; Visual *visual; Colormap colormap; } XftRenderingContext;`
    - `XftFontSet *XftLoadFontSet(Display *dpy, const char *normal_pattern, const char *bold_pattern, const char *italic_pattern, const char *symbol_pattern);`
    - `void XftFreeFontSet(XftFontSet *fonts);`
    - `XftRenderingContext *XftCreateContext(Drawable drawable, Visual *visual, Colormap colormap, unsigned long fg_pixel, unsigned long bg_pixel);`
    - `void XftDestroyContext(XftRenderingContext *ctx);`
    - `void XftChangeDrawable(XftRenderingContext *ctx, Drawable new_drawable);`
    - `int XftGetFontHeight(const XftFont *font);`
    - `int XftGetFontWidth(const XftFont *font);`
    - `void XftDrawStringUtf8Wrapped(XftRenderingContext *ctx, XftFont *font, int x, int y, const char *text, int len);`
  - Create `xft_utils.c`: Implementation
    - Font loading with fallback chain: try each pattern, on failure try next in chain
    - Fallback chain (man page body — monospace): D2Coding → Noto Sans Mono CJK KR → DejaVu Sans Mono → monospace → fixed
    - Fallback chain (UI chrome — proportional): NanumMyeongjo → Noto Serif CJK KR → DejaVu Serif → serif → fixed
    - If ALL fonts fail, print error to stderr and exit (or use XLoadQueryFont("fixed") as absolute fallback)
    - XftDraw creation using widget's actual visual/colormap (NOT DefaultVisual/DefaultColormap)
    - Context management: create/destroy/change drawable
    - Font metrics: height, width, ascent, descent helpers
    - Color allocation: XftColorAllocValue from Pixel values
    - All functions check for NULL returns and handle errors gracefully

  **Must NOT do**:
  - Do not create rendering functions for nroff formatting yet (that's Task 8)
  - Do not use DefaultVisual() or DefaultColormap() — always use widget's actual values
  - Do not skip NULL checks on XftFont returns

  **Recommended Agent Profile**:
  - **Category**: `deep`
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 1, 2, 4)
  - **Parallel Group**: Wave 1
  - **Blocks**: Tasks 6, 7
  - **Blocked By**: Task 1 (build system must declare xft_utils.c)

  **References**:
  - `ScrollByLP.h` (lines 60-85): Original ScrollByLine private data showing font/GC structure — understand what fields need Xft equivalents
  - `ScrollByL.h` (lines 44-47): Font name patterns (MANPAGE_NORMAL etc.) — understand the 4-font system
  - `man.h` (lines 87-89): XmanFonts struct with single `XFontStruct *directory` — will need Xft version
  - Xft(3) man page or X11/Xft/Xft.h: API reference for XftFont, XftDraw, XftColor types
  - `~/src/xcalc/xmcalc.c` (lines 462-477): Reference pattern for XLoadQueryFont + XmFontListCreate — understand the font loading pattern

  **Why Each Reference Matters**:
  - ScrollByLP.h: Shows what font fields the original widget stored — must replicate with Xft types
  - ScrollByL.h: Font name patterns tell us what categories of fonts we need (normal/bold/italic/symbol)
  - XmanFonts: Current struct needs Xft equivalent for directory font
  - Xft(3): Core API reference for all Xft functions we'll call
  - xcalc pattern: Shows how font loading works in a Motif context (even though it uses core fonts)

  **Acceptance Criteria**:
  - [ ] `xft_utils.h` declares all functions listed above
  - [ ] `xft_utils.c` implements all functions with NULL checks and error handling
  - [ ] `XftLoadFontSet()` tries D2Coding first for man page body fonts, then fallback chain
  - [ ] `XftLoadFontSet()` tries NanumMyeongjo first for UI chrome font, then fallback chain
  - [ ] `XftCreateContext()` uses caller-provided visual/colormap (not DefaultVisual)
  - [ ] `XftDestroyContext()` frees all Xft resources (XftDrawDestroy, XftColorFree)
  - [ ] `make` compiles xft_utils.o without errors

  **QA Scenarios**:

  ```
  Scenario: Xft utility compilation
    Tool: Bash
    Preconditions: Task 1 build system is in place
    Steps:
      1. Run `make xft_utils.o` — expect success
      2. Run `nm xft_utils.o | grep -c 'XftLoadFontSet'` — expect ≥ 1
      3. Run `nm xft_utils.o | grep -c 'XftCreateContext'` — expect ≥ 1
      4. Run `nm xft_utils.o | grep -c 'XftDestroyContext'` — expect ≥ 1
      5. Run `nm xft_utils.o | grep -c 'DefaultVisual'` — expect 0 (must not use DefaultVisual)
    Expected Result: Object file compiled with all functions, no DefaultVisual usage
    Failure Indicators: Compilation errors, missing symbols, DefaultVisual reference
    Evidence: .sisyphus/evidence/task-3-xft-utils.log

  Scenario: Font fallback chain works
    Tool: Bash (with Xvfb)
    Preconditions: Xvfb available, fontconfig installed
    Steps:
      1. Create minimal test program that calls XftLoadFontSet with D2Coding (man page body) and NanumMyeongjo (UI chrome)
      2. Compile and run under xvfb-run
      3. If D2Coding unavailable, verify it falls back to Noto Sans Mono CJK KR, then DejaVu Sans Mono
      4. If NanumMyeongjo unavailable, verify it falls back to Noto Serif CJK KR, then DejaVu Serif
      5. Verify no segmentation fault on any fallback
    Expected Result: Program loads at least one font from fallback chain without crash
    Failure Indicators: Segfault, NULL dereference, "no fonts available" error
    Evidence: .sisyphus/evidence/task-3-font-fallback.log
  ```

  **Commit**: YES
  - Message: `feat: add Xft font utility subsystem (xft_utils.c/h)`
  - Files: `xft_utils.c`, `xft_utils.h`
  - Pre-commit: `make xft_utils.o`

- [ ] 4. Create font configuration and fallback system

  **What to do**:
  - Create `xman_fonts.h`: Define font configuration constants and fallback chains
    - **Man page body fonts (monospace)**:
      - `#define XMAN_MANPAGE_FONT "D2Coding"`
      - `#define XMAN_MANPAGE_FALLBACK_1 "Noto Sans Mono CJK KR"`
      - `#define XMAN_MANPAGE_FALLBACK_2 "DejaVu Sans Mono"`
      - `#define XMAN_MANPAGE_FALLBACK_3 "monospace"`
      - `#define XMAN_MANPAGE_FALLBACK_4 "fixed"`
    - **UI chrome font (proportional)**:
      - `#define XMAN_UI_FONT "NanumMyeongjo"`
      - `#define XMAN_UI_FALLBACK_1 "Noto Serif CJK KR"`
      - `#define XMAN_UI_FALLBACK_2 "DejaVu Serif"`
      - `#define XMAN_UI_FALLBACK_3 "serif"`
      - `#define XMAN_UI_FALLBACK_4 "fixed"`
    - `#define XMAN_MANPAGE_FONT_SIZE 12`
    - `#define XMAN_DIRECTORY_FONT_SIZE 14`
    - Font pattern builders: `XftFontBuildPattern(const char *family, int size, int weight, int slant)`
    - Update `XmanFonts` struct in man.h: Add `XftFontSet *manpage_fonts` and `XftFont *directory_font` alongside or replacing `XFontStruct *directory`
  - Create `xman_fonts.c`: Implementation
    - `XftFontBuildPattern()`: Constructs FcPattern for fontconfig matching with fallback
    - `XftFontOpenWithFallback()`: Tries each pattern in fallback chain, returns first success or NULL
  - `XmanLoadManpageFonts(Display *dpy)`: Loads all 4 manpage fonts (normal, bold, italic, symbol) with **D2Coding** fallback chain (monospace)
  - `XmanLoadDirectoryFont(Display *dpy)`: Loads directory font with **NanumMyeongjo** fallback chain (proportional)
    - `XmanFreeFonts(XmanFonts *fonts)`: Frees all loaded Xft fonts
  - Add Xft resource fields to `Xman_Resources` in man.h:
    - `char *font_normal_pattern` (resource: "manpageFontNormal")
    - `char *font_bold_pattern` (resource: "manpageFontBold")
    - `char *font_italic_pattern` (resource: "manpageFontItalic")
    - `char *font_symbol_pattern` (resource: "manpageFontSymbol")
    - `char *font_directory_pattern` (resource: "directoryFontNormal")
  - Add XtResource entries in main.c for new font pattern resources

  **Must NOT do**:
  - Do not modify the nroff parser
  - Do not change ScrollByLine rendering yet
  - Do not remove the old `XFontStruct *directory` field yet (keep for transition)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 1-3)
  - **Parallel Group**: Wave 1
  - **Blocks**: Tasks 6, 7
  - **Blocked By**: Task 1 (build system), Task 3 (xft_utils.h)

  **References**:
  - `man.h` (lines 87-89, 178-193): XmanFonts and Xman_Resources structs — understand current font resource system
  - `main.c` (lines 45-78): XtResource array — pattern for adding new resources
  - `defs.h` (line 52): `DIRECTORY_NORMAL` default font pattern — must replace with NanumMyeongjo (proportional, for UI chrome)
  - `ScrollByL.h` (lines 44-47): MANPAGE_NORMAL/BOLD/ITALIC/SYMBOL patterns — must replace with D2Coding-based monospace variants
  - `xft_utils.h`: The API just created in Task 3 — this task uses it

  **Why Each Reference Matters**:
  - XmanFonts/Xman_Resources: Must add Xft fields alongside existing XFontStruct fields
  - main.c XtResource: Pattern for how to declare new resources so they work with XtGetApplicationResources
  - DIRECTORY_NORMAL: Current default font pattern — must replace with NanumMyeongjo (UI chrome, proportional)
  - MANPAGE_* patterns: Must replace with D2Coding-based monospace variants (man page body)
  - MANPAGE_* patterns: The 4 font categories that need Xft equivalents
  - xft_utils.h: The API this code will call

  **Acceptance Criteria**:
  - [ ] `xman_fonts.h` defines all font pattern constants and fallback chain
  - [ ] `xman_fonts.c` implements XftFontBuildPattern, XftFontOpenWithFallback, XmanLoadManpageFonts, XmanFreeFonts
  - [ ] XmanFonts struct in man.h has XftFontSet *manpage_fonts field
  - [ ] Xman_Resources struct in man.h has font pattern string fields
  - [ ] main.c has XtResource entries for all new font pattern resources
  - [ ] `make` compiles xman_fonts.o without errors

  **QA Scenarios**:

  ```
  Scenario: Font configuration compiles
    Tool: Bash
    Preconditions: Tasks 1 and 3 complete
    Steps:
      1. Run `make xman_fonts.o` — expect success
      2. Run `nm xman_fonts.o | grep -c 'XftFontOpenWithFallback'` — expect ≥ 1
      3. Run `nm xman_fonts.o | grep -c 'XmanLoadManpageFonts'` — expect ≥ 1
      4. Run `grep 'D2Coding' xman_fonts.h | wc -l` — expect ≥ 1 (man page body default)
      5. Run `grep 'NanumMyeongjo' xman_fonts.h | wc -l` — expect ≥ 1 (UI chrome default)
      5. Run `grep 'XftFontSet' man.h | wc -l` — expect ≥ 1
    Expected Result: Font config module compiled with all functions, D2Coding man page default and NanumMyeongjo UI default set
    Failure Indicators: Compilation errors, missing symbols, no default font constant
    Evidence: .sisyphus/evidence/task-4-font-config.log
  ```

  **Commit**: YES
  - Message: `feat: add font configuration and fallback system`
  - Files: `xman_fonts.c`, `xman_fonts.h`, `man.h`, `main.c`

- [ ] 5. Create ScrollMotive data structures and header (scroll_motive.h)

  **What to do**:
  - Create `scroll_motive.h`: Public API for the Motif-based man page display widget
    - Widget class declaration: `extern WidgetClass scrollMotiveWidgetClass;`
    - Typedef: `ScrollMotiveWidget` and `ScrollMotiveWidgetClass`
    - New resource names: `XmNmanFontSet` (XftFontSet*), `XmNmanPageFile` (FILE*), `XmNmanIndent` (Dimension), `XmNmanForceVert` (Boolean)
    - Function prototypes:
      - `void ScrollMotiveSetFile(Widget w, FILE *file);`
      - `void ScrollMotiveScrollToLine(Widget w, int line);`
      - `int ScrollMotiveGetTotalLines(Widget w);`
      - `int ScrollMotiveGetVisibleLines(Widget w);`
  - Create `scroll_motiveP.h`: Private header with instance and class records
    - Instance part fields (mirroring ScrollByLP.h structure):
      - `XftFontSet *fonts` — 4 fonts (normal, bold, italic, symbol)
      - `XftRenderingContext render_ctx` — Xft draw/color context
      - `int h_width` — character width cache
      - `int font_height` — line height cache
      - `int line_pointer` — current scroll position
      - `int lines` — total number of lines
      - `char **top_line` — array of line pointers
      - `Dimension indent` — left margin
      - `Boolean force_vert` — always show scrollbar
      - `FILE *file` — current file being displayed
      - `Widget scrollbar` — XmScrollBar child widget
      - `Pixmap back_pixmap` — double-buffer pixmap for smooth scrolling
      - `XftColor fg_color, bg_color` — foreground/background colors
    - Class part: minimal (mumble field like original)
  - This task creates ONLY the header files — no .c implementation yet

  **Must NOT do**:
  - Do not implement any rendering or scrolling logic yet
  - Do not modify any existing files except to add `#include "scroll_motive.h"` where needed

  **Recommended Agent Profile**:
  - **Category**: `quick`
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on Task 2)
  - **Parallel Group**: Wave 2 (start after Wave 1)
  - **Blocks**: Tasks 6, 7, 8, 9, 10
  - **Blocked By**: Task 2

  **References**:
  - `ScrollByL.h` (lines 31-68): Original public header — understand resource names and class declarations
  - `ScrollByLP.h` (lines 46-98): Original private header — understand instance/class record structure
  - `xft_utils.h`: Xft font set and context types
  - `xman_fonts.h`: Font configuration constants

  **Why Each Reference Matters**:
  - ScrollByL.h: Resource names and class declaration pattern to replicate
  - ScrollByLP.h: Instance record fields to replicate with Xft types replacing XFontStruct/GC
  - xft_utils.h: XftFontSet and XftRenderingContext types to use in new fields
  - xman_fonts.h: Font pattern constants for resource defaults

  **Acceptance Criteria**:
  - [ ] `scroll_motive.h` declares `scrollMotiveWidgetClass` and `ScrollMotiveWidget`
  - [ ] `scroll_motive.h` declares `ScrollMotiveSetFile`, `ScrollMotiveScrollToLine`, `ScrollMotiveGetTotalLines`, `ScrollMotiveGetVisibleLines`
  - [ ] `scroll_motiveP.h` has instance record with `XftFontSet *fonts`, `XftRenderingContext render_ctx`, `Widget scrollbar`, `char **top_line`, `int lines`, `int line_pointer`
  - [ ] `scroll_motiveP.h` includes `<Xm/Xm.h>` and `<X11/Xft/Xft.h>`
  - [ ] Headers compile in isolation (can be included without errors)

  **QA Scenarios**:

  ```
  Scenario: Headers compile and declare expected symbols
    Tool: Bash
    Preconditions: Tasks 1-3 complete
    Steps:
      1. Run `gcc -c -I. scroll_motive.h 2>&1 | head -5` — expect no errors
      2. Run `grep -c 'scrollMotiveWidgetClass' scroll_motive.h` — expect 1
      3. Run `grep -c 'XftFontSet' scroll_motiveP.h` — expect ≥ 1
      4. Run `grep -c 'XftRenderingContext' scroll_motiveP.h` — expect ≥ 1
      5. Run `grep -c 'Widget scrollbar' scroll_motiveP.h` — expect 1
    Expected Result: Headers exist, compile, and declare all expected types/functions
    Failure Indicators: Compilation errors, missing declarations
    Evidence: .sisyphus/evidence/task-5-scroll-motive-headers.log
  ```

  **Commit**: YES
  - Message: `feat: add ScrollMotive widget data structures and header`
  - Files: `scroll_motive.h`, `scroll_motiveP.h`

- [ ] 6. Implement ScrollMotive Xft text renderer

  **What to do**:
  - Create `scroll_motive.c`: Begin implementation with Xft rendering core
  - Implement `_ScrollMotiveInitialize()`: Set default resources, create GC, initialize Xft context
  - Implement `_ScrollMotiveRealize()`: Realize widget, create XftDraw using widget's actual visual and colormap (NOT DefaultVisual), allocate back_pixmap for double-buffering
  - Implement `_ScrollMotiveExpose()`: Handle expose events — clear area, redraw visible lines using Xft
  - Implement `_ScrollMotiveResize()`: Handle resize — update back_pixmap dimensions, recalculate visible_lines, redraw
  - Implement `_ScrollMotiveDestroy()`: Free XftDraw, XftColors, XftFontSet, back_pixmap, line pointers
  - Implement `_ScrollMotiveSetValuesHook()`: Handle font changes — reload Xft fonts, recalculate metrics
  - Core rendering function: `_ScrollMotiveDrawLines(Widget w, int from_line, int to_line)`
    - For each line, parse nroff formatting characters (bold/italic markers)
    - Determine which font variant (normal/bold/italic/symbol) to use for each segment
    - All font variants use monospace fonts (D2Coding family) for column alignment
    - Calculate x position: left_margin (indent) + tab expansion (monospace tab stops at 8 columns)
    - Calculate y position: top_margin + line_number * font_height
    - For each text segment: call `XftDrawStringUtf8(ctx, font, color, x, y, text, len)`
    - Clear background before each segment when switching fonts (prevent anti-aliased ghosting)
  - Implement color management:
    - Extract foreground/background pixels from widget resources
    - Create XftColor from pixels using `XftColorAllocValue()`
    - Free colors in Destroy using `XftColorFree()`

  **Must NOT do**:
  - Do not implement nroff parsing yet (Task 8)
  - Do not implement file loading yet (Task 9)
  - Do not implement scrollbar management yet (Task 7)
  - Do NOT use DefaultVisual() or DefaultColormap() for XftDrawCreate

  **Recommended Agent Profile**:
  - **Category**: `deep`
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on Tasks 3, 4, 5)
  - **Parallel Group**: Wave 2
  - **Blocks**: Task 10
  - **Blocked By**: Tasks 3, 4, 5

  **References**:
  - `ScrollByL.c` (lines 100-400): Original Initialize, Realize, Redisplay, Resize, SetValues — understand widget lifecycle
  - `ScrollByL.c` (lines 400-700): Original drawing functions (PrintText, DumpText) — understand nroff rendering approach (but don't copy Xaw GC approach)
  - `ScrollByLP.h` (lines 59-85): Original private fields — map each field to Xft equivalent
  - `xft_utils.h`: XftFontSet, XftRenderingContext APIs
  - `xman_fonts.h`: Font loading and fallback functions

  **Why Each Reference Matters**:
  - ScrollByL.c Initialize/Realize: Must replicate widget lifecycle but with Xft resource creation instead of X GC creation
  - ScrollByL.c PrintText/DumpText: Must understand what formatting features to render (bold via overstrike, italic via underscore, symbol font, tab expansion) but implement with Xft not XDrawString
  - ScrollByLP.h: Must replicate all state fields needed for scrolling and rendering
  - xft_utils.h: APIs for font loading, context creation, font metrics
  - xman_fonts.h: Font fallback chain for rendering

  **Acceptance Criteria**:
  - [ ] `scroll_motive.c` implements Initialize, Realize, Expose, Resize, Destroy, SetValuesHook
  - [ ] `_ScrollMotiveRealize()` uses widget's visual/colormap (not DefaultVisual/DefaultColormap)
  - [ ] `_ScrollMotiveExpose()` clears background before drawing (anti-aliased ghosting prevention)
  - [ ] `_ScrollMotiveDestroy()` calls XftDrawDestroy, XftColorFree, XFreePixmap for back_pixmap
  - [ ] `_ScrollMotiveDrawLines()` uses XftDrawStringUtf8 for text rendering
  - [ ] `make` compiles scroll_motive.o without errors

  **QA Scenarios**:

  ```
  Scenario: ScrollMotive core compiles
    Tool: Bash
    Preconditions: Tasks 1-5 complete
    Steps:
      1. Run `make scroll_motive.o` — expect success
      2. Run `nm scroll_motive.o | grep -c '_ScrollMotiveExpose'` — expect ≥ 1
      3. Run `nm scroll_motive.o | grep -c '_ScrollMotiveRealize'` — expect ≥ 1
      4. Run `nm scroll_motive.o | grep -c 'DefaultVisual'` — expect 0 (must not use DefaultVisual)
      5. Run `nm scroll_motive.o | grep -c 'XftDrawString'` — expect ≥ 1
    Expected Result: Object compiled with all lifecycle functions, no DefaultVisual, uses Xft rendering
    Failure Indicators: Compilation errors, DefaultVisual found, no Xft rendering
    Evidence: .sisyphus/evidence/task-6-scroll-motive-renderer.log
  ```

  **Commit**: YES
  - Message: `feat: implement ScrollMotive Xft text renderer`
  - Files: `scroll_motive.c`

- [ ] 7. Implement ScrollMotive scrollbar integration

  **What to do**:
  - Add scrollbar creation in `_ScrollMotiveInitialize()` or `_ScrollMotiveRealize()`:
    - Create `XmScrollBar` as child of the XmDrawingArea widget
    - Set scrollbar to right side with appropriate width
    - Add callbacks: `XmNvalueChangedCallback` and `XmNdragCallback` → `VerticalJump`, `XmNincrementCallback` and `XmNdecrementCallback` → `VerticalScroll`
  - Implement `VerticalScroll()`: Handle line-by-line scrolling (increment/decrement)
  - Implement `VerticalJump()`: Handle thumb drag (jump to position)
  - Implement scrollbar thumb update: After any scroll or content change, call `XmScrollBarSetValues()` to update thumb size and position
  - Add `_ScrollMotiveResize()` scrollbar update: Recalculate visible_lines, update thumb size
  - Handle `XmNuseRight` equivalent: Position scrollbar on right side using XmForm attachment or geometry management

  **Must NOT do**:
  - Do not implement nroff parsing or file loading yet (Tasks 8, 9)
  - Do not use Xaw Scrollbar widget or `XawScrollbarSetThumb`

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 8, 9)
  - **Parallel Group**: Wave 2
  - **Blocks**: Task 10
  - **Blocked By**: Tasks 3, 4, 5

  **References**:
  - `ScrollByL.c` (lines 350-400): Original `VerticalScroll` and `VerticalJump` callbacks — understand scroll behavior
  - `ScrollByL.c` (lines 200-250): Original scrollbar creation (`XtCreateManagedWidget("scrollbar", scrollbarWidgetClass, ...)`) — understand scrollbar parentage and geometry
  - `ScrollByL.c` (lines 600-700): Original `Layout()` function — understand how scrollbar width affects text offset
  - `~/src/xcalc/xmcalc.c` (lines 525-565): Reference for XmForm attachment patterns

  **Why Each Reference Matters**:
  - VerticalScroll/VerticalJump: Must replicate exact scroll behavior but with XmScrollBar callbacks
  - Original scrollbar creation: Understand how scrollbar was embedded as child widget
  - Layout function: Understand how text rendering offset accounts for scrollbar width
  - xcalc XmForm: Shows how to position child widgets in Motif

  **Acceptance Criteria**:
  - [ ] `VerticalScroll()` and `VerticalJump()` callback functions implemented
  - [ ] XmScrollBar created as child widget with proper resources
  - [ ] Scrollbar position updates via `XmScrollBarSetValues()`
  - [ ] Text rendering offset accounts for scrollbar width
  - [ ] `make` compiles without errors

  **QA Scenarios**:

  ```
  Scenario: Scrollbar integration compiles
    Tool: Bash
    Steps:
      1. Run `make scroll_motive.o` — expect success
      2. Run `nm scroll_motive.o | grep -c 'VerticalScroll'` — expect ≥ 1
      3. Run `nm scroll_motive.o | grep -c 'VerticalJump'` — expect ≥ 1
      4. Run `nm scroll_motive.o | grep -c 'XmScrollBarSetValues'` — expect ≥ 1
      5. Run `nm scroll_motive.o | grep -c 'scrollbarWidgetClass'` — expect 0 (no Xaw scrollbar)
    Expected Result: Scrollbar functions present, uses XmScrollBar, no Xaw scrollbar references
    Evidence: .sisyphus/evidence/task-7-scrollbar.log
  ```

  **Commit**: YES
  - Message: `feat: implement ScrollMotive scrollbar integration`
  - Files: `scroll_motive.c`

- [ ] 8. Implement ScrollMotive nroff parser (bold/italic/symbol)

  **What to do**:
  - Port nroff formatting parser from `ScrollByL.c` `PrintText()`/`DumpText()` to `scroll_motive.c`
  - Create `_ScrollMotiveParseLine()`: Parse a single line of nroff output
    - Detect bold: character + backspace + same character pattern (`c\bc`)
    - Detect italic: underscore + backspace + character pattern (`_\bc`)
    - Detect symbol: lines starting with specific symbol markers
    - Detect tab characters: expand to next tab stop (8-column, monospace width)
    - All font variants are monospace (D2Coding family) — tab stops and column alignment are preserved
    - Detect overstrike: accumulate characters at same position
    - Return array of segments: `{font_variant, text, x_position}`
  - Create `_ScrollMotiveRenderSegment()`: Render a single text segment with appropriate Xft font
  - The parser must handle:
    - Empty lines (blank line = paragraph separator)
    - Lines with only whitespace
    - Lines with mixed bold/italic/normal text
    - Tab characters (expand to spaces at 8-column boundaries)
    - Backspace characters for overstriking
  - Maintain font height calculation: use Xft font metrics (ascent + descent) instead of XFontStruct->ascent/descent

  **Must NOT do**:
  - Do not add UTF-8 support (preserve ASCII-only behavior)
  - Do not change nroff parsing logic — port existing behavior exactly
  - Do not implement file loading yet (Task 9)

  **Recommended Agent Profile**:
  - **Category**: `deep`
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 7, 9)
  - **Parallel Group**: Wave 2
  - **Blocks**: Task 10
  - **Blocked By**: Task 5

  **References**:
  - `ScrollByL.c` (lines 450-700): Original `PrintText()` and `DumpText()` functions — THE nroff parser implementation to port
  - `ScrollByL.c` (lines 750-950): Original `_ScrollByLineParseNroff()` or equivalent line-by-line parser
  - `ScrollByL.h` (lines 44-47): Font name patterns showing the 4 font categories
  - `xft_utils.h`: `XftGetFontHeight()`, `XftGetFontWidth()` for font metrics

  **Why Each Reference Matters**:
  - PrintText/DumpText: The actual nroff formatting logic — must understand every overstrike pattern
  - Line parser: How lines are stored and iterated
  - Font categories: Which font variant (normal/bold/italic/symbol) maps to which formatting
  - xft_utils: New font metric functions replacing XTextWidth/XFontStruct metrics

  **Acceptance Criteria**:
  - [ ] `_ScrollMotiveParseLine()` correctly identifies bold (c\bc), italic (_\bc), and normal segments
  - [ ] `_ScrollMotiveParseLine()` expands tabs to spaces at 8-column boundaries
  - [ ] `_ScrollMotiveRenderSegment()` uses correct Xft font variant per segment type
  - [ ] Font height uses Xft font metrics (not XFontStruct)
  - [ ] `make` compiles without errors

  **QA Scenarios**:

  ```
  Scenario: Nroff parser handles formatting
    Tool: Bash
    Steps:
      1. Run `make scroll_motive.o` — expect success
      2. Run `nm scroll_motive.o | grep -c '_ScrollMotiveParseLine'` — expect ≥ 1
      3. Run `nm scroll_motive.o | grep -c '_ScrollMotiveRenderSegment'` — expect ≥ 1
      4. Verify the parser code handles: blank lines, bold overstrike (c\bc), italic (_\bc), tabs, mixed formatting
    Expected Result: Parser and renderer functions present and compile
    Evidence: .sisyphus/evidence/task-8-nroff-parser.log
  ```

  **Commit**: YES
  - Message: `feat: implement ScrollMotive nroff parser (bold/italic/symbol)`
  - Files: `scroll_motive.c`

- [ ] 9. Implement ScrollMotive file loading and line management

  **What to do**:
  - Port file loading logic from `ScrollByL.c` to `scroll_motive.c`
  - Implement `_ScrollMotiveLoadFile()`: Read entire file into memory, build line pointer array
    - Read file using fgets() line by line
    - Allocate and grow `char **top_line` array
    - Count total lines for scrollbar thumb sizing
    - Handle NULL file (empty display)
    - Handle very long lines (no line length limit — allocate dynamically)
  - Implement `_ScrollMotiveFreeLines()`: Free all line pointers and the top_line array
  - Implement `ScrollMotiveSetFile()`: Public API to load a new file
    - Call `_ScrollMotiveFreeLines()` for current content
    - Call `_ScrollMotiveLoadFile()` with new FILE*
    - Update scrollbar range (XmScrollBarSetValues)
    - Reset line_pointer to 0
    - Trigger full redraw
  - Implement `ScrollMotiveGetTotalLines()` and `ScrollMotiveGetVisibleLines()`: Public APIs returning line counts

  **Must NOT do**:
  - Do not modify how `OpenFile()` in `misc.c` works
  - Do not change the FILE* management pattern (man_globals->curr_file stays the same)
  - Do not implement the XtNfile resource yet (Task 10 will wire it)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 7, 8)
  - **Parallel Group**: Wave 2
  - **Blocks**: Task 10
  - **Blocked By**: Task 5

  **References**:
  - `ScrollByL.c` (lines 250-350): Original file loading (`_ScrollByLineLoadFile`) and line array building
  - `ScrollByL.c` (lines 150-200): Original Initialize setting default dimensions
  - `misc.c` (lines 143-159): `OpenFile()` function that calls XtSetValues with XtNfile — understand how files are loaded

  **Why Each Reference Matters**:
  - Original file loading: Must replicate dynamic line array allocation
  - Initialize defaults: Width/height resource defaults
  - OpenFile: The external API that triggers file loading via XtSetValues

  **Acceptance Criteria**:
  - [ ] `_ScrollMotiveLoadFile()` reads file and builds line pointer array
  - [ ] `_ScrollMotiveFreeLines()` frees all allocated memory
  - [ ] `ScrollMotiveSetFile()` replaces current content with new file
  - [ ] `ScrollMotiveGetTotalLines()` and `ScrollMotiveGetVisibleLines()` return correct counts
  - [ ] `make` compiles without errors

  **QA Scenarios**:

  ```
  Scenario: File loading functions compile
    Tool: Bash
    Steps:
      1. Run `make scroll_motive.o` — expect success
      2. Run `nm scroll_motive.o | grep -c 'ScrollMotiveSetFile'` — expect ≥ 1
      3. Run `nm scroll_motive.o | grep -c '_ScrollMotiveLoadFile'` — expect ≥ 1
      4. Run `nm scroll_motive.o | grep -c '_ScrollMotiveFreeLines'` — expect ≥ 1
      5. Run `nm scroll_motive.o | grep -c 'ScrollMotiveGetTotalLines'` — expect ≥ 1
    Expected Result: File loading functions compiled and exported
    Evidence: .sisyphus/evidence/task-9-file-loading.log
  ```

  **Commit**: YES
  - Message: `feat: implement ScrollMotive file loading and line management`
  - Files: `scroll_motive.c`

- [ ] 10. Integrate ScrollMotive with man page display (OpenFile, expose)

  **What to do**:
  - Wire `ScrollMotiveSetFile()` into the man page display pipeline
  - Replace `OpenFile()` in `misc.c` to use ScrollMotive instead of ScrollByLine:
    - Old: `XtSetValues(man_globals->manpagewidgets.manpage, XtNfile, ...)` → New: `ScrollMotiveSetFile(man_globals->manpagewidgets.manpage, file)`
  - Replace `manpagewidgets.manpage` widget creation in `buttons.c`:
    - Old: `mpw->manpage = XtCreateWidget(MANUALPAGE, scrollByLineWidgetClass, pane, NULL, ZERO)`
    - New: `mpw->manpage = XtCreateWidget(MANUALPAGE, scrollMotiveWidgetClass, pane, NULL, ZERO)`
  - Add XtNresource for font set: Pass XftFontSet via `XmNmanFontSet` resource
  - Add XtNresource for indent: Pass indent value via `XmNmanIndent` resource
  - Ensure expose events trigger proper Xft redraw:
    - `_ScrollMotiveExpose()` must call `_ScrollMotiveDrawLines()` for damaged region
    - Double-buffering: Draw to back_pixmap, then XCopyArea to widget window
  - Handle window destruction: `_ScrollMotiveDestroy()` must free back_pixmap
  - Add `XmNuseRight` equivalent if scrollbar should be on right side
  - Verify that `man_globals->manpagewidgets.manpage` widget pointer works with both ScrollByLine (old) and ScrollMotive (new) — ensure all code referencing this widget still works

  **Must NOT do**:
  - Do not remove ScrollByL.c/.h files from the source tree yet (just remove from Makefile.am — done in Task 1)
  - Do not change the ManPageWidgets struct layout

  **Recommended Agent Profile**:
  - **Category**: `deep`
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on Tasks 6, 7, 8, 9)
  - **Parallel Group**: Wave 2 (last task in wave)
  - **Blocks**: Task 25
  - **Blocked By**: Tasks 6, 7, 8, 9

  **References**:
  - `buttons.c` (lines 308-310): Original `mpw->manpage = XtCreateWidget(MANUALPAGE, scrollByLineWidgetClass, ...)` — exact line to replace
  - `misc.c` (lines 143-159): `OpenFile()` function — where XtSetValues(XtNfile) is called, must replace with ScrollMotiveSetFile
  - `scroll_motive.h`: Public API (ScrollMotiveSetFile, ScrollMotiveScrollToLine, etc.)
  - `ScrollByL.h` (lines 44-57): Resource names to port (XmNmanualFontNormal, etc.)

  **Why Each Reference Matters**:
  - buttons.c: Must replace exact widget class in exact creation context
  - misc.c OpenFile: Must replace exact XtSetValues call with new API
  - scroll_motive.h: Public API to call instead of XtSetValues
  - ScrollByL.h resource names: Must map XtN resources to XmN resources

  **Acceptance Criteria**:
  - [ ] `buttons.c` creates `scrollMotiveWidgetClass` instead of `scrollByLineWidgetClass`
  - [ ] `misc.c` `OpenFile()` calls `ScrollMotiveSetFile()` instead of `XtSetValues(XtNfile)`
  - [ ] Expose events trigger Xft redraw via `_ScrollMotiveDrawLines()`
  - [ ] Double-buffering: back_pixmap used, XCopyArea copies to window
  - [ ] `make` links successfully (may still have Xaw-related errors in other files)
  - [ ] No reference to `scrollByLineWidgetClass` in buttons.c

  **QA Scenarios**:

  ```
  Scenario: ScrollMotive integration compiles
    Tool: Bash
    Steps:
      1. Run `make 2>&1 | grep -c 'scrollByLineWidgetClass'` — expect 0 in buttons.c
      2. Run `make 2>&1 | grep -c 'ScrollMotiveSetFile'` — expect ≥ 1
      3. Run `grep -c 'scrollMotiveWidgetClass' buttons.c` — expect ≥ 1
      4. Run `grep -c 'scrollByLineWidgetClass' buttons.c` — expect 0
    Expected Result: buttons.c uses ScrollMotive, misc.c calls ScrollMotiveSetFile
    Failure Indicators: scrollByLineWidgetClass still referenced, compilation errors
    Evidence: .sisyphus/evidence/task-10-integration.log
  ```

  **Commit**: YES
  - Message: `feat: integrate ScrollMotive with man page display`
  - Files: `buttons.c`, `misc.c`, `scroll_motive.c`

- [ ] 11. Convert TopBox (form/label/commands) to XmForm/XmLabel/XmPushButton

  **What to do**:
  - Rewrite `MakeTopBox()` in `buttons.c`:
    - `formWidgetClass` → `xmFormWidgetClass` — set XmNmarginWidth, XmNmarginHeight
    - `labelWidgetClass "topLabel"` → `xmLabelWidgetClass` — use XmStringCreateLocalized for label text, set XmNleftAttachment=XmATTACH_FORM, XmNtopAttachment=XmATTACH_FORM
    - `commandWidgetClass "helpButton"` → `xmPushButtonWidgetClass` — set XmNleftAttachment=XmATTACH_FORM, XmNtopAttachment=XmATTACH_WIDGET, XmNtopWidget=topLabel, add Arm()/Disarm() to translations
    - `commandWidgetClass "quitButton"` → `xmPushButtonWidgetClass` — set XmNleftAttachment=XmATTACH_WIDGET, XmNleftWidget=helpButton, XmNtopAttachment=XmATTACH_WIDGET, XmNtopWidget=topLabel
    - `commandWidgetClass "manpageButton"` → `xmPushButtonWidgetClass` — set XmNleftAttachment=XmATTACH_FORM, XmNtopAttachment=XmATTACH_WIDGET, XmNtopWidget=helpButton
  - Replace `FormUpWidgets()` with programmatic XmForm attachments (delete the function, replace with individual XtSetArg/XtSetValues calls)
  - Convert all `XtNlabel` string resources to `XmNlabelString` with `XmStringCreateLocalized()`
  - Convert all `XtNfromHoriz`/`XtNfromVert` to `XmNleftAttachment`/`XmNtopAttachment` with `XmNleftWidget`/`XmNtopWidget`
  - Set `XmNlabelString` programmatically for all buttons (Motif ignores resource-file labels)
  - Add translations programmatically for helpButton, quitButton, manpageButton with `XtOverrideTranslations()`
  - Icon pixmap: Use `XCreateBitmapFromData()` programmatically (not resource file)

  **Must NOT do**:
  - Do not change callback signatures — `Quit()`, `PopupHelp()`, `CreateNewManpage()` stay as XtActionProc
  - Do not change `FormUpWidgets()` signature yet (it will be removed entirely)
  - Do not use resource-file widget names for XmForm attachments

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 12-18)
  - **Parallel Group**: Wave 3
  - **Blocks**: Task 25
  - **Blocked By**: Task 2

  **References**:
  - `buttons.c` (lines 58-140): `MakeTopBox()` — the entire function to rewrite
  - `buttons.c` (lines 642-715): `FormUpWidgets()` — to be replaced with programmatic attachments
  - `buttons.c` (lines 724-750): `ConvertNamesToWidgets()` — to be removed
  - `~/src/xcalc/xmcalc.c` (lines 445-514): Reference for XmForm attachment pattern, XmPushButton creation, XmStringCreateLocalized

  **Why Each Reference Matters**:
  - MakeTopBox: The exact function being rewritten
  - FormUpWidgets: Must understand what layout logic to replicate with XmForm attachments
  - ConvertNamesToWidgets: Must understand it's being removed (Motif uses direct widget references)
  - xcalc: Shows the programmatic XmForm attachment pattern that works

  **Acceptance Criteria**:
  - [ ] `MakeTopBox()` creates `xmFormWidgetClass`, `xmLabelWidgetClass`, `xmPushButtonWidgetClass`
  - [ ] All buttons have `XmNlabelString` set programmatically via `XmStringCreateLocalized()`
  - [ ] All buttons have translations with `Arm()`/`Disarm()`
  - [ ] All XmForm attachments set programmatically (no XtNfromHoriz/XtNfromVert)
  - [ ] `FormUpWidgets()` function removed or no longer called
  - [ ] Icon pixmap set via `XCreateBitmapFromData()` (not resource string)
  - [ ] `make` compiles without errors

  **QA Scenarios**:

  ```
  Scenario: TopBox creates Motif widgets
    Tool: Bash
    Steps:
      1. Run `make buttons.o 2>&1 | grep -c 'xmFormWidgetClass'` — check for Motif widget usage
      2. Run `grep -c 'commandWidgetClass' buttons.c` — expect 0
      3. Run `grep -c 'XmStringCreateLocalized' buttons.c` — expect ≥ 3 (3 buttons)
      4. Run `grep -c 'XmNleftAttachment' buttons.c` — expect ≥ 3 (form attachments)
      5. Run `grep -c 'Arm()' buttons.c` — expect ≥ 3 (button translations)
    Expected Result: TopBox uses Motif widgets, programmatic labels, form attachments, Arm/Disarm
    Failure Indicators: Xaw widget classes remaining, missing XmString calls
    Evidence: .sisyphus/evidence/task-11-topbox.log
  ```

  **Commit**: YES
  - Message: `ui: convert TopBox to XmForm/XmLabel/XmPushButton`
  - Files: `buttons.c`

- [ ] 12. Convert option menu to XmMenuBar + XmCascadeButton

  **What to do**:
  - Rewrite `CreateOptionMenu()` in `buttons.c`:
    - Replace `menuButtonWidgetClass "options"` with `XmCascadeButton` inside an `XmRowColumn` menu bar
    - Replace `simpleMenuWidgetClass "optionMenu"` with `XmCreatePulldownMenu()`
    - Replace 9 `smeBSBObjectClass` entries with `xmPushButtonWidgetClass` entries inside the pulldown menu
    - Each menu entry gets `XmNactivateCallback` → `OptionCallback` with `man_globals` as client data
    - Store menu entry widget IDs in `man_globals->dir_entry`, `man_globals->manpage_entry`, etc.
  - Replace `XtNmenuName` resource with `XmNsubMenuId` connecting cascade button to pulldown menu
  - Convert menu entry labels to `XmNlabelString` with `XmStringCreateLocalized()`
  - Set `XmNmnemonic` for keyboard shortcuts on menu items
  - Handle menu destroy callback for `MenuStruct` data (from section menu — will be in Task 13)

  **Must NOT do**:
  - Do not change `OptionCallback()` signature yet (Task 19)
  - Do not modify `handler.c` yet

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 11, 13-18)
  - **Parallel Group**: Wave 3
  - **Blocks**: Task 25
  - **Blocked By**: Task 2

  **References**:
  - `buttons.c` (lines 428-488): `CreateOptionMenu()` — the function to rewrite
  - `handler.c` (lines 63-93): `OptionCallback()` — callback signature and logic (must preserve man_globals pointer pattern)
  - `~/src/xcalc/xmcalc.c`: Reference for Motif menu creation pattern (though xcalc uses button grid, not menus)

  **Why Each Reference Matters**:
  - CreateOptionMenu: Exact function to rewrite with all 9 menu entries
  - OptionCallback: Must understand how callback data (man_globals) is passed and used
  - xcalc: Shows Motif widget creation patterns (even though xcalc doesn't have menus)

  **Acceptance Criteria**:
  - [ ] `CreateOptionMenu()` creates `XmRowColumn` menu bar and `XmCascadeButton`
  - [ ] Pulldown menu created with `XmCreatePulldownMenu()`
  - [ ] All 9 menu entries are `xmPushButtonWidgetClass` with `XmNactivateCallback`
  - [ ] All menu entry labels set with `XmStringCreateLocalized()`
  - [ ] `man_globals->dir_entry`, `man_globals->manpage_entry`, etc. store widget IDs
  - [ ] No `simpleMenuWidgetClass` or `smeBSBObjectClass` references remain

  **QA Scenarios**:

  ```
  Scenario: Option menu uses Motif widgets
    Tool: Bash
    Steps:
      1. Run `grep -c 'simpleMenuWidgetClass' buttons.c` — expect 0
      2. Run `grep -c 'smeBSBObjectClass' buttons.c` — expect 0
      3. Run `grep -c 'XmCreatePulldownMenu' buttons.c` — expect ≥ 1
      4. Run `grep -c 'XmNactivateCallback' buttons.c` — expect ≥ 9 (one per menu entry)
      5. Run `grep -c 'XmStringCreateLocalized' buttons.c` — expect ≥ 9
    Expected Result: Option menu uses Motif pulldown menu with XmPushButton entries
    Failure Indicators: Xaw menu widgets still present, missing callbacks
    Evidence: .sisyphus/evidence/task-12-option-menu.log
  ```

  **Commit**: YES
  - Message: `ui: convert option menu to XmMenuBar + XmCascadeButton`
  - Files: `buttons.c`

- [ ] 13. Convert section menu to XmPulldownMenu

  **What to do**:
  - Rewrite `CreateSectionMenu()` in `buttons.c`:
    - Replace `simpleMenuWidgetClass "sectionMenu"` with `XmCreatePulldownMenu()`
    - Replace dynamic `smeBSBObjectClass` entries with `xmPushButtonWidgetClass` entries
    - Each section entry gets `XmNactivateCallback` → `DirPopupCallback` with `MenuStruct` as client data
    - Keep `XtAddCallback(XtNdestroyCallback, MenuDestroy)` pattern for memory cleanup
    - Section labels use `XmNlabelString` with `XmStringCreateLocalized(manual[i].blabel)`
    - Connect section cascade button to section pulldown menu via `XmNsubMenuId`
  - Handle the `XtSetSensitive()` calls for section menu entries that were in the old code

  **Must NOT do**:
  - Do not change `DirPopupCallback()` signature yet (Task 19)
  - Do not change `MenuStruct` definition

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 11-12, 14-18)
  - **Parallel Group**: Wave 3
  - **Blocks**: Task 25
  - **Blocked By**: Task 2

  **References**:
  - `buttons.c` (lines 497-527): `CreateSectionMenu()` — the function to rewrite
  - `handler.c` (lines 241-277): `DirPopupCallback()` — callback that receives MenuStruct data
  - `man.h` (lines 108-112): `MenuStruct` definition — must preserve data structure

  **Why Each Reference Matters**:
  - CreateSectionMenu: Exact function with dynamic menu creation loop
  - DirPopupCallback: Must understand how MenuStruct data is retrieved and used
  - MenuStruct: The callback data structure that carries section number and man_globals

  **Acceptance Criteria**:
  - [ ] `CreateSectionMenu()` uses `XmCreatePulldownMenu()`
  - [ ] Section entries are `xmPushButtonWidgetClass` with `XmNactivateCallback`
  - [ ] `MenuStruct` data passed correctly to callbacks
  - [ ] `XtAddCallback(XtNdestroyCallback, MenuDestroy)` still called for memory cleanup
  - [ ] Section labels use `XmStringCreateLocalized(manual[i].blabel)`
  - [ ] No `smeBSBObjectClass` references remain

  **QA Scenarios**:

  ```
  Scenario: Section menu uses Motif pulldown
    Tool: Bash
    Steps:
      1. Run `grep -c 'simpleMenuWidgetClass.*sectionMenu' buttons.c` — expect 0
      2. Run `grep -c 'XmCreatePulldownMenu' buttons.c` — expect ≥ 2 (option + section)
      3. Run `grep -c 'MenuStruct' buttons.c` — expect ≥ 1 (still used)
      4. Run `grep -c 'MenuDestroy' buttons.c` — expect ≥ 1 (cleanup callback)
    Expected Result: Section menu uses Motif pulldown, MenuStruct preserved
    Evidence: .sisyphus/evidence/task-13-section-menu.log
  ```

  **Commit**: YES
  - Message: `ui: convert section menu to XmPulldownMenu`
  - Files: `buttons.c`

- [ ] 14. Convert manpage widget (paned/label) to XmPanedWindow

  **What to do**:
  - Rewrite `CreateManpageWidget()` in `buttons.c`:
    - `panedWidgetClass "vertPane"` → `xmPanedWindowWidgetClass` — set `XmNsashWidth`, `XmNsashHeight`, `XmNseparatorOn`
    - `panedWidgetClass "horizPane"` → `xmPanedWindowWidgetClass` — same resources
    - `menuButtonWidgetClass "options"` → `XmCascadeButton` (already converted in Task 12, but ensure it's in the correct pane)
    - `menuButtonWidgetClass "sections"` → `XmCascadeButton` (already converted in Task 13, but ensure it's in the correct pane)
    - `labelWidgetClass "manualTitle"` → `xmLabelWidgetClass` — use `XmNlabelString` with `XmStringCreateLocalized()`
  - Convert `XtNpreferredPaneSize` to `XmNpaneMinimum`/`XmNpaneMaximum` for directory height
  - Handle the `XtNsensitive` → `XmNsensitive` (same resource name, but verify)
  - Handle `XtSetSensitive()` calls — verify they work with Motif widgets
  - Convert `XtNlabel` resource on `both_screens_entry` to `XmNlabelString`

  **Must NOT do**:
  - Do not modify the directory/manpage content area yet (that's Tasks 15 and 10)
  - Do not change `StartManpage()` logic yet

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 11-13, 15-18)
  - **Parallel Group**: Wave 3
  - **Blocks**: Task 25
  - **Blocked By**: Task 2

  **References**:
  - `buttons.c` (lines 213-311): `CreateManpageWidget()` — the function to rewrite
  - `handler.c` (lines 106-150): `ToggleBothShownState()` — uses `XtNpreferredPaneSize` to set directory height
  - `~/src/xcalc/xmcalc.c` (lines 445-514): Reference for XmForm widget hierarchy

  **Why Each Reference Matters**:
  - CreateManpageWidget: The entire manpage shell creation function
  - ToggleBothShownState: Must understand how pane height is adjusted dynamically
  - xcalc: Shows Motif widget hierarchy pattern

  **Acceptance Criteria**:
  - [ ] `CreateManpageWidget()` creates `xmPanedWindowWidgetClass` for both panes
  - [ ] `manualTitle` label uses `XmNlabelString` with `XmStringCreateLocalized()`
  - [ ] `XtNpreferredPaneSize` replaced with `XmNpaneMinimum`/`XmNpaneMaximum`
  - [ ] `XtNsensitive` calls work correctly with Motif widgets
  - [ ] No `panedWidgetClass` references remain in buttons.c

  **QA Scenarios**:

  ```
  Scenario: Manpage widget uses Motif panes
    Tool: Bash
    Steps:
      1. Run `grep -c 'panedWidgetClass' buttons.c` — expect 0
      2. Run `grep -c 'xmPanedWindowWidgetClass' buttons.c` — expect ≥ 2
      3. Run `grep -c 'XmNpaneMinimum' buttons.c` — expect ≥ 1
      4. Run `grep -c 'XmNlabelString' buttons.c` — expect ≥ 1 (manualTitle)
    Expected Result: Manpage widget uses XmPanedWindow, labels use XmString
    Evidence: .sisyphus/evidence/task-14-manpage-pane.log
  ```

  **Commit**: YES
  - Message: `ui: convert manpage widget to XmPanedWindow`
  - Files: `buttons.c`

- [ ] 15. Convert directory list (viewport+list) to XmScrolledWindow+XmList

  **What to do**:
  - Rewrite `MakeDirectoryBox()` in `buttons.c`:
    - Replace `viewportWidgetClass` → `XmScrolledWindow` with `XmNscrollingPolicy=XmAUTOMATIC`
    - Replace `listWidgetClass` → `xmListWidgetClass`
    - Convert `XtNlist` resource: Replace `char **` array with `XmStringTable` (array of XmString)
    - Convert `XtNfont` resource: Replace `XFontStruct *` with `XmNfontList` using `XmFONT_IS_XFT` or `XmFontListCreate()`
    - Convert `XtNcallback` → `XmNdefaultActionCallback` (or `XmNsingleSelectionCallback` or `XmNbrowseSelectionCallback`)
  - Create `CreateXmStringList()` helper function:
    - Takes `char **entries` and `int count`
    - Returns `XmStringTable` (array of XmString)
    - Must free XmStringTable after XmList creation
  - Rewrite `DirectoryHandler()` callback:
    - Old: `XawListReturnStruct *ret_val = (XawListReturnStruct *) ret_val` → Get selected index
    - New: Extract selected index from `XmListCallbackStruct *cbs = (XmListCallbackStruct *) call_data`
    - Replace `XawListUnhighlight()` → `XmListDeselectAllItems()`
    - Replace `XawListHighlight()` → `XmListSelectPos()`
  - Handle `man_globals->manpagewidgets.directory` and `man_globals->manpagewidgets.box[]`:
    - `directory` becomes the XmScrolledWindow parent
    - `box[section]` becomes the XmList widget inside XmScrolledWindow

  **Must NOT do**:
  - Do not change `DoSearch()` or `BEntrySearch()` logic
  - Do not change `CreateManpageName()` logic

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 11-14, 16-18)
  - **Parallel Group**: Wave 3
  - **Blocks**: Task 25
  - **Blocked By**: Task 2

  **References**:
  - `buttons.c` (lines 562-588): `MakeDirectoryBox()` — the function to rewrite
  - `buttons.c` (lines 535-551): `CreateList()` — creates char** array, must create XmStringTable instead
  - `handler.c` (lines 216-229): `DirectoryHandler()` — callback signature change (XawListReturnStruct → XmListCallbackStruct)
  - `handler.c` (lines 267-268): `XawListUnhighlight()` calls — must become `XmListDeselectAllItems()`
  - `search.c` (line 346): `XawListHighlight()` call — must become `XmListSelectPos()`

  **Why Each Reference Matters**:
  - MakeDirectoryBox: The list creation function to rewrite
  - CreateList: Must understand string array creation to convert to XmStringTable
  - DirectoryHandler: Callback data structure changes entirely
  - XawListUnhighlight/Highlight: Must find ALL calls and replace

  **Acceptance Criteria**:
  - [ ] `MakeDirectoryBox()` creates `XmScrolledWindow` + `xmListWidgetClass`
  - [ ] List items created with `XmStringTable` and `XmStringCreateLocalized()`
  - [ ] `XmNdefaultActionCallback` connected to `DirectoryHandler`
  - [ ] `DirectoryHandler()` uses `XmListCallbackStruct` instead of `XawListReturnStruct`
  - [ ] `XawListUnhighlight()` replaced with `XmListDeselectAllItems()`
  - [ ] `XawListHighlight()` replaced with `XmListSelectPos()`
  - [ ] `XmStringTable` freed after XmList creation

  **QA Scenarios**:

  ```
  Scenario: Directory list uses XmList
    Tool: Bash
    Steps:
      1. Run `grep -c 'viewportWidgetClass' buttons.c` — expect 0
      2. Run `grep -c 'listWidgetClass' buttons.c` — expect 0
      3. Run `grep -c 'xmListWidgetClass' buttons.c` — expect ≥ 1
      4. Run `grep -c 'XmStringTable' buttons.c` — expect ≥ 1
      5. Run `grep -c 'XmNdefaultActionCallback' buttons.c` — expect ≥ 1
      6. Run `grep -c 'XawListReturnStruct' handler.c` — expect 0
    Expected Result: Directory uses XmList, callback data structure converted
    Evidence: .sisyphus/evidence/task-15-directory-list.log
  ```

  **Commit**: YES
  - Message: `ui: convert directory list to XmScrolledWindow + XmList`
  - Files: `buttons.c`, `handler.c`

- [ ] 16. Convert search dialog (XawDialog) to custom XmForm dialog

  **What to do**:
  - Rewrite `MakeSearchWidget()` in `search.c`:
    - Replace `transientShellWidgetClass "search"` → `xmDialogShellWidgetClass` or `transientShellWidgetClass` with `XmCreateFormDialog()`
    - Replace `dialogWidgetClass "dialog"` → `xmFormWidgetClass` as dialog content area
    - Replace `XawDialogGetValueString()` → `XmTextGetString()` on the XmTextField
    - Replace `XtNvalue` (dialog value) → `XmNvalue` on XmTextField
    - Replace `XawDialogAddButton()` calls → create `xmPushButtonWidgetClass` children manually with `XmNleftAttachment`/`XmNtopAttachment`
    - Create XmTextField for search input with `XmNeditMode=XmSINGLE_LINE_EDIT`
    - Set `XmNkeyboardFocusPolicy` to ensure text field gets focus
    - Replace `XtNameToWidget(dialog, "value")` → direct widget pointer (store in man_globals)
  - Store search text widget pointer in `man_globals->text_widget` (already exists)
  - Clear search string: Replace `XtSetArg(XtNvalue, "")` → `XmTextFieldSetString(text_widget, "")`

  **Must NOT do**:
  - Do not change `DoSearch()` search logic
  - Do not change `SearchString()` logic (only how it gets the string)
  - Do not remove `man_globals->text_widget` field

  **Recommended Agent Profile**:
  - **Category**: `deep`
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 11-15, 17-18)
  - **Parallel Group**: Wave 3
  - **Blocks**: Task 21 (search.c conversion)
  - **Blocked By**: Task 2

  **References**:
  - `search.c` (lines 49-105): `MakeSearchWidget()` — the function to rewrite
  - `search.c` (lines 113-125): `SearchString()` — uses `XawDialogGetValueString()` to get search text
  - `handler.c` (lines 466-487): `PopupSearch()` — pops up the search widget
  - `handler.c` (lines 498-502): `RemoveSearch()` — pops down the search widget
  - `handler.c` (lines 560-627): `Search()` — clears search string, calls DoSearch

  **Why Each Reference Matters**:
  - MakeSearchWidget: The entire search dialog creation to rewrite
  - SearchString: Must convert from XawDialogGetValueString to XmTextGetString
  - PopupSearch/RemoveSearch: Simple XtPopup/XtPopdown that should work with Motif dialog shell too
  - Search action: Must find all XawDialogGetValueString and XtNvalue references

  **Acceptance Criteria**:
  - [ ] `MakeSearchWidget()` creates `transientShellWidgetClass` + `xmFormWidgetClass` + `xmTextFieldWidgetClass` + `xmPushButtonWidgetClass` buttons
  - [ ] No `dialogWidgetClass` or `XawDialogAddButton` references remain
  - [ ] `SearchString()` uses `XmTextGetString()` instead of `XawDialogGetValueString()`
  - [ ] Search text field gets keyboard focus on popup
  - [ ] `XmTextFieldSetString()` used to clear search string
  - [ ] `XmStringFree()` called on all XmString allocations

  **QA Scenarios**:

  ```
  Scenario: Search dialog uses Motif widgets
    Tool: Bash
    Steps:
      1. Run `grep -c 'dialogWidgetClass' search.c` — expect 0
      2. Run `grep -c 'XawDialogAddButton' search.c` — expect 0
      3. Run `grep -c 'XawDialogGetValueString' search.c` — expect 0
      4. Run `grep -c 'xmFormWidgetClass\|xmTextFieldWidgetClass' search.c` — expect ≥ 2
      5. Run `grep -c 'XmTextGetString' search.c` — expect ≥ 1
    Expected Result: Search dialog uses XmForm + XmTextField + XmPushButton, no Xaw Dialog
    Evidence: .sisyphus/evidence/task-16-search-dialog.log
  ```

  **Commit**: YES
  - Message: `ui: convert search dialog to custom XmForm dialog`
  - Files: `search.c`

- [ ] 17. Convert save/standby/warning dialogs to XmDialogShell + XmMessageBox

  **What to do**:
  - Rewrite `MakeSaveWidgets()` in `buttons.c`:
    - Replace `transientShellWidgetClass "pleaseStandBy"` → `XmCreateInformationDialog()` or custom dialog with XmLabel
    - Replace `transientShellWidgetClass "likeToSave"` → `XmCreateQuestionDialog()` with Yes/No buttons
    - Replace `dialogWidgetClass "dialog"` → `xmMessageBoxWidgetClass`
    - Replace `XawDialogAddButton()` → `XmNokLabelString`/`XmNcancelLabelString` for button labels
    - Convert `labelWidgetClass "label"` (standby) → `xmLabelWidgetClass` with `XmNlabelString`
  - Rewrite `PopupWarning()` in `misc.c`:
    - Replace `transientShellWidgetClass "warnShell"` → `XmCreateWarningDialog()`
    - Replace `dialogWidgetClass "warnDialog"` → `xmMessageBoxWidgetClass`
    - Replace `XawDialogAddButton("dismiss")` → `XmNokLabelString "Dismiss"` + `XmNokCallback`
  - Save dialog: Handle `SaveFormattedPage()` action:
    - "yes" button → `XmNokCallback` calling save logic
    - "no" button → `XmNcancelCallback` calling cancel logic
  - Standby dialog: Just a label with "Please Stand By" text, popped up/down

  **Must NOT do**:
  - Do not change the actual save/file logic in `SaveFormattedPage()`
  - Do not change the temp file management

  **Recommended Agent Profile**:
  - **Category**: `quick`
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 11-16, 18)
  - **Parallel Group**: Wave 3
  - **Blocks**: Task 25
  - **Blocked By**: Task 2

  **References**:
  - `buttons.c` (lines 598-632): `MakeSaveWidgets()` — the function to rewrite
  - `misc.c` (lines 70-121): `PopupWarning()` — the warning dialog function
  - `handler.c` (lines 296-363): `SaveFormattedPage()` — uses `XtParent(XtParent(w))` for dialog traversal — must update
  - `handler.c` (lines 498-502): `RemoveSearch()` — pops down search widget

  **Why Each Reference Matters**:
  - MakeSaveWidgets: Both save and standby dialogs to rewrite
  - PopupWarning: Warning dialog creation and popup logic
  - SaveFormattedPage: Dialog traversal via XtParent — may need update for Motif dialog shell
  - RemoveSearch: Similar popup/popdown pattern

  **Acceptance Criteria**:
  - [ ] `MakeSaveWidgets()` creates Motif dialog widgets (XmMessageBox or XmLabel)
  - [ ] `PopupWarning()` creates `XmCreateWarningDialog()` or equivalent
  - [ ] No `XawDialogAddButton` references remain in buttons.c or misc.c
  - [ ] No `dialogWidgetClass` references remain
  - [ ] Save dialog has Yes/No buttons with proper callbacks
  - [ ] Standby dialog shows "Please Stand By" text

  **QA Scenarios**:

  ```
  Scenario: Dialog widgets use Motif
    Tool: Bash
    Steps:
      1. Run `grep -c 'dialogWidgetClass' buttons.c misc.c` — expect 0
      2. Run `grep -c 'XawDialogAddButton' buttons.c misc.c` — expect 0
      3. Run `grep -c 'XmCreateWarningDialog\|XmCreateQuestionDialog\|XmCreateInformationDialog' misc.c buttons.c` — expect ≥ 1
      4. Run `grep -c 'xmMessageBoxWidgetClass' buttons.c misc.c` — expect ≥ 1
    Expected Result: All dialogs use Motif message boxes, no Xaw Dialog
    Evidence: .sisyphus/evidence/task-17-dialogs.log
  ```

  **Commit**: YES
  - Message: `ui: convert save/standby/warning dialogs to XmDialogShell + XmMessageBox`
  - Files: `buttons.c`, `misc.c`

- [ ] 18. Convert help window to Motif

  **What to do**:
  - The help window reuses `CreateManpageWidget()` with `full_instance=FALSE`
  - Verify that the Motif-converted `CreateManpageWidget()` works for help mode:
    - `sections` menu button should be insensitive (already handled via `XtSetSensitive`)
    - `dir_entry`, `manpage_entry`, `help_entry`, `search_entry`, `both_screens_entry` should be insensitive
    - Verify `XmNsensitive` works the same as `XtNsensitive`
  - Verify `MakeHelpWidget()` in `help.c` works with Motif widgets:
    - The help widget is created via `CreateManpageWidget(man_globals, HELPNAME, FALSE)`
    - This should work if `CreateManpageWidget` is properly converted
  - Ensure help file opening (`OpenHelpfile()`) uses `ScrollMotiveSetFile()` instead of `XtSetValues(XtNfile)`

  **Must NOT do**:
  - Do not create a separate help widget structure — it reuses the manpage widget
  - Do not change `OpenHelpfile()` logic (just the file loading mechanism)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 11-17)
  - **Parallel Group**: Wave 3
  - **Blocks**: Task 25
  - **Blocked By**: Tasks 2, 10

  **References**:
  - `help.c` (lines 46-87): `MakeHelpWidget()` — creates help via CreateManpageWidget
  - `help.c` (lines 95-108): `OpenHelpfile()` — opens help file, calls OpenFile which uses XtSetValues(XtNfile)
  - `buttons.c` (lines 213-311): `CreateManpageWidget()` — must be already converted for Motif
  - `buttons.c` (lines 270-283): Sensitive widget setting in help mode — verify XmNsensitive works

  **Why Each Reference Matters**:
  - MakeHelpWidget: Simple function, just needs verification that Motif conversion works
  - OpenHelpfile: Uses OpenFile → must use ScrollMotiveSetFile
  - CreateManpageWidget: Must be properly converted for full_instance=FALSE case
  - Sensitive widgets: Must verify XmNsensitive works for help mode

  **Acceptance Criteria**:
  - [ ] `MakeHelpWidget()` works with Motif-converted `CreateManpageWidget()`
  - [ ] `OpenHelpfile()` → `OpenFile()` → `ScrollMotiveSetFile()` chain works
  - [ ] Help widget menus are insensitive when `full_instance=FALSE`
  - [ ] Help window displays help text with Xft rendering

  **QA Scenarios**:

  ```
  Scenario: Help window creates properly
    Tool: Bash
    Steps:
      1. Run `grep -c 'OpenFile' help.c` — expect ≥ 1 (still uses OpenFile)
      2. Run `grep -c 'XtSetValues.*XtNfile' misc.c` — expect 0 (replaced with ScrollMotiveSetFile)
      3. Run `grep -c 'ScrollMotiveSetFile' misc.c` — expect ≥ 1
    Expected Result: Help window uses ScrollMotive for file loading, Motif widgets for display
    Evidence: .sisyphus/evidence/task-18-help-window.log
  ```

  **Commit**: YES
  - Message: `ui: convert help window to Motif`
  - Files: `help.c`, `misc.c`

---

## Final Verification Wave

- [ ] F1. **Plan Compliance Audit** — `oracle`
  Read the plan end-to-end. For each "Must Have": verify implementation exists (read file, check symbol). For each "Must NOT Have": search codebase for forbidden patterns — reject with file:line if found. Check evidence files exist in .sisyphus/evidence/. Compare deliverables against plan.
  Output: `Must Have [N/N] | Must NOT Have [N/N] | Tasks [N/N] | VERDICT: APPROVE/REJECT`

- [ ] F2. **Code Quality Review** — `unspecified-high`
  Run `make` + any lint checks. Review all changed files for: `as any`/type casts, empty catches, printf in prod, commented-out code, unused imports, Xaw include remnants, Xft resource leaks. Check AI slop: excessive comments, over-abstraction, generic names.
  Output: `Build [PASS/FAIL] | Lint [PASS/FAIL] | Files [N clean/N issues] | VERDICT`

- [ ] F3. **Real Manual QA** — `unspecified-high`
  Start from clean build. Execute EVERY QA scenario from EVERY task — follow exact steps, capture evidence. Test cross-task integration (menu + directory + man page display working together). Test edge cases: empty search, invalid section, large man page. Save to `.sisyphus/evidence/final-qa/`.
  Output: `Scenarios [N/N pass] | Integration [N/N] | Edge Cases [N tested] | VERDICT`

- [ ] F4. **Scope Fidelity Check** — `deep`
  For each task: read "What to do", read actual diff. Verify 1:1 — everything in spec was built (no missing), nothing beyond spec was built (no creep). Check "Must NOT do" compliance. Detect cross-task contamination. Flag unaccounted changes.
  Output: `Tasks [N/N compliant] | Contamination [CLEAN/N issues] | Unaccounted [CLEAN/N files] | VERDICT`

---

## Commit Strategy

- **Task 1**: `build: switch configure.ac/Makefile.am from xaw7 to motif+xft`
- **Task 2**: `headers: replace Xaw includes with Xm/Xft includes in man.h`
- **Task 3**: `feat: add Xft font utility subsystem (xft_utils.c/h)`
- **Task 4**: `feat: add font configuration and fallback system`
- **Task 5**: `feat: add ScrollMotive widget data structures and header`
- **Task 6**: `feat: implement ScrollMotive Xft text renderer`
- **Task 7**: `feat: implement ScrollMotive scrollbar integration`
- **Task 8**: `feat: implement ScrollMotive nroff parser (bold/italic/symbol)`
- **Task 9**: `feat: implement ScrollMotive file loading and line management`
- **Task 10**: `feat: integrate ScrollMotive with man page display`
- **Task 11**: `ui: convert TopBox to XmForm/XmLabel/XmPushButton`
- **Task 12**: `ui: convert option menu to XmMenuBar + XmCascadeButton`
- **Task 13**: `ui: convert section menu to XmPulldownMenu`
- **Task 14**: `ui: convert manpage widget to XmPanedWindow`
- **Task 15**: `ui: convert directory list to XmScrolledWindow + XmList`
- **Task 16**: `ui: convert search dialog to custom XmForm dialog`
- **Task 17**: `ui: convert save/standby/warning dialogs to XmDialogShell`
- **Task 18**: `ui: convert help window to Motif`
- **Task 19**: `handler: convert callbacks and action routines for Motif`
- **Task 20**: `misc: convert utilities for XmString`
- **Task 21**: `search: convert DoSearch/MakeSearchWidget for Motif`
- **Task 22**: `misc: convert tkfuncs.c for Motif widget access`
- **Task 23**: `res: rewrite app-defaults/Xmman for Motif resources`
- **Task 24**: `main: convert resource definitions and initialization`
- **Task 25**: `feat: wire all callbacks and verify functionality`
- **Task 26**: `feat: add keyboard translations for Motif action system`
- **Task 27**: `feat: implement ICCCM and cursor management for Motif`
- **Task 28**: `chore: final integration build and link verification`

---

## Success Criteria

### Verification Commands
```bash
# Build succeeds with no Xaw references
./autogen.sh && ./configure && make
# Expected: zero errors, zero warnings

# No Xaw dependency
ldd xmman | grep -i xaw
# Expected: no output (returns 1)

# Motif + Xft linked
ldd xmman | grep -E 'libXm|libXft|libfontconfig'
# Expected: all three found

# No Xaw includes in source
grep -r '<X11/Xaw/' src/
# Expected: no output (returns 1)

# Binary runs and can open display
xvfb-run ./xmman -help
# Expected: prints usage and exits 0
```

### Final Checklist
- [ ] All "Must Have" present
- [ ] All "Must NOT Have" absent
- [ ] Build passes with zero Xaw dependencies
- [ ] Man page renders with Xft (D2Coding monospace visible in body, NanumMyeongjo visible in UI)
- [ ] All 9 option menu entries functional
- [ ] Dynamic section menu entries functional
- [ ] Search dialog works (manual + apropos)
- [ ] Help window displays
- [ ] Save formatted page works
- [ ] Font fallback works when D2Coding or NanumMyeongjo is unavailable