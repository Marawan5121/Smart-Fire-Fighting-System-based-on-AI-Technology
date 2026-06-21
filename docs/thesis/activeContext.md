# Active Context — SFFS Thesis (current working state)

**Phase:** Authoring complete — all 4 chapters written (`chapter1.md`–`chapter4.md`). Ready for Pandoc assembly.
**Source of truth:** Active production codebase (CODE WINS over draft / BOM / verbal inputs).
**Next action:** Optional — assemble the four chapters into one `.docx` via Pandoc (per the recorded docx-js/index standards), and/or supply the 2 pending Ch 1 figures.

---

## Completed this session

- **Both conversions executed** (terminal MCP restored):
  - `pandoc Project_Stage_1.docx -o docs/thesis/term1_draft.md` → 115.8 KB + extracted `docs/thesis/media/`.
  - `pdftotext -layout "Engineering Component Dimensions and Quantities V2.pdf" docs/thesis/component_dimensions_v2.md` → 1.25 KB.
- **Live code re-read from disk** and Memory Bank rewritten to mirror it (`config.h`, `platformio.ini`, Python `config.py`).
- Hardware/software/AI representations corrected to the code: 4-room / 9-servo / 4-pump zonal architecture; YOLOv11 `best_nano_111.pt`; Mosquitto `localhost:1883`; Telegram `sffs_bot`.
- Verified thesis front-matter captured from the draft (institution, authors, supervisor, title).

## Source reconciliation — draft/BOM vs CODE (code wins)

The converted draft (`term1_draft.md`, Jan 2025 first-term) and the BOM PDF describe an earlier design. The following contradictions are resolved in favor of the code and must be applied when writing chapters:

| Earlier source claim | Active code reality (authoritative) |
|----------------------|-------------------------------------|
| YOLOv5, "thermal cameras" | YOLOv11 (`best_nano_111.pt`), standard camera + CUDA FP16 |
| PIR motion sensors | Vision line-crossing occupancy tracker (`OCCUPANCY_LINE_X=300`) |
| Electrical current sensor (ACS712) | Removed — not present |
| MPU6050, SIM800L GSM (in BOM) | Removed — not present |
| 360° single rotating servo + nozzle, 1 pump (BOM/draft) | 9 servos via PCA9685 (valve/4 doors/2 corridors/2 windows) + 4 room pumps |
| BAMB balloon escape system | Not in implemented system |
| PC-based hybrid processing only | ESP32 level-driven controller + local GPU vision over MQTT |

These are recorded so chapter text never reintroduces removed/legacy elements.

## Notes on extracted artifacts

- `component_dimensions_v2.md` has minor encoding artifacts (`×`/`°` rendered as `�`) and lists legacy parts (SIM800L, MPU6050) and single-servo/single-pump counts — use only for physical dimensions of *retained* components; quantities and removed parts are superseded by the code.
- `term1_draft.md` is useful for front-matter, abstract intent, and literature-review references only; all technical system descriptions defer to the code.
- `docs/thesis/media/` holds images Pandoc extracted from the draft (e.g., institution logo) — available for front-matter if needed.

## Chapter file plan (4-chapter mandate)

One file per chapter in `docs/thesis/`, semantic headings + `![Figure X.Y: ...](path)` captions for dynamic TOC/TOF/TOT.

| File | Chapter | Status |
|------|---------|--------|
| `chapter1.md` | Introduction & Project Overview | ✅ Expanded (5 Mermaid figs, 5 tables, formal math, 2 raster placeholders) |
| `chapter2.md` | Hardware Architecture & Component Specifications | ✅ Written (full length; 2 Mermaid figs, 3 tables, 5 prototype photos embedded) |
| `chapter3.md` | Full System Integration & Implementation | ✅ Written (full length; 2 Mermaid figs, 4 tables, 9 build/runtime photos; firmware + Python re-inspected) |
| `chapter4.md` | Empirical Testing, Results, Validation & Conclusion | ✅ Written (full length; 1 Mermaid seq, 6 tables, 5 results figures; conclusion + roadmap) |

**Empirical-data caveat (Ch 4):** no measured-metrics artifacts (`results.csv`, confusion-matrix exports) exist on disk, so the benchmark/confusion-matrix/latency cells are clearly-flagged *representative* figures with the exact procedure to obtain the student's real values (`yolo val`, timestamp differencing). Code-derived constants (2 s / 5 s / 45 s cadences, conf 0.50 / IoU 0.20, FP16) are exact.

Chapter 1 sections: 1.1 Introduction · 1.2 Problem Statement · 1.3 Objectives & Scope · 1.4 Proposed System Overview · 1.5 Thesis Book Organization (outlines the four chapters above).
