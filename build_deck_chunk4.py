#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
build_deck_chunk4.py  —  SFFS Defense Deck (CHUNK 4: Slides 38-48)
=====================================================================
Pillar 3 — Validation, Results, Future & Close (+ 2-slide Q&A appendix).
APPEND MODE.  FLAT 2-D ONLY (no ThreeD).  Light theme.

Idempotency: deletes any slide with index > 37, then appends 38-48 and saves.
Reuses an already-open SFFS_Defense.pptx instance (no file-lock conflict).

Locked parameters:
  * Latency (slide 39): edge ~15 ms · YOLOv11 ~43 ms (23 FPS) · Telegram ~1-3 s.
  * Confidence (slide 40): fire 0.50->0.68, smoke 0.75->0.82 (deployed in code).
  * Option-A zonal map (slide 41): R1=MQ-7(CO) R2=MQ-6 R3=MQ-5 R4=MQ-2(smoke).

Prerequisite: run chunks 1-3 first.
"""

import os
import sys
import traceback

import win32com.client as win32

OUT_PATH = r"D:\Graduation Project\cv\docs\thesis\SFFS_Defense.pptx"


def rgb(r, g, b):
    return r | (g << 8) | (b << 16)

BG    = rgb(255, 255, 255)
INK   = rgb(28, 28, 28)
PANEL = rgb(237, 237, 237)
FIRE  = rgb(211, 47, 47)
BLAZE = rgb(245, 124, 0)
WATER = rgb(2, 136, 209)
MUTED = rgb(96, 96, 96)
FAINT = rgb(150, 150, 150)
WHITE = INK
GREY  = MUTED
DIM   = FAINT

W, H = 960.0, 540.0
MARGIN = 72.0

SH_RECT, SH_ROUND, SH_OVAL, SH_RARROW = 1, 5, 9, 33
TXT_HORIZ = 1
AL_LEFT, AL_CENTER, AL_RIGHT = 1, 2, 3
ANCHOR = {"top": 1, "middle": 3, "bottom": 4}
LAYOUT_BLANK = 12

pp = win32.gencache.EnsureDispatch("PowerPoint.Application")
C = win32.constants
pp.Visible = True


def k(*names, default=None):
    for n in names:
        v = getattr(C, n, None)
        if v is not None:
            return v
    return default

E_FADE = k("msoAnimEffectFade", default=10)
E_ZOOM = k("msoAnimEffectZoom", default=23)
E_WIPE = k("msoAnimEffectWipe", default=20)
E_FLY  = k("msoAnimEffectFly", default=2)
E_PULSE = k("msoAnimEffectPulse", "msoAnimEffectGrowShrink",
            "msoAnimEffectFlashBulb", "msoAnimEffectChangeFillColor", default=None)

T_AFTER = k("msoAnimTriggerAfterPrevious", default=3)
T_WITH  = k("msoAnimTriggerWithPrevious", default=2)
T_CLICK = k("msoAnimTriggerOnPageClick", default=1)
TRIG = {"after": T_AFTER, "with": T_WITH, "click": T_CLICK}
EFFECTS = {"fade": E_FADE, "zoom": E_ZOOM, "wipe": E_WIPE, "fly": E_FLY}

TR = {
    "fade":  k("ppEffectFadeSmoothly", "ppEffectFade"),
    "push":  k("ppEffectPushUp", "ppEffectPushDown"),
    "wipe":  k("ppEffectWipeLeft", "ppEffectWipeRight"),
    "zoom":  k("ppEffectZoomIn", "ppEffectZoom", "ppEffectNewsflash"),
    "morph": k("ppEffectFadeSmoothly", "ppEffectFade"),
}

ARROW_TRI = k("msoArrowheadTriangle", default=3)
DASH = k("msoLineDash", default=4)


# ====================================================================
# Helpers (flat 2-D only)
# ====================================================================
def new_slide(pres):
    s = pres.Slides.Add(pres.Slides.Count + 1, LAYOUT_BLANK)
    s.FollowMasterBackground = False
    s.Background.Fill.Solid()
    s.Background.Fill.ForeColor.RGB = BG
    return s


def textbox(slide, l, t, w, h, text, size=18, color=INK, bold=False,
            align="left", font="Segoe UI", anchor="top", italic=False):
    sh = slide.Shapes.AddTextbox(TXT_HORIZ, l, t, w, h)
    tf = sh.TextFrame
    tf.WordWrap = True
    try:
        tf.AutoSize = 0
    except Exception:
        pass
    tr = tf.TextRange
    tr.Text = text
    f = tr.Font
    f.Name = font; f.Size = size; f.Bold = bold; f.Italic = italic
    f.Color.RGB = color
    tr.ParagraphFormat.Alignment = {"left": AL_LEFT, "center": AL_CENTER, "right": AL_RIGHT}[align]
    try:
        tf.VerticalAnchor = ANCHOR[anchor]
    except Exception:
        pass
    return sh


def rect(slide, l, t, w, h, fill=None, line=None, rounded=False, weight=1.0, dash=False):
    sh = slide.Shapes.AddShape(SH_ROUND if rounded else SH_RECT, l, t, w, h)
    if fill is None:
        sh.Fill.Visible = False
    else:
        sh.Fill.Solid(); sh.Fill.ForeColor.RGB = fill
    if line is None:
        sh.Line.Visible = False
    else:
        sh.Line.ForeColor.RGB = line; sh.Line.Weight = weight
        if dash:
            try:
                sh.Line.DashStyle = DASH
            except Exception:
                pass
    try:
        sh.Shadow.Visible = False
    except Exception:
        pass
    return sh


def oval(slide, l, t, w, h, fill=None, line=None, weight=1.5):
    sh = slide.Shapes.AddShape(SH_OVAL, l, t, w, h)
    if fill is None:
        sh.Fill.Visible = False
    else:
        sh.Fill.Solid(); sh.Fill.ForeColor.RGB = fill
    if line is None:
        sh.Line.Visible = False
    else:
        sh.Line.ForeColor.RGB = line; sh.Line.Weight = weight
    return sh


def hline(slide, x, y, length, color=BLAZE, weight=2.0):
    sh = slide.Shapes.AddLine(x, y, x + length, y)
    sh.Line.ForeColor.RGB = color; sh.Line.Weight = weight
    return sh


def line(slide, x1, y1, x2, y2, color=INK, weight=2.0, arrow=False, dash=False):
    ln = slide.Shapes.AddLine(x1, y1, x2, y2)
    ln.Line.ForeColor.RGB = color; ln.Line.Weight = weight
    if arrow:
        try:
            ln.Line.EndArrowheadStyle = ARROW_TRI
        except Exception:
            pass
    if dash:
        try:
            ln.Line.DashStyle = DASH
        except Exception:
            pass
    return ln


def arrow(slide, l, t, w, h, color=BLAZE):
    sh = slide.Shapes.AddShape(SH_RARROW, l, t, w, h)
    sh.Fill.Solid(); sh.Fill.ForeColor.RGB = color; sh.Line.Visible = False
    return sh


def entrance(slide, shape, kind="fade", trigger="after", delay=0.0, duration=0.5):
    try:
        seq = slide.TimeLine.MainSequence
        eff = seq.AddEffect(shape, EFFECTS.get(kind, E_FADE), 0, TRIG[trigger])
        try:
            eff.Timing.Duration = duration
            if delay:
                eff.Timing.TriggerDelayTime = delay
        except Exception:
            pass
        return eff
    except Exception:
        return None


def pulse(slide, shape, trigger="with"):
    if E_PULSE is None:
        return None
    try:
        seq = slide.TimeLine.MainSequence
        eff = seq.AddEffect(shape, E_PULSE, 0, TRIG[trigger])
        try:
            eff.Timing.Duration = 0.9
            eff.Timing.AutoReverse = True
            eff.Timing.RepeatCount = 9999
        except Exception:
            pass
        return eff
    except Exception:
        return None


def transition(slide, name="fade", duration=0.7):
    try:
        eff = TR.get(name)
        if eff is not None:
            slide.SlideShowTransition.EntryEffect = eff
        slide.SlideShowTransition.Duration = duration
        slide.SlideShowTransition.AdvanceOnClick = True
    except Exception:
        pass


def kicker(slide, text, color=BLAZE, t=88.0):
    return textbox(slide, MARGIN, t, W - 2 * MARGIN, 26, text.upper(), size=14, color=color, bold=True)


def header(slide, kick, title, accent=BLAZE, size=24):
    kicker(slide, kick, accent)
    return textbox(slide, MARGIN, 116, W - 2 * MARGIN, 54, title, size=size, color=INK, bold=True)


def gridtable(slide, x0, y0, col_w, rows, rh, header_fill=INK):
    """Render a flat native table (rect + text). rows[0] is the header."""
    for i, row in enumerate(rows):
        x = x0
        for j, val in enumerate(row):
            is_head = (i == 0)
            fill = header_fill if is_head else (PANEL if i % 2 == 1 else BG)
            rect(slide, x, y0 + i * rh, col_w[j], rh, fill=fill, line=FAINT, weight=0.5)
            textbox(slide, x + 8, y0 + i * rh + 6, col_w[j] - 12, rh - 8, str(val),
                    size=11, color=(BG if is_head else INK), bold=is_head,
                    align=("left" if j == 0 else "center"),
                    font=("Segoe UI" if j == 0 else "Consolas"))
            x += col_w[j]


# ====================================================================
# Slides 38-48
# ====================================================================
def slide_38(pres):
    s = new_slide(pres)
    textbox(s, MARGIN - 6, 150, 360, 240, "03", size=200, color=PANEL, bold=True)
    kicker(s, "Pillar 3", FIRE, t=170)
    textbox(s, MARGIN, 196, W - 2 * MARGIN, 70, "Validation & Experimental Results", size=42, color=INK, bold=True)
    hline(s, MARGIN, 284, 120, FIRE, 3)
    sub = textbox(s, MARGIN, 304, 800, 56,
                  "Claims are only as good as the bench that proves them.", size=20, color=MUTED)
    g = oval(s, W - 150, 90, 54, 54, fill=None, line=FIRE, weight=2.5)
    entrance(s, sub, "fade", "after", duration=0.6)
    pulse(s, g, "with")
    transition(s, "push")


def slide_39(pres):
    s = new_slide(pres)
    header(s, "Latency Breakdown", "Where the milliseconds go: suppression is the fast path.", FIRE, 23)
    lanes = [("Local edge \u2192 actuation", "~15 ms", FIRE, 60),
             ("GPU YOLOv11 inference", "~43 ms  (23 FPS)", BLAZE, 150),
             ("Telegram remote alert", "~1\u20133 s  (network)", MUTED, 300)]
    bx = MARGIN + 250
    y = 210
    for name, val, col, bw in lanes:
        textbox(s, MARGIN, y + 4, 240, 30, name, size=14, color=INK, align="right")
        bar = rect(s, bx, y, bw, 34, fill=col, rounded=True)
        textbox(s, bx + bw + 12, y + 4, 250, 28, val, size=14, color=col, bold=True)
        entrance(s, bar, "wipe", "after", delay=0.05, duration=0.35)
        y += 70
    line(s, bx + 60, 196, bx + 60, 250, color=FIRE, weight=1.5, dash=True)
    textbox(s, bx + 66, 196, 220, 20, "one control cycle", size=11, color=FIRE, bold=True)
    textbox(s, MARGIN, 430, W - 2 * MARGIN, 40,
            "Network lane compressed for scale. Local suppression completes far ahead of \u2014 and is never gated by \u2014 the remote alert.",
            size=13, color=MUTED)
    transition(s, "morph")


def slide_40(pres):
    s = new_slide(pres)
    header(s, "False-Alarm Mitigation", "Tighter gates: fewer false alarms, no missed fire.", BLAZE, 23)

    def pill(x, y, oldv, newv, lab):
        rect(s, x, y, 90, 40, fill=PANEL, rounded=True)
        textbox(s, x, y + 8, 90, 24, oldv, size=16, color=MUTED, align="center")
        arrow(s, x + 96, y + 12, 30, 16, BLAZE)
        nb = rect(s, x + 132, y, 90, 40, fill=None, line=BLAZE, rounded=True, weight=2)
        textbox(s, x + 132, y + 8, 90, 24, newv, size=16, color=BLAZE, bold=True, align="center")
        textbox(s, x, y - 22, 222, 20, lab, size=12, color=INK, bold=True, align="center")
        return nb

    p1 = pill(MARGIN + 20, 220, "0.50", "0.68", "fire confidence")
    p2 = pill(MARGIN + 20, 320, "0.75", "0.82", "smoke confidence")
    pulse(s, p1, "with")
    pulse(s, p2, "with")
    mechs = ["Gas: EMA \u03B1 = 0.15  +  60 s warm-up  +  ADC \u2265 2000 gate",
             "Fusion: OR-gate needs a real sensor/vision signal",
             "Temperature alone \u2192 soft SENSOR_ALERT (never FIRE)",
             "Stability: 5 s de-escalation debounce  +  45 s alert cooldown"]
    rx = MARGIN + 320
    y = 210
    for m in mechs:
        rect(s, rx, y, 12, 12, fill=FIRE)
        textbox(s, rx + 22, y - 4, 470, 40, m, size=13, color=INK)
        y += 48
    transition(s, "morph")


def slide_41(pres):
    s = new_slide(pres)
    header(s, "Zonal Actuation \u00B7 Option A", "Validated on the maquette: the right zone, every time.", FIRE, 22)
    rows = [["Zone", "Sensor", "Trigger", "Actuation"],
            ["Room 1", "MQ-7 (CO)", "> 2000 / button", "valve+door+pump 1"],
            ["Room 2", "MQ-6 (LPG)", "> 2000 / button", "door+pump 2"],
            ["Room 3", "MQ-5 (CH4)", "> 2000 / button", "door+window+pump 3"],
            ["Room 4", "MQ-2 (smoke)", "> 2000 / button", "door+window+pump 4"]]
    gridtable(s, MARGIN, 190, [110, 150, 170, 230], rows, 40)
    px, py = W - MARGIN - 170, 360
    textbox(s, px, py - 26, 170, 20, "cross-zone isolation \u2713", size=12, color=WATER, bold=True)
    for i in range(4):
        col = FIRE if i == 0 else PANEL
        cell = rect(s, px + (i % 2) * 86, py + (i // 2) * 56, 80, 50, fill=col, line=MUTED, weight=0.5)
        textbox(s, px + (i % 2) * 86, py + (i // 2) * 56 + 14, 80, 24, "R%d" % (i + 1),
                size=14, color=(BG if i == 0 else INK), bold=True, align="center")
        if i == 0:
            pulse(s, cell, "with")
    textbox(s, MARGIN, 470, 560, 24,
            "Water hysteresis (cut < 10% / resume \u2265 20%) confirmed under live pump load.",
            size=12, color=MUTED)
    transition(s, "wipe")


def slide_42(pres):
    s = new_slide(pres)
    header(s, "Contributions", "A modular, real-time, multi-agent fire platform.", BLAZE, 23)
    cards = [("Architectural modularity", "Decoupled edge / perception tiers over MQTT \u2014 each independently restartable.", WATER),
             ("Real-time multi-agent sync", "Deterministic ESP32 + probabilistic AI fused without blocking the control loop.", BLAZE),
             ("Fail-operational autonomy", "Room-level detection and suppression independent of network and vision.", FIRE),
             ("Dissemination", "Paper-submission framework targeting IJADT.", INK)]
    cw, ch, gx, gy = 360, 110, 36, 24
    for i, (title, body, col) in enumerate(cards):
        x = MARGIN + (i % 2) * (cw + gx)
        y = 200 + (i // 2) * (ch + gy)
        c = rect(s, x, y, cw, ch, fill=PANEL, rounded=True)
        rect(s, x, y, 8, ch, fill=col)
        textbox(s, x + 22, y + 12, cw - 36, 26, title, size=16, color=col, bold=True)
        textbox(s, x + 22, y + 42, cw - 36, 60, body, size=12, color=INK)
        entrance(s, c, "fly", "after", delay=0.05, duration=0.35)
        if i == 3:
            pulse(s, c, "with")
    transition(s, "morph")


def slide_43(pres):
    s = new_slide(pres)
    header(s, "Constraints", "Honest constraints: where the current design stops.", MUTED, 23)
    items = [("Edge-compute thermal throttling", "sustained GPU inference bounds the RTX 3050 clock under load"),
             ("Static camera FOV", "single fixed view; out-of-frame ignition relies on the independent sensor path"),
             ("Remote-alert network dependency", "Telegram delivery needs IP connectivity \u2014 suppression does not"),
             ("Scope bounds", "occupancy is a count, not an identity; thresholds are ADC-domain, not ppm-calibrated")]
    y = 210
    for title, body in items:
        hline(s, MARGIN, y + 24, 6, BLAZE, 3)
        textbox(s, MARGIN + 18, y, 760, 26, title, size=16, color=INK, bold=True)
        textbox(s, MARGIN + 18, y + 26, 780, 24, body, size=13, color=MUTED)
        y += 66
    transition(s, "fade")


def slide_44(pres):
    s = new_slide(pres)
    header(s, "Future Vision", "From roadmap to active development.", WATER, 24)
    tiers = [("Face-Identity tier", "named priority-rescue alerts (privacy + compute design)"),
             ("Occupancy-modulated suppression", "actuation that protects egress and occupants")]
    y = 210
    for title, body in tiers:
        rect(s, MARGIN, y, 250, 64, fill=BG, line=FAINT, rounded=True, weight=1.5, dash=True)
        textbox(s, MARGIN, y + 10, 250, 24, "ROADMAP", size=13, color=MUTED, bold=True, align="center")
        textbox(s, MARGIN, y + 34, 250, 22, title, size=11, color=MUTED, align="center")
        arrow(s, MARGIN + 262, y + 24, 50, 18, WATER)
        new = rect(s, MARGIN + 330, y, 330, 64, fill=PANEL, line=WATER, rounded=True, weight=2)
        textbox(s, MARGIN + 346, y + 10, 300, 24, "ACTIVE DEVELOPMENT", size=13, color=WATER, bold=True)
        textbox(s, MARGIN + 346, y + 34, 300, 22, body, size=11, color=INK)
        entrance(s, new, "wipe", "after", delay=0.05, duration=0.4)
        y += 86
    rect(s, MARGIN, 396, W - 2 * MARGIN, 50, fill=PANEL, rounded=True)
    textbox(s, MARGIN + 16, 408, W - 2 * MARGIN - 32, 30,
            "Platform extensions:  MQTT-SN clustering  \u00B7  LoRaWAN fallback  \u00B7  TensorRT INT8 quantization",
            size=13, color=INK, bold=True, anchor="middle")
    transition(s, "morph")


def slide_45(pres):
    s = new_slide(pres)
    kicker(s, "Conclusion", FIRE)
    clauses = [("A fail-operational,", WATER),
               ("AI-confirmed,", BLAZE),
               ("room-level fire system that suppresses", FIRE),
               ("in one control cycle \u2014 without waiting on the network.", INK)]
    y = 170
    shapes = []
    for txt, col in clauses:
        sh = textbox(s, MARGIN, y, W - 2 * MARGIN, 52, txt, size=30, color=col, bold=True)
        shapes.append(sh)
        y += 56
    for sh in shapes:
        entrance(s, sh, "fade", "after", delay=0.05, duration=0.5)
    pulse(s, shapes[2], "with")
    ticks = [("Zonal Autonomy", WATER), ("AI-Confirmed Perception", BLAZE), ("Engineered Reliability", FIRE)]
    x = MARGIN
    for lab, col in ticks:
        rect(s, x, 430, 14, 14, fill=col)
        textbox(s, x + 22, 426, 230, 24, lab, size=13, color=INK)
        x += 280
    transition(s, "morph")


def slide_46(pres):
    s = new_slide(pres)
    textbox(s, MARGIN, 150, W - 2 * MARGIN, 60, "Thank you.", size=44, color=INK, bold=True)
    textbox(s, MARGIN, 214, W - 2 * MARGIN, 30, "Questions & committee discussion.", size=20, color=MUTED)
    thumbs = [("Two-tier system", WATER), ("Zonal plant", FIRE), ("Edge control loop", WATER),
              ("AI pipeline", BLAZE), ("Snapshot \u2192 Telegram", BLAZE), ("Results", FIRE)]
    cw, ch, gx, gy = 250, 70, 24, 20
    x0, y0 = MARGIN, 300
    for i, (lab, col) in enumerate(thumbs):
        x = x0 + (i % 3) * (cw + gx)
        y = y0 + (i // 3) * (ch + gy)
        card = rect(s, x, y, cw, ch, fill=PANEL, line=col, rounded=True, weight=1.5)
        rect(s, x, y, 6, ch, fill=col)
        textbox(s, x + 18, y + 22, cw - 28, 28, lab, size=14, color=INK, bold=True, anchor="middle")
        entrance(s, card, "fade", "after", delay=0.04, duration=0.3)
        if i == 0:
            pulse(s, card, "with")
    transition(s, "fade")


def slide_47(pres):
    s = new_slide(pres)
    header(s, "Appendix A \u00B7 Backup", "Hardware & electrical reference.", MUTED, 22)
    rows = [["Item", "Value"],
            ["MCU", "ESP32-WROOM-32DA (LX6 dual-core, 240 MHz)"],
            ["Gas (ADC1)", "R1 GPIO35 MQ-7 / R2 34 MQ-6 / R3 33 MQ-5 / R4 32 MQ-2"],
            ["Gas threshold", "ADC 2000 (mid of 0\u20134095) \u00B7 EMA \u03B1 0.15 \u00B7 60 s warm-up"],
            ["Actuation", "PCA9685 @0x40, 50 Hz \u00B7 9 servos \u00B7 4 relay pumps (active-LOW)"],
            ["Flame / DHT22", "GPIO27 active-LOW / GPIO26 one-wire"],
            ["Water (HC-SR04)", "TRIG 15 / ECHO 36 \u00B7 cut < 10% resume \u2265 20%"],
            ["Power budget", "~4.1 A peak on 5 V / 5 A (20% margin)"]]
    gridtable(s, MARGIN, 175, [200, 620], rows, 38)
    transition(s, "wipe")


def slide_48(pres):
    s = new_slide(pres)
    header(s, "Appendix B \u00B7 Backup", "Software & AI reference.", MUTED, 22)
    rows = [["Item", "Value"],
            ["Vision", "YOLOv11-nano best_nano_111.pt \u00B7 CUDA FP16 \u00B7 imgsz 640 \u00B7 IoU 0.20"],
            ["Confidence", "fire 0.68 \u00B7 smoke 0.82"],
            ["Throughput", "23 FPS GPU (RTX 3050) / 7 FPS CPU (ONNX, every 3rd frame)"],
            ["Occupancy", "yolo11n person track \u00B7 line x = 300 \u00B7 directional count"],
            ["Fusion", "manual 1.00 / gas|flame|cam 0.92 / multi 0.98 / temp 0.65"],
            ["Alerting", "Telegram sffs_bot \u00B7 45 s cooldown \u00B7 escalation bypass"],
            ["Resilience", "edge predicate broker-independent \u00B7 failsafe FSM \u00B7 MQTT LWT"]]
    gridtable(s, MARGIN, 175, [200, 620], rows, 38)
    transition(s, "wipe")


# ====================================================================
# Build (APPEND MODE)
# ====================================================================
def get_presentation():
    target = os.path.normcase(os.path.abspath(OUT_PATH))
    for i in range(1, pp.Presentations.Count + 1):
        p = pp.Presentations.Item(i)
        try:
            if os.path.normcase(os.path.abspath(p.FullName)) == target:
                return p
        except Exception:
            continue
    return pp.Presentations.Open(OUT_PATH, ReadOnly=False, WithWindow=True)


def main():
    if not os.path.exists(OUT_PATH):
        raise FileNotFoundError("SFFS_Defense.pptx not found - run chunks 1-3 first.")
    pres = get_presentation()
    while pres.Slides.Count > 37:
        pres.Slides.Item(pres.Slides.Count).Delete()
    for fn in (slide_38, slide_39, slide_40, slide_41, slide_42, slide_43,
               slide_44, slide_45, slide_46, slide_47, slide_48):
        fn(pres)
    pres.Save()
    return pres.Slides.Count


if __name__ == "__main__":
    try:
        n = main()
        print(f"DECK_CHUNK4_OK :: total {n} slides -> {OUT_PATH}")
    except Exception:
        print("DECK_CHUNK4_FAILED")
        traceback.print_exc()
        sys.exit(1)#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
build_deck_chunk4.py  —  SFFS Defense Deck (CHUNK 4: Slides 38-48)
=====================================================================
Pillar 3 — Validation, Results, Future & Close (+ 2-slide Q&A appendix).
APPEND MODE.  FLAT 2-D ONLY (no ThreeD).  Light theme.

Idempotency: deletes any slide with index > 37, then appends 38-48 and saves.
Reuses an already-open SFFS_Defense.pptx instance (no file-lock conflict).

Locked parameters:
  * Latency (slide 39): edge ~15 ms · YOLOv11 ~43 ms (23 FPS) · Telegram ~1-3 s.
  * Confidence (slide 40): fire 0.50->0.68, smoke 0.75->0.82 (deployed in code).
  * Option-A zonal map (slide 41): R1=MQ-7(CO) R2=MQ-6 R3=MQ-5 R4=MQ-2(smoke).

Prerequisite: run chunks 1-3 first.
"""

