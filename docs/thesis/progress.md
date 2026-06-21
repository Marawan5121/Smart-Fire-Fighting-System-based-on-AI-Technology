# Progress — SFFS Thesis

## Setup checklist

| Item | Status |
|------|--------|
| `docs/thesis/` workspace created | ✅ Done |
| Memory Bank (`projectContext`, `activeContext`, `progress`) | ✅ Done |
| Hardware scope refactor recorded | ✅ Done |
| Software/AI stack recorded | ✅ Done |
| Visual-asset register (filesystem scan) | ✅ Done |
| Compilation / index / protection standards recorded | ✅ Done |
| Pandoc `Project_Stage_1.docx` → `term1_draft.md` | ✅ Done (115.8 KB + media/) |
| PDF → `component_dimensions_v2.md` extraction | ✅ Done (1.25 KB, pdftotext) |
| Live code re-read; Memory Bank mirrored to code | ✅ Done |
| Book structure mapped to 4 chapters | ✅ Done |
| Chapter 1 (`chapter1.md`) drafted | ✅ Done — expanded to full length (Mermaid + math + 5 tables) |
| Chapter 2 (`chapter2.md`) drafted | ✅ Done — full length (2 Mermaid, 3 tables, 5 prototype figures embedded) |
| Chapter 3 (`chapter3.md`) drafted | ✅ Done — full length (2 Mermaid, 4 tables, 9 build/runtime figures; code re-inspected) |
| Chapter 4 (`chapter4.md`) drafted | ✅ Done — full length (1 Mermaid seq, 6 tables, 5 results figures; conclusion + roadmap) |
| All 4 chapters written | ✅ Done — ready for Pandoc assembly |
| Pending raster figures for Ch 1 (user to provide) | ⚠️ 2 requested (see below) |

---

## Visual-Asset Register

All paths are absolute on disk. **Caption convention for chapters:** `![Figure X.Y: Caption](relative/path)`.
**Path caveats for Markdown/Pandoc:** filenames containing spaces — `node_red_dashboard (2).png` and `telegram masseg.png` — must be URL-encoded (`%20`) or wrapped in angle brackets `![...](<.../telegram masseg.png>)`. Note `telegram masseg.png` is a **.png** (the earlier reference to `.jpg` does not match disk).

### Curated screenshots — `D:\Graduation Project\cv\photos\`

| File | Subject | Target chapter |
|------|---------|----------------|
| `makit_front_viwe.png` | Physical prototype — front view | Ch 2 (Hardware) — ✅ embedded (Fig 2.3) |
| `makit_back_viwe.png` | Physical prototype — back view | Ch 2 — ✅ embedded (Fig 2.7) |
| `makit_right_viwe.png` | Physical prototype — right view | Ch 2 — ✅ embedded (Fig 2.6) |
| `makit_top_viwe.png` | Physical prototype — top view | Ch 2 — ✅ embedded (Fig 2.5) |
| `makit_cornaer_viwe.png` | Physical prototype — corner view | Ch 2 — ✅ embedded (Fig 2.4) |
| `vs.png` | PlatformIO build in VS Code | Ch 3 — ✅ embedded (Fig 3.3) |
| `vs_termnal_output.png` | Serial device monitor — runtime console | Ch 3 — ✅ embedded (Fig 3.4) |
| `node_red_control.png` | Node-RED control flow (overview) | Ch 3 — ✅ embedded (Fig 3.5) |
| `node_red_control-part1.png` | Node-RED flow — segment 1 | Ch 3 — ✅ embedded (Fig 3.6) |
| `node_red_control-part2.png` | Node-RED flow — segment 2 | Ch 3 — ✅ embedded (Fig 3.7) |
| `node_red_control-part3.png` | Node-RED flow — segment 3 | Ch 3 — ✅ embedded (Fig 3.8) |
| `node_red_dashboard (2).png` | SFFS Control Room dashboard UI | Ch 3 — ✅ embedded (Fig 3.9) |
| `node_red_termnal.png` | Node-RED runtime terminal | Ch 3 — ✅ embedded (Fig 3.10) |
| `running_cv_model.png` | YOLOv11 live detection window | Ch 3 — ✅ embedded (Fig 3.11) |
| `gpu_run_fire_fire_case.png` | GPU inference — FIRE case detection | Ch 4 — ✅ embedded (Fig 4.1) |
| `gpu_run_fire_safe_case.png` | GPU inference — SAFE case | Ch 4 — ✅ embedded (Fig 4.2) |
| `cpu_run_fire_safe_case.png` | CPU inference — SAFE case (CPU vs GPU comparison) | Ch 4 — ✅ embedded (Fig 4.3) |
| `telegram masseg.png` | Automated Telegram alert capture | Ch 4 — ✅ embedded (Fig 4.9) |

### Runtime detection captures — `D:\Graduation Project\cv\Real-Time-Smoke-Fire-Detection-YOLO11\detected_fires\`

Eight automated alert frames (sample evidence for the detection/alert pipeline) — 3 embedded in Ch 4 (Figs 4.6–4.8); full set referenced:

| File |
|------|
| `alert_20260618-212833-993578.jpg` |
| `alert_20260618-212843-997218.jpg` |
| `alert_20260618-213010-984362.jpg` |
| `alert_20260619-001311-916843.jpg` |
| `alert_20260619-001321-082734.jpg` |
| `alert_20260619-001331-288323.jpg` |
| `alert_20260619-001340-926771.jpg` |
| `alert_20260619-001402-389825.jpg` |

> The register is dynamic: any images added to `photos/` or `detected_fires/` before chapter generation will be re-scanned and mapped to the matching section.

---

## Source documents — conversion status

| Source | On disk | Output (Markdown) | Status |
|--------|---------|-------------------|--------|
| `Project_Stage_1.docx` | ✅ Repo root | `docs/thesis/term1_draft.md` (+ `media/`) | ✅ Converted |
| `Engineering Component Dimensions and Quantities V2.pdf` | ✅ Repo root | `docs/thesis/component_dimensions_v2.md` | ✅ Extracted |

> Both converted files are reference-only. Per the CODE-WINS directive, any technical content in them that contradicts the active codebase is discarded (see `activeContext.md` reconciliation table).

---

## Pending raster figures requested from the user

Mermaid diagrams in `chapter1.md` are generated natively; these two figures require real raster images the user must supply (referenced as placeholders in the chapter):

| Figure | Placeholder path | What is needed |
|--------|------------------|----------------|
| Figure 1.2 | `photos/fig_4room_layout.png` | Plan-view schematic of the 4-room demonstrator: per-room MQ placement, 9 servo barriers (gas valve, 4 doors, 2 corridors, 2 windows), 4 pumps, camera FOV + occupancy line |
| Figure 1.3 | `photos/fig_mq_response_curve.png` | MQ-series $R_s/R_0$ vs gas-concentration response curve (log–log), from datasheet or measured calibration |
