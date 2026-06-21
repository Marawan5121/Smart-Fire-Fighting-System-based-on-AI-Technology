#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
build_deck_chunk3.py  —  SFFS Defense Deck (CHUNK 3: Slides 26-37)
=====================================================================
Pillar 2 — AI & Vision Pipeline.  APPEND MODE.  FLAT 2-D ONLY (no ThreeD).

Behaviour:
  * Opens the existing docs/thesis/SFFS_Defense.pptx (reuses an already-open
    instance if present -> no file-lock conflict).
  * Idempotency: deletes any slide with index > 25 before appending.
  * Appends slides 26-37, then saves in place.

Image embedding:
  * Slides 29 / 30 / 31 / 37 use Shapes.AddPicture with absolute paths,
    explicit width/height, each wrapped in try/except -> graceful degrade to
    a clean bordered caption box if the file is physically missing.

Bench-locked room->sensor mapping (reflected on slides 30 & 35):
  Room 1 = MQ-7 (CO) · Room 2 = MQ-6 · Room 3 = MQ-5 · Room 4 = MQ-2 (smoke)

Honesty splits (locked):
  * Occupancy-aware ALERTING = DELIVERED ; Occupancy-MODULATED suppression = ROADMAP.
  * Face-Identity layer = ROADMAP.