import os
import sys
import traceback

import win32com.client as win32

OUT_PATH = r"D:\Graduation Project\cv\docs\thesis\SFFS_Defense.pptx"


def rgb(r, g, b):
    return r | (g << 8) | (b << 16)

BG    = rgb(255, 255, 255)
INK   = rgb(28, 28, 28)
PANEL = rgb(237, 237, 237)
FIRE  = rgb(211, 47, 47)
BLAZE = rgb(245, 124, 0)
WATER = rgb(2, 136, 209)
MUTED = rgb(96, 96, 96)
FAINT = rgb(150, 150, 150)
WHITE = INK
GREY  = MUTED
DIM   = FAINT

W, H = 960.0, 540.0
MARGIN = 72.0

SH_RECT, SH_ROUND, SH_OVAL, SH_RARROW = 1, 5, 9, 33
TXT_HORIZ = 1
AL_LEFT, AL_CENTER, AL_RIGHT = 1, 2, 3
ANCHOR = {"top": 1, "middle": 3, "bottom": 4}
LAYOUT_BLANK = 12

pp = win32.gencache.EnsureDispatch("PowerPoint.Application")
C = win32.constants
pp.Visible = True


def k(*names, default=None):
    for n in names:
        v = getattr(C, n, None)
        if v is not None:
            return v
    return default

E_FADE = k("msoAnimEffectFade", default=10)
E_ZOOM = k("msoAnimEffectZoom", default=23)
E_WIPE = k("msoAnimEffectWipe", default=20)
E_FLY  = k("msoAnimEffectFly", default=2)
E_PULSE = k("msoAnimEffectPulse", "msoAnimEffectGrowShrink",
            "msoAnimEffectFlashBulb", "msoAnimEffectChangeFillColor", default=None)

T_AFTER = k("msoAnimTriggerAfterPrevious", default=3)
T_WITH  = k("msoAnimTriggerWithPrevious", default=2)
T_CLICK = k("msoAnimTriggerOnPageClick", default=1)
TRIG = {"after": T_AFTER, "with": T_WITH, "click": T_CLICK}
EFFECTS = {"fade": E_FADE, "zoom": E_ZOOM, "wipe": E_WIPE, "fly": E_FLY}

TR = {
    "fade":  k("ppEffectFadeSmoothly", "ppEffectFade"),
    "push":  k("ppEffectPushUp", "ppEffectPushDown"),
    "wipe":  k("ppEffectWipeLeft", "ppEffectWipeRight"),
    "zoom":  k("ppEffectZoomIn", "ppEffectZoom", "ppEffectNewsflash"),
    "morph": k("ppEffectFadeSmoothly", "ppEffectFade"),
}