Prerequisite: run build_deck_chunk1.py and build_deck_chunk2.py first.
=====================================================================
"""

import os
import sys
import traceback

import win32com.client as win32

OUT_PATH = r"D:\Graduation Project\cv\docs\thesis\SFFS_Defense.pptx"
PHOTOS   = r"D:\Graduation Project\cv\photos"
DETECTED = r"D:\Graduation Project\cv\Real-Time-Smoke-Fire-Detection-YOLO11\detected_fires"

IMG_SNAPSHOT = os.path.join(DETECTED, "alert_20260619-001311-916843.jpg")
IMG_TELEGRAM = os.path.join(PHOTOS, "telegram masseg.png")
IMG_GPU      = os.path.join(PHOTOS, "gpu_run_fire_fire_case.png")
IMG_CPU      = os.path.join(PHOTOS, "cpu_run_fire_safe_case.png")
IMG_LIVE     = os.path.join(PHOTOS, "running_cv_model.png")


# ---- Colors (PowerPoint stores RGB as 0x00BBGGRR) -------------------
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

# ---- Geometry (16:9 @ 960x540 pt) -----------------------------------
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


def add_picture(slide, path, l, t, w, h, caption=None, frame=INK):
    """Insert a real image; degrade to a clean bordered caption box if missing."""
    try:
        if not os.path.exists(path):
            raise FileNotFoundError(path)
        pic = slide.Shapes.AddPicture(path, False, True, l, t, w, h)
        try:
            pic.Line.Visible = True
            pic.Line.ForeColor.RGB = frame
            pic.Line.Weight = 1.25
        except Exception:
            pass
        if caption:
            textbox(slide, l, t + h + 4, w, 20, caption, size=10, color=MUTED, align="center")
        return pic
    except Exception:
        box = rect(slide, l, t, w, h, fill=PANEL, line=FAINT, rounded=True, weight=1.5)
        name = os.path.basename(path)
        textbox(slide, l + 8, t + h / 2 - 22, w - 16, 44,
                "[ image not found on disk ]\n%s" % name, size=10, color=FAINT,
                align="center", anchor="middle")
        if caption:
            textbox(slide, l, t + h + 4, w, 20, caption, size=10, color=MUTED, align="center")
        return box


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


def tag(slide, l, t, text, delivered=True):
    col = (WATER if delivered else FAINT)
    b = rect(slide, l, t, 110, 24, fill=None, line=col, rounded=True, weight=1.5, dash=(not delivered))
    textbox(slide, l, t + 2, 110, 20, text, size=11, color=col, bold=True, align="center")
    return b


# ====================================================================
# Slides 26-37
# ====================================================================
def slide_26(pres):
    s = new_slide(pres)
    textbox(s, MARGIN - 6, 150, 360, 240, "02", size=200, color=PANEL, bold=True)
    kicker(s, "Pillar 2", BLAZE, t=170)
    textbox(s, MARGIN, 196, W - 2 * MARGIN, 70, "AI-Confirmed Perception", size=48, color=INK, bold=True)
    hline(s, MARGIN, 284, 120, BLAZE, 3)
    sub = textbox(s, MARGIN, 304, 780, 56,
                  "The system doesn't just sense \u2014 it sees, confirms, and tells you.", size=20, color=MUTED)
    g = oval(s, W - 150, 90, 54, 54, fill=None, line=BLAZE, weight=2.5)
    entrance(s, sub, "fade", "after", duration=0.6)
    pulse(s, g, "with")
    transition(s, "push")


def slide_27(pres):
    s = new_slide(pres)
    header(s, "Perception Loop", "Capture \u2192 detect + track \u2192 fuse \u2192 alert.", BLAZE, 24)
    nodes = [("Camera", MUTED), ("YOLOv11", BLAZE), ("Occupancy", WATER),
             ("Decision\nEngine", WATER), ("MQTT /\nTelegram", FIRE)]
    nw, gap = 150, 22
    x = MARGIN
    y = 270
    prev = None
    for i, (txt, col) in enumerate(nodes):
        b = rect(s, x, y, nw, 70, fill=PANEL, line=col, rounded=True, weight=2)
        textbox(s, x, y + 8, nw, 54, txt, size=15, color=INK, bold=True, align="center", anchor="middle")
        entrance(s, b, "fade", "after", delay=0.05, duration=0.3)
        if prev is not None:
            a = arrow(s, x - gap - 2, y + 27, gap, 16, MUTED)
            entrance(s, a, "wipe", "after", duration=0.2)
        prev = b
        x += nw + gap
    transition(s, "morph")


def slide_28(pres):
    s = new_slide(pres)
    header(s, "Detector", "YOLOv11 runs dual-mode: CUDA FP16 on GPU, ONNX on CPU.", BLAZE, 23)
    stages = ["frame", "letterbox 640", "FP16 tensor", "forward", "NMS @ IoU 0.20", "boxes"]
    x = MARGIN
    y = 235
    for i, st in enumerate(stages):
        b = rect(s, x, y, 130, 50, fill=PANEL, line=BLAZE, rounded=True, weight=1.5)
        textbox(s, x, y + 6, 130, 38, st, size=12, color=INK, align="center", anchor="middle", font="Consolas")
        entrance(s, b, "wipe", "after", delay=0.04, duration=0.25)
        if i < len(stages) - 1:
            arrow(s, x + 132, y + 19, 14, 12, MUTED)
        x += 144
        if x > W - MARGIN - 130:
            x = MARGIN
            y += 70
    textbox(s, MARGIN, 360, W - 2 * MARGIN, 110,
            "Acceptance gates:  conf \u2265 0.50 (fire)  ·  conf \u2265 0.75 (smoke)\n"
            "detect() never mutates the frame \u2014 draw() is a single final pass (clean frame for both models)\n"
            "GPU: every frame  ·  CPU: fire inference every 3rd frame (tracking stays every frame)",
            size=14, color=INK)
    transition(s, "morph")


def slide_29(pres):
    s = new_slide(pres)
    header(s, "Snapshot Pipeline", "The moment of detection becomes ground truth.", BLAZE, 24)
    steps = [("YOLOv11 confirms flame/smoke", BLAZE),
             ("capture affected-room frame", FIRE),
             ("save  detected_fires/alert_<ts>.jpg", FIRE),
             ("dispatch to Telegram (non-blocking)", FIRE)]
    y = 210
    for txt, col in steps:
        rect(s, MARGIN, y, 14, 14, fill=col)
        textbox(s, MARGIN + 24, y - 4, 380, 24, txt, size=13, color=INK, font=("Consolas" if "alert_" in txt else "Segoe UI"))
        y += 46
    textbox(s, MARGIN, y + 6, 420, 40,
            "The saved frame is the exact image transmitted \u2014 on-screen detection and mobile alert are identical.",
            size=12, color=MUTED)
    img = add_picture(s, IMG_SNAPSHOT, W - MARGIN - 320, 200, 320, 240,
                      caption="detected_fires\\alert_20260619-001311-916843.jpg")
    entrance(s, img, "fade", "after", duration=0.5)
    transition(s, "morph")


def slide_30(pres):
    s = new_slide(pres)
    header(s, "Instant Visual Alerting", "Image + metrics on the responder's phone.", FIRE, 23)
    # left: real telegram screenshot (portrait)
    img = add_picture(s, IMG_TELEGRAM, MARGIN, 180, 210, 320,
                      caption="photos\\telegram masseg.png")
    entrance(s, img, "fade", "after", duration=0.5)
    # right: caption anatomy
    items = ["State  \u00B7  confidence  \u00B7  source",
             "Per-zone gas  (room \u2192 sensor)",
             "Occupancy count  (rescue priority)",
             "Annotated detection image"]
    y = 184
    for it in items:
        rect(s, MARGIN + 280, y, 12, 12, fill=BLAZE)
        textbox(s, MARGIN + 302, y - 4, 470, 22, it, size=13, color=INK)
        y += 30
    # explicit telemetry string -- bench-locked room->sensor mapping
    tel = rect(s, MARGIN + 300, y + 4, 472, 132, fill=PANEL, line=MUTED, rounded=True, weight=1)
    textbox(s, MARGIN + 314, y + 14, 448, 116,
            "FIRE   conf 0.98   src GAS+CAM\n"
            "R1  MQ-7 (CO)    : 0212\n"
            "R2  MQ-6 (LPG)   : 0480\n"
            "R3  MQ-5 (CH4)   : 0470\n"
            "R4  MQ-2 (smoke) : 2150\n"
            "occupancy: 2 inside",
            size=12, color=INK, font="Consolas")
    entrance(s, tel, "fade", "after", duration=0.4)
    # cooldown logic
    box = rect(s, MARGIN + 300, y + 144, 472, 64, fill=None, line=FIRE, rounded=True, weight=1.5)
    textbox(s, MARGIN + 314, y + 152, 448, 50,
            "AlertGate \u2014 45 s / severity \u00B7 escalation bypasses cooldown \u00B7 SAFE resets",
            size=12, color=INK)
    pulse(s, box, "with")
    transition(s, "morph")


def slide_31(pres):
    s = new_slide(pres)
    header(s, "Edge Benchmark", "Real-time on the edge: 23 FPS GPU, 7 FPS CPU.", BLAZE, 23)
    t1 = rect(s, MARGIN, 180, 200, 84, fill=PANEL, rounded=True)
    textbox(s, MARGIN, 190, 200, 44, "23 FPS", size=30, color=BLAZE, bold=True, align="center")
    textbox(s, MARGIN, 234, 200, 24, "GPU \u00B7 RTX 3050 (FP16)", size=12, color=MUTED, align="center")
    t2 = rect(s, MARGIN + 220, 180, 200, 84, fill=PANEL, rounded=True)
    textbox(s, MARGIN + 220, 190, 200, 44, "7 FPS", size=30, color=WATER, bold=True, align="center")
    textbox(s, MARGIN + 220, 234, 200, 24, "CPU (ONNX, every 3rd)", size=12, color=MUTED, align="center")
    pulse(s, t1, "with")
    g = add_picture(s, IMG_GPU, MARGIN, 290, 300, 180, caption="GPU \u2014 fire case")
    c = add_picture(s, IMG_CPU, MARGIN + 330, 290, 300, 180, caption="CPU \u2014 safe case")
    entrance(s, g, "fade", "after", duration=0.4)
    entrance(s, c, "fade", "after", duration=0.4)
    transition(s, "wipe")


def slide_32(pres):
    s = new_slide(pres)
    header(s, "Dual-Mode Vision", "Count everyone, identify no-one \u2014 yet.", BLAZE, 24)
    d = rect(s, MARGIN, 210, 360, 220, fill=PANEL, line=WATER, rounded=True, weight=2)
    rect(s, MARGIN, 210, 360, 8, fill=WATER)
    textbox(s, MARGIN + 20, 232, 320, 30, "Line-Crossing Headcount", size=18, color=INK, bold=True)
    tag(s, MARGIN + 20, 270, "DELIVERED", True)
    textbox(s, MARGIN + 20, 308, 320, 110,
            "Persistent-ID person tracking; a vertical boundary at x = 300 yields a live, directional occupancy count that feeds rescue-priority alerts.",
            size=13, color=INK)
    r = rect(s, W - MARGIN - 360, 210, 360, 220, fill=BG, line=FAINT, rounded=True, weight=1.5, dash=True)
    rect(s, W - MARGIN - 360, 210, 360, 8, fill=FAINT)
    textbox(s, W - MARGIN - 340, 232, 320, 30, "Face-Identity Layer", size=18, color=MUTED, bold=True)
    tag(s, W - MARGIN - 340, 270, "ROADMAP", False)
    textbox(s, W - MARGIN - 340, 308, 320, 110,
            "A designed extension: per-person identification to drive named priority-rescue alerts. Deferred on privacy and compute grounds.",
            size=13, color=MUTED)
    entrance(s, d, "fly", "after", duration=0.4)
    pulse(s, d, "with")
    entrance(s, r, "fade", "after", duration=0.4)
    transition(s, "morph")


def slide_33(pres):
    s = new_slide(pres)
    header(s, "Occupancy \u00B7 Delivered", "Occupancy by line-crossing: persistent tracks across x = 300.", WATER, 22)
    frame = rect(s, MARGIN, 200, 460, 250, fill=PANEL, line=MUTED, rounded=False, weight=1)
    bx = MARGIN + 250
    line(s, bx, 205, bx, 445, color=WATER, weight=2.5, dash=True)
    textbox(s, bx - 60, 180, 120, 20, "x = 300", size=12, color=WATER, bold=True, align="center")
    textbox(s, MARGIN + 20, 210, 120, 20, "OUTSIDE", size=11, color=MUTED, bold=True)
    textbox(s, bx + 80, 210, 120, 20, "INSIDE", size=11, color=WATER, bold=True)
    p1 = oval(s, MARGIN + 120, 300, 36, 36, fill=None, line=MUTED, weight=2)
    p2 = oval(s, bx + 110, 330, 36, 36, fill=None, line=WATER, weight=2)
    arrow(s, MARGIN + 160, 312, 80, 14, FAINT)
    cnt = rect(s, W - MARGIN - 230, 220, 230, 90, fill=PANEL, line=WATER, rounded=True, weight=2)
    textbox(s, W - MARGIN - 230, 232, 230, 30, "inside_count", size=14, color=MUTED, align="center", font="Consolas")
    textbox(s, W - MARGIN - 230, 258, 230, 44, "+1", size=28, color=WATER, bold=True, align="center")
    textbox(s, W - MARGIN - 245, 330, 245, 110,
            "model.track(persist=True, classes=[0])\n\ncenter_x > 300 = inside\nside change \u21D2 \u00B11 (floored at 0)",
            size=12, color=INK, font="Consolas")
    pulse(s, cnt, "with")
    entrance(s, p2, "fade", "after", duration=0.3)
    transition(s, "wipe")


def slide_34(pres):
    s = new_slide(pres)
    header(s, "Roadmap", "A face-identity tier for priority rescue.", FAINT, 24)
    rect(s, MARGIN, 170, W - 2 * MARGIN, 40, fill=None, line=FAINT, rounded=True, weight=1.5, dash=True)
    textbox(s, MARGIN, 178, W - 2 * MARGIN, 26, "ROADMAP \u2014 designed, not yet implemented", size=15, color=MUTED, bold=True, align="center")
    flow = [("face embeddings", FAINT), ("match known occupants", FAINT), ("named priority-rescue alert", MUTED)]
    x = MARGIN + 40
    y = 280
    for i, (txt, col) in enumerate(flow):
        b = rect(s, x, y, 230, 64, fill=BG, line=col, rounded=True, weight=1.5, dash=True)
        textbox(s, x, y + 8, 230, 48, txt, size=14, color=col, align="center", anchor="middle")
        if i < len(flow) - 1:
            arrow(s, x + 232, y + 26, 30, 14, FAINT)
        x += 262
    textbox(s, MARGIN, 380, W - 2 * MARGIN, 60,
            "Deferred deliberately: identity adds privacy obligations and compute load. The delivered occupancy "
            "count already enables rescue prioritisation without identifying anyone.",
            size=13, color=MUTED)
    transition(s, "fade")


def slide_35(pres):
    s = new_slide(pres)
    header(s, "Fusion", "Sensors and vision agree before the structure escalates.", BLAZE, 23)
    # zonal gas vector (bench-locked room -> sensor mapping)
    textbox(s, MARGIN, 166, W - 2 * MARGIN, 24,
            "Zonal gas vector \u2014 Z1: MQ-7 (CO)  \u00B7  Z2: MQ-6  \u00B7  Z3: MQ-5  \u00B7  Z4: MQ-2 (smoke).  Any zone > 2000 raises the GAS source.",
            size=12, color=MUTED)
    rows = [("Manual override", "FIRE", "1.00", FIRE),
            ("Gas (any zone) / Flame / Camera", "FIRE", "0.92", FIRE),
            ("Multiple sources agree", "FIRE", "0.98", FIRE),
            ("Temperature only", "SENSOR_ALERT", "0.65", BLAZE),
            ("None", "SAFE", "\u2014", WATER)]
    cw = [330, 200, 110]
    x0, y0, rh = MARGIN, 200, 40
    headers = ["Source", "State", "Conf."]
    x = x0
    for j, hh in enumerate(headers):
        rect(s, x, y0, cw[j], rh, fill=INK)
        textbox(s, x + 8, y0 + 7, cw[j] - 12, 26, hh, size=13, color=BG, bold=True,
                align=("left" if j == 0 else "center"))
        x += cw[j]
    for i, (src, st, cf, col) in enumerate(rows):
        x = x0
        vals = [src, st, cf]
        for j, val in enumerate(vals):
            cell = rect(s, x, y0 + (i + 1) * rh, cw[j], rh, fill=PANEL if i % 2 == 0 else BG, line=FAINT, weight=0.5)
            cc = col if j >= 1 else INK
            textbox(s, x + 8, y0 + (i + 1) * rh + 7, cw[j] - 12, 26, val, size=12,
                    color=cc, bold=(j == 1), align=("left" if j == 0 else "center"),
                    font=("Consolas" if j >= 1 else "Segoe UI"))
            if i == 2 and j == 2:
                pulse(s, cell, "with")
            x += cw[j]
    textbox(s, MARGIN, y0 + 6 * rh + 14, W - 2 * MARGIN, 26,
            "Asymmetric debounce: escalate immediately (safety first); de-escalate only after a 5 s cooldown.",
            size=13, color=MUTED)
    transition(s, "wipe")


def slide_36(pres):
    s = new_slide(pres)
    header(s, "Occupancy-Aware", "Alert prioritisation today, suppression modulation next.", WATER, 23)
    d = rect(s, MARGIN, 210, 360, 230, fill=PANEL, line=WATER, rounded=True, weight=2)
    rect(s, MARGIN, 210, 360, 8, fill=WATER)
    textbox(s, MARGIN + 20, 232, 320, 28, "Occupancy-Aware Alerting", size=17, color=INK, bold=True)
    tag(s, MARGIN + 20, 268, "DELIVERED", True)
    textbox(s, MARGIN + 20, 306, 320, 120,
            "The live head-count is embedded in the Telegram alert, raising rescue priority when people remain inside an active hazard zone.",
            size=13, color=INK)
    r = rect(s, W - MARGIN - 360, 210, 360, 230, fill=BG, line=FAINT, rounded=True, weight=1.5, dash=True)
    rect(s, W - MARGIN - 360, 210, 360, 8, fill=FAINT)
    textbox(s, W - MARGIN - 340, 232, 320, 28, "Occupancy-Modulated Suppression", size=16, color=MUTED, bold=True)
    tag(s, W - MARGIN - 340, 268, "ROADMAP", False)
    textbox(s, W - MARGIN - 340, 306, 320, 120,
            "A planned extension in which actuation adapts to protect occupants (e.g. holding an egress path). Not yet wired into the ESP32 control law.",
            size=13, color=MUTED)
    entrance(s, d, "fly", "after", duration=0.4)
    pulse(s, d, "with")
    entrance(s, r, "fade", "after", duration=0.4)
    transition(s, "morph")


def slide_37(pres):
    s = new_slide(pres)
    header(s, "Live Demonstration", "From detection to suppression \u2014 in front of you.", FIRE, 24)
    img = add_picture(s, IMG_LIVE, MARGIN, 190, 380, 240, caption="photos\\running_cv_model.png")
    entrance(s, img, "fade", "after", duration=0.5)
    vx = W - MARGIN - 360
    rect(s, vx, 190, 360, 203, fill=PANEL, line=INK, rounded=True, weight=1.75)
    tri = s.Shapes.AddShape(SH_OVAL, vx + 150, 250, 60, 60)
    tri.Fill.Solid(); tri.Fill.ForeColor.RGB = FIRE; tri.Line.Visible = False
    textbox(s, vx, 360, 360, 24, "Fallback demo video \u2014 insert .mp4", size=12, color=MUTED, align="center")
    steps = ["1 · induce gas in a room \u2192 local suppression fires",
             "2 · present flame \u2192 global override",
             "3 · show YOLO confirmation + snapshot \u2192 Telegram",
             "4 · dashboard mirrors every state live"]
    y = 410
    for st in steps:
        textbox(s, MARGIN, y, W - 2 * MARGIN, 22, st, size=12, color=INK)
        y += 24
    badge = rect(s, W - MARGIN - 90, 150, 90, 28, fill=FIRE, rounded=True)
    textbox(s, W - MARGIN - 90, 154, 90, 22, "LIVE", size=13, color=BG, bold=True, align="center")
    pulse(s, badge, "with")
    pulse(s, tri, "with")
    transition(s, "morph", duration=0.9)


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
        raise FileNotFoundError("SFFS_Defense.pptx not found - run chunks 1 and 2 first.")
    pres = get_presentation()
    while pres.Slides.Count > 25:
        pres.Slides.Item(pres.Slides.Count).Delete()
    for fn in (slide_26, slide_27, slide_28, slide_29, slide_30, slide_31,
               slide_32, slide_33, slide_34, slide_35, slide_36, slide_37):
        fn(pres)
    pres.Save()
    return pres.Slides.Count


if __name__ == "__main__":
    try:
        n = main()
        print(f"DECK_CHUNK3_OK :: total {n} slides -> {OUT_PATH}")
    except Exception:
        print("DECK_CHUNK3_FAILED")
        traceback.print_exc()
        sys.exit(1)