ARROW_TRI = k("msoArrowheadTriangle", default=3)
DASH = k("msoLineDash", default=4)


# ====================================================================
# Helpers (flat 2-D only)
# ====================================================================
def new_slide(pres):
    s = pres.Slides.Add(pres.Slides.Count + 1, LAYOUT_BLANK)
    s.FollowMasterBackground = False
    s.Background.Fill.Solid()
    s.Background.Fill.ForeColor.RGB = BG
    return s


def textbox(slide, l, t, w, h, text, size=18, color=INK, bold=False,
            align="left", font="Segoe UI", anchor="top", italic=False):
    sh = slide.Shapes.AddTextbox(TXT_HORIZ, l, t, w, h)
    tf = sh.TextFrame
    tf.WordWrap = True
    try:
        tf.AutoSize = 0
    except Exception:
        pass
    tr = tf.TextRange
    tr.Text = text
    f = tr.Font
    f.Name = font; f.Size = size; f.Bold = bold; f.Italic = italic
    f.Color.RGB = color
    tr.ParagraphFormat.Alignment = {"left": AL_LEFT, "center": AL_CENTER, "right": AL_RIGHT}[align]
    try:
        tf.VerticalAnchor = ANCHOR[anchor]
    except Exception:
        pass
    return sh


def rect(slide, l, t, w, h, fill=None, line=None, rounded=False, weight=1.0, dash=False):
    sh = slide.Shapes.AddShape(SH_ROUND if rounded else SH_RECT, l, t, w, h)
    if fill is None:
        sh.Fill.Visible = False
    else:
        sh.Fill.Solid(); sh.Fill.ForeColor.RGB = fill
    if line is None:
        sh.Line.Visible = False
    else:
        sh.Line.ForeColor.RGB = line; sh.Line.Weight = weight
        if dash:
            try:
                sh.Line.DashStyle = DASH
            except Exception:
                pass
    try:
        sh.Shadow.Visible = False
    except Exception:
        pass
    return sh


def oval(slide, l, t, w, h, fill=None, line=None, weight=1.5):
    sh = slide.Shapes.AddShape(SH_OVAL, l, t, w, h)
    if fill is None:
        sh.Fill.Visible = False
    else:
        sh.Fill.Solid(); sh.Fill.ForeColor.RGB = fill
    if line is None:
        sh.Line.Visible = False
    else:
        sh.Line.ForeColor.RGB = line; sh.Line.Weight = weight
    return sh


def hline(slide, x, y, length, color=BLAZE, weight=2.0):
    sh = slide.Shapes.AddLine(x, y, x + length, y)
    sh.Line.ForeColor.RGB = color; sh.Line.Weight = weight
    return sh


def line(slide, x1, y1, x2, y2, color=INK, weight=2.0, arrow=False, dash=False):
    ln = slide.Shapes.AddLine(x1, y1, x2, y2)
    ln.Line.ForeColor.RGB = color; ln.Line.Weight = weight
    if arrow:
        try:
            ln.Line.EndArrowheadStyle = ARROW_TRI
        except Exception:
            pass
    if dash:
        try:
            ln.Line.DashStyle = DASH
        except Exception:
            pass
    return ln


def arrow(slide, l, t, w, h, color=BLAZE):
    sh = slide.Shapes.AddShape(SH_RARROW, l, t, w, h)
    sh.Fill.Solid(); sh.Fill.ForeColor.RGB = color; sh.Line.Visible = False
    return sh


def entrance(slide, shape, kind="fade", trigger="after", delay=0.0, duration=0.5):
    try:
        seq = slide.TimeLine.MainSequence
        eff = seq.AddEffect(shape, EFFECTS.get(kind, E_FADE), 0, TRIG[trigger])
        try:
            eff.Timing.Duration = duration
            if delay:
                eff.Timing.TriggerDelayTime = delay
        except Exception:
            pass
        return eff
    except Exception:
        return None


def pulse(slide, shape, trigger="with"):
    if E_PULSE is None:
        return None
    try:
        seq = slide.TimeLine.MainSequence
        eff = seq.AddEffect(shape, E_PULSE, 0, TRIG[trigger])
        try:
            eff.Timing.Duration = 0.9
            eff.Timing.AutoReverse = True
            eff.Timing.RepeatCount = 9999
        except Exception:
            pass
        return eff
    except Exception:
        return None


def transition(slide, name="fade", duration=0.7):
    try:
        eff = TR.get(name)
        if eff is not None:
            slide.SlideShowTransition.EntryEffect = eff
        slide.SlideShowTransition.Duration = duration
        slide.SlideShowTransition.AdvanceOnClick = True
    except Exception:
        pass


def kicker(slide, text, color=BLAZE, t=88.0):
    return textbox(slide, MARGIN, t, W - 2 * MARGIN, 26, text.upper(), size=14, color=color, bold=True)


def header(slide, kick, title, accent=BLAZE, size=24):
    kicker(slide, kick, accent)
    return textbox(slide, MARGIN, 116, W - 2 * MARGIN, 54, title, size=size, color=INK, bold=True)


def gridtable(slide, x0, y0, col_w, rows, rh, header_fill=INK):
    """Render a flat native table (rect + text). rows[0] is the header."""
    for i, row in enumerate(rows):
        x = x0
        for j, val in enumerate(row):
            is_head = (i == 0)
            fill = header_fill if is_head else (PANEL if i % 2 == 1 else BG)
            rect(slide, x, y0 + i * rh, col_w[j], rh, fill=fill, line=FAINT, weight=0.5)
            textbox(slide, x + 8, y0 + i * rh + 6, col_w[j] - 12, rh - 8, str(val),
                    size=11, color=(BG if is_head else INK), bold=is_head,
                    align=("left" if j == 0 else "center"),
                    font=("Segoe UI" if j == 0 else "Consolas"))
            x += col_w[j]


# ====================================================================
# Slides 38-48
# ====================================================================
def slide_38(pres):
    s = new_slide(pres)
    textbox(s, MARGIN - 6, 150, 360, 240, "03", size=200, color=PANEL, bold=True)
    kicker(s, "Pillar 3", FIRE, t=170)
    textbox(s, MARGIN, 196, W - 2 * MARGIN, 70, "Validation & Experimental Results", size=42, color=INK, bold=True)
    hline(s, MARGIN, 284, 120, FIRE, 3)
    sub = textbox(s, MARGIN, 304, 800, 56,
                  "Claims are only as good as the bench that proves them.", size=20, color=MUTED)
    g = oval(s, W - 150, 90, 54, 54, fill=None, line=FIRE, weight=2.5)
    entrance(s, sub, "fade", "after", duration=0.6)
    pulse(s, g, "with")
    transition(s, "push")


def slide_39(pres):
    s = new_slide(pres)
    header(s, "Latency Breakdown", "Where the milliseconds go: suppression is the fast path.", FIRE, 23)
    lanes = [("Local edge \u2192 actuation", "~15 ms", FIRE, 60),
             ("GPU YOLOv11 inference", "~43 ms  (23 FPS)", BLAZE, 150),
             ("Telegram remote alert", "~1\u20133 s  (network)", MUTED, 300)]
    bx = MARGIN + 250
    y = 210
    for name, val, col, bw in lanes:
        textbox(s, MARGIN, y + 4, 240, 30, name, size=14, color=INK, align="right")
        bar = rect(s, bx, y, bw, 34, fill=col, rounded=True)
        textbox(s, bx + bw + 12, y + 4, 250, 28, val, size=14, color=col, bold=True)
        entrance(s, bar, "wipe", "after", delay=0.05, duration=0.35)
        y += 70
    line(s, bx + 60, 196, bx + 60, 250, color=FIRE, weight=1.5, dash=True)
    textbox(s, bx + 66, 196, 220, 20, "one control cycle", size=11, color=FIRE, bold=True)
    textbox(s, MARGIN, 430, W - 2 * MARGIN, 40,
            "Network lane compressed for scale. Local suppression completes far ahead of \u2014 and is never gated by \u2014 the remote alert.",
            size=13, color=MUTED)
    transition(s, "morph")


def slide_40(pres):
    s = new_slide(pres)
    header(s, "False-Alarm Mitigation", "Tighter gates: fewer false alarms, no missed fire.", BLAZE, 23)

    def pill(x, y, oldv, newv, lab):
        rect(s, x, y, 90, 40, fill=PANEL, rounded=True)
        textbox(s, x, y + 8, 90, 24, oldv, size=16, color=MUTED, align="center")
        arrow(s, x + 96, y + 12, 30, 16, BLAZE)
        nb = rect(s, x + 132, y, 90, 40, fill=None, line=BLAZE, rounded=True, weight=2)
        textbox(s, x + 132, y + 8, 90, 24, newv, size=16, color=BLAZE, bold=True, align="center")
        textbox(s, x, y - 22, 222, 20, lab, size=12, color=INK, bold=True, align="center")
        return nb

    p1 = pill(MARGIN + 20, 220, "0.50", "0.68", "fire confidence")
    p2 = pill(MARGIN + 20, 320, "0.75", "0.82", "smoke confidence")
    pulse(s, p1, "with")
    pulse(s, p2, "with")
    mechs = ["Gas: EMA \u03B1 = 0.15  +  60 s warm-up  +  ADC \u2265 2000 gate",
             "Fusion: OR-gate needs a real sensor/vision signal",
             "Temperature alone \u2192 soft SENSOR_ALERT (never FIRE)",
             "Stability: 5 s de-escalation debounce  +  45 s alert cooldown"]
    rx = MARGIN + 320
    y = 210
    for m in mechs:
        rect(s, rx, y, 12, 12, fill=FIRE)
        textbox(s, rx + 22, y - 4, 470, 40, m, size=13, color=INK)
        y += 48
    transition(s, "morph")


def slide_41(pres):
    s = new_slide(pres)
    header(s, "Zonal Actuation \u00B7 Option A", "Validated on the maquette: the right zone, every time.", FIRE, 22)
    rows = [["Zone", "Sensor", "Trigger", "Actuation"],
            ["Room 1", "MQ-7 (CO)", "> 2000 / button", "valve+door+pump 1"],
            ["Room 2", "MQ-6 (LPG)", "> 2000 / button", "door+pump 2"],
            ["Room 3", "MQ-5 (CH4)", "> 2000 / button", "door+window+pump 3"],
            ["Room 4", "MQ-2 (smoke)", "> 2000 / button", "door+window+pump 4"]]
    gridtable(s, MARGIN, 190, [110, 150, 170, 230], rows, 40)
    px, py = W - MARGIN - 170, 360
    textbox(s, px, py - 26, 170, 20, "cross-zone isolation \u2713", size=12, color=WATER, bold=True)
    for i in range(4):
        col = FIRE if i == 0 else PANEL
        cell = rect(s, px + (i % 2) * 86, py + (i // 2) * 56, 80, 50, fill=col, line=MUTED, weight=0.5)
        textbox(s, px + (i % 2) * 86, py + (i // 2) * 56 + 14, 80, 24, "R%d" % (i + 1),
                size=14, color=(BG if i == 0 else INK), bold=True, align="center")
        if i == 0:
            pulse(s, cell, "with")
    textbox(s, MARGIN, 470, 560, 24,
            "Water hysteresis (cut < 10% / resume \u2265 20%) confirmed under live pump load.",
            size=12, color=MUTED)
    transition(s, "wipe")


def slide_42(pres):
    s = new_slide(pres)
    header(s, "Contributions", "A modular, real-time, multi-agent fire platform.", BLAZE, 23)
    cards = [("Architectural modularity", "Decoupled edge / perception tiers over MQTT \u2014 each independently restartable.", WATER),
             ("Real-time multi-agent sync", "Deterministic ESP32 + probabilistic AI fused without blocking the control loop.", BLAZE),
             ("Fail-operational autonomy", "Room-level detection and suppression independent of network and vision.", FIRE),
             ("Dissemination", "Paper-submission framework targeting IJADT.", INK)]
    cw, ch, gx, gy = 360, 110, 36, 24
    for i, (title, body, col) in enumerate(cards):
        x = MARGIN + (i % 2) * (cw + gx)
        y = 200 + (i // 2) * (ch + gy)
        c = rect(s, x, y, cw, ch, fill=PANEL, rounded=True)
        rect(s, x, y, 8, ch, fill=col)
        textbox(s, x + 22, y + 12, cw - 36, 26, title, size=16, color=col, bold=True)
        textbox(s, x + 22, y + 42, cw - 36, 60, body, size=12, color=INK)
        entrance(s, c, "fly", "after", delay=0.05, duration=0.35)
        if i == 3:
            pulse(s, c, "with")
    transition(s, "morph")


def slide_43(pres):
    s = new_slide(pres)
    header(s, "Constraints", "Honest constraints: where the current design stops.", MUTED, 23)
    items = [("Edge-compute thermal throttling", "sustained GPU inference bounds the RTX 3050 clock under load"),
             ("Static camera FOV", "single fixed view; out-of-frame ignition relies on the independent sensor path"),
             ("Remote-alert network dependency", "Telegram delivery needs IP connectivity \u2014 suppression does not"),
             ("Scope bounds", "occupancy is a count, not an identity; thresholds are ADC-domain, not ppm-calibrated")]
    y = 210
    for title, body in items:
        hline(s, MARGIN, y + 24, 6, BLAZE, 3)
        textbox(s, MARGIN + 18, y, 760, 26, title, size=16, color=INK, bold=True)
        textbox(s, MARGIN + 18, y + 26, 780, 24, body, size=13, color=MUTED)
        y += 66
    transition(s, "fade")


def slide_44(pres):
    s = new_slide(pres)
    header(s, "Future Vision", "From roadmap to active development.", WATER, 24)
    tiers = [("Face-Identity tier", "named priority-rescue alerts (privacy + compute design)"),
             ("Occupancy-modulated suppression", "actuation that protects egress and occupants")]
    y = 210
    for title, body in tiers:
        rect(s, MARGIN, y, 250, 64, fill=BG, line=FAINT, rounded=True, weight=1.5, dash=True)
        textbox(s, MARGIN, y + 10, 250, 24, "ROADMAP", size=13, color=MUTED, bold=True, align="center")
        textbox(s, MARGIN, y + 34, 250, 22, title, size=11, color=MUTED, align="center")
        arrow(s, MARGIN + 262, y + 24, 50, 18, WATER)
        new = rect(s, MARGIN + 330, y, 330, 64, fill=PANEL, line=WATER, rounded=True, weight=2)
        textbox(s, MARGIN + 346, y + 10, 300, 24, "ACTIVE DEVELOPMENT", size=13, color=WATER, bold=True)
        textbox(s, MARGIN + 346, y + 34, 300, 22, body, size=11, color=INK)
        entrance(s, new, "wipe", "after", delay=0.05, duration=0.4)
        y += 86
    rect(s, MARGIN, 396, W - 2 * MARGIN, 50, fill=PANEL, rounded=True)
    textbox(s, MARGIN + 16, 408, W - 2 * MARGIN - 32, 30,
            "Platform extensions:  MQTT-SN clustering  \u00B7  LoRaWAN fallback  \u00B7  TensorRT INT8 quantization",
            size=13, color=INK, bold=True, anchor="middle")
    transition(s, "morph")


def slide_45(pres):
    s = new_slide(pres)
    kicker(s, "Conclusion", FIRE)
    clauses = [("A fail-operational,", WATER),
               ("AI-confirmed,", BLAZE),
               ("room-level fire system that suppresses", FIRE),
               ("in one control cycle \u2014 without waiting on the network.", INK)]
    y = 170
    shapes = []
    for txt, col in clauses:
        sh = textbox(s, MARGIN, y, W - 2 * MARGIN, 52, txt, size=30, color=col, bold=True)
        shapes.append(sh)
        y += 56
    for sh in shapes:
        entrance(s, sh, "fade", "after", delay=0.05, duration=0.5)
    pulse(s, shapes[2], "with")
    ticks = [("Zonal Autonomy", WATER), ("AI-Confirmed Perception", BLAZE), ("Engineered Reliability", FIRE)]
    x = MARGIN
    for lab, col in ticks:
        rect(s, x, 430, 14, 14, fill=col)
        textbox(s, x + 22, 426, 230, 24, lab, size=13, color=INK)
        x += 280
    transition(s, "morph")


def slide_46(pres):
    s = new_slide(pres)
    textbox(s, MARGIN, 150, W - 2 * MARGIN, 60, "Thank you.", size=44, color=INK, bold=True)
    textbox(s, MARGIN, 214, W - 2 * MARGIN, 30, "Questions & committee discussion.", size=20, color=MUTED)
    thumbs = [("Two-tier system", WATER), ("Zonal plant", FIRE), ("Edge control loop", WATER),
              ("AI pipeline", BLAZE), ("Snapshot \u2192 Telegram", BLAZE), ("Results", FIRE)]
    cw, ch, gx, gy = 250, 70, 24, 20
    x0, y0 = MARGIN, 300
    for i, (lab, col) in enumerate(thumbs):
        x = x0 + (i % 3) * (cw + gx)
        y = y0 + (i // 3) * (ch + gy)
        card = rect(s, x, y, cw, ch, fill=PANEL, line=col, rounded=True, weight=1.5)
        rect(s, x, y, 6, ch, fill=col)
        textbox(s, x + 18, y + 22, cw - 28, 28, lab, size=14, color=INK, bold=True, anchor="middle")
        entrance(s, card, "fade", "after", delay=0.04, duration=0.3)
        if i == 0:
            pulse(s, card, "with")
    transition(s, "fade")


def slide_47(pres):
    s = new_slide(pres)
    header(s, "Appendix A \u00B7 Backup", "Hardware & electrical reference.", MUTED, 22)
    rows = [["Item", "Value"],
            ["MCU", "ESP32-WROOM-32DA (LX6 dual-core, 240 MHz)"],
            ["Gas (ADC1)", "R1 GPIO35 MQ-7 / R2 34 MQ-6 / R3 33 MQ-5 / R4 32 MQ-2"],
            ["Gas threshold", "ADC 2000 (mid of 0\u20134095) \u00B7 EMA \u03B1 0.15 \u00B7 60 s warm-up"],
            ["Actuation", "PCA9685 @0x40, 50 Hz \u00B7 9 servos \u00B7 4 relay pumps (active-LOW)"],
            ["Flame / DHT22", "GPIO27 active-LOW / GPIO26 one-wire"],
            ["Water (HC-SR04)", "TRIG 15 / ECHO 36 \u00B7 cut < 10% resume \u2265 20%"],
            ["Power budget", "~4.1 A peak on 5 V / 5 A (20% margin)"]]
    gridtable(s, MARGIN, 175, [200, 620], rows, 38)
    transition(s, "wipe")


def slide_48(pres):
    s = new_slide(pres)
    header(s, "Appendix B \u00B7 Backup", "Software & AI reference.", MUTED, 22)
    rows = [["Item", "Value"],
            ["Vision", "YOLOv11-nano best_nano_111.pt \u00B7 CUDA FP16 \u00B7 imgsz 640 \u00B7 IoU 0.20"],
            ["Confidence", "fire 0.68 \u00B7 smoke 0.82"],
            ["Throughput", "23 FPS GPU (RTX 3050) / 7 FPS CPU (ONNX, every 3rd frame)"],
            ["Occupancy", "yolo11n person track \u00B7 line x = 300 \u00B7 directional count"],
            ["Fusion", "manual 1.00 / gas|flame|cam 0.92 / multi 0.98 / temp 0.65"],
            ["Alerting", "Telegram sffs_bot \u00B7 45 s cooldown \u00B7 escalation bypass"],
            ["Resilience", "edge predicate broker-independent \u00B7 failsafe FSM \u00B7 MQTT LWT"]]
    gridtable(s, MARGIN, 175, [200, 620], rows, 38)
    transition(s, "wipe")


# ====================================================================
# Build (APPEND MODE)
# ====================================================================
def get_presentation():
    target = os.path.normcase(os.path.abspath(OUT_PATH))
    for i in range(1, pp.Presentations.Count + 1):
        p = pp.Presentations.Item(i)
        try:
            if os.path.normcase(os.path.abspath(p.FullName)) == target:
                return p
        except Exception:
            continue
    return pp.Presentations.Open(OUT_PATH, ReadOnly=False, WithWindow=True)


def main():
    if not os.path.exists(OUT_PATH):
        raise FileNotFoundError("SFFS_Defense.pptx not found - run chunks 1-3 first.")
    pres = get_presentation()
    while pres.Slides.Count > 37:
        pres.Slides.Item(pres.Slides.Count).Delete()
    for fn in (slide_38, slide_39, slide_40, slide_41, slide_42, slide_43,
               slide_44, slide_45, slide_46, slide_47, slide_48):
        fn(pres)
    pres.Save()
    return pres.Slides.Count


if __name__ == "__main__":
    try:
        n = main()
        print(f"DECK_CHUNK4_OK :: total {n} slides -> {OUT_PATH}")
    except Exception:
        print("DECK_CHUNK4_FAILED")
        traceback.print_exc()
        sys.exit(1)