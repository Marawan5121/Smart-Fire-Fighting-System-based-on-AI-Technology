"""
SFFS Fire/Smoke Detector — Dual-Mode (CUDA FP16 / CPU ONNX)
===========================================================
Detection is fully decoupled from visualization:
  - detect(frame)        → runs inference on the CLEAN raw frame, returns a
                           FireResult (no drawing, no frame mutation).
  - draw(frame, result)  → renders boxes/labels onto a frame in one final pass.

This separation is required so the occupancy tracker and the fire model both
ingest an identical, un-annotated, fixed-resolution frame (fixes tracking churn).

Device handling:
  - 'cuda' → load .pt, move to GPU, run FP16 (half=True) for ~2x throughput.
  - 'cpu'  → prefer a pre-exported .onnx (static imgsz=640) for OpenVINO/ORT
             acceleration; fall back to .pt with a loud warning if absent.
"""

import numpy as np
from ultralytics import YOLO
import cvzone
import cv2
import logging
from pathlib import Path
from typing import NamedTuple, Optional, List, Tuple


class FireResult(NamedTuple):
    """Immutable detection result for a single frame."""
    label: Optional[str]                              # "Fire" | "Smoke" | None
    confidence: float                                 # confidence of `label`
    drawables: List[Tuple[np.ndarray, str, float]]    # (box_xyxy, class_name, conf)


# A reusable empty result for skipped/failed frames.
EMPTY_RESULT = FireResult(label=None, confidence=0.0, drawables=[])


class Detector:
    def __init__(
        self,
        model_path: Path,
        device: str = "cpu",
        imgsz: int = 640,
        iou_threshold: float = 0.2,
        min_confidence: float = 0.68,
        smoke_confidence: float = 0.82,
    ):
        """
        Args:
            model_path:       Path to the trained .pt model.
            device:           'cuda' or 'cpu'.
            imgsz:            Static inference resolution (must match ONNX export).
            iou_threshold:    NMS IoU threshold.
            min_confidence:   Min confidence to accept a Fire detection.
            smoke_confidence: Higher bar for Smoke (noisier class).
        """
        self.logger = logging.getLogger(__name__)

        self.device = "cuda" if str(device).lower() in ("cuda", "0", "gpu") else "cpu"
        self.use_half = (self.device == "cuda")
        self.imgsz = imgsz
        self.iou_threshold = iou_threshold
        self.min_confidence = min_confidence
        self.smoke_confidence = smoke_confidence
        self.last_confidence = 0.0

        self.colors = {"fire": (0, 0, 255), "smoke": (128, 128, 128)}

        self.model = self._load_model(Path(model_path))
    # Fix ONNX/CPU model class names attribute mapping conflict
        if hasattr(self.model, "names"):
            self.names = self.model.names
        elif hasattr(self.model, "model") and hasattr(self.model.model, "names"):
            self.names = self.model.model.names
        else:
            # Default fallback classes for SFFS dataset (0: fire, 1: smoke) if metadata is missing
            self.names = {0: 'fire', 1: 'smoke'}

        self.logger.info(f"[Detector] Class names loaded: {self.names}")
        self.logger.info(
            f"[Detector] Ready | device={self.device} | "
            f"fp16={self.use_half} | imgsz={self.imgsz}"
        )

    # ------------------------------------------------------------------
    # Model loading (dual-mode)
    # ------------------------------------------------------------------
    def _load_model(self, pt_path: Path) -> YOLO:
        """Load the GPU (.pt FP16) or CPU (.onnx, else .pt) backend."""
        try:
            if self.device == "cuda":
                model = YOLO(str(pt_path))
                model.to("cuda")  # FP16 is applied at inference via half=True
                self.logger.info(f"[Detector] GPU model loaded: {pt_path.name}")
                return model

            # CPU path → prefer a pre-exported ONNX with static imgsz.
            onnx_path = self._resolve_onnx(pt_path)
            if onnx_path is not None:
                self.logger.info(f"[Detector] CPU ONNX model loaded: {onnx_path.name}")
                return YOLO(str(onnx_path), task="detect")

            self.logger.warning(
                f"[Detector] No .onnx found next to {pt_path.name}. Falling back to "
                f"the .pt model on CPU (slower). Export with:\n"
                f"    yolo export model={pt_path} format=onnx imgsz={self.imgsz} simplify=True"
            )
            return YOLO(str(pt_path))

        except Exception as e:
            self.logger.error(f"[Detector] FATAL: model load failed: {e}")
            raise

    def _resolve_onnx(self, pt_path: Path) -> Optional[Path]:
        """Find a sibling/sibling-dir ONNX export. Returns None if absent."""
        sibling = pt_path.with_suffix(".onnx")
        if sibling.exists():
            return sibling
        if pt_path.parent.exists():
            for candidate in pt_path.parent.glob("*.onnx"):
                return candidate
        return None

    # ------------------------------------------------------------------
    # Inference (no drawing, no mutation)
    # ------------------------------------------------------------------
    def detect(self, frame: np.ndarray) -> FireResult:
        """
        Run fire/smoke inference on a clean frame.
        Boxes are returned in the input frame's coordinate space.
        """
        try:
            results = self.model(
                frame,
                imgsz=self.imgsz,
                iou=self.iou_threshold,
                conf=self.min_confidence,
                device=self.device,
                half=self.use_half,
                verbose=False,
            )
        except Exception as e:
            self.logger.error(f"[Detector] Inference error: {e}")
            self.last_confidence = 0.0
            return EMPTY_RESULT

        boxes_data = results[0].boxes
        if boxes_data is None or len(boxes_data) == 0:
            self.last_confidence = 0.0
            return EMPTY_RESULT

        boxes = boxes_data.xyxy.cpu().numpy().astype(int)
        class_ids = boxes_data.cls.cpu().numpy().astype(int)
        confidences = boxes_data.conf.cpu().numpy()

        # Highest-confidence detection first.
        order = np.argsort(-confidences)

        label: Optional[str] = None
        label_conf = 0.0
        drawables: List[Tuple[np.ndarray, str, float]] = []

        for i in order:
            class_name = self.names[class_ids[i]]
            conf = float(confidences[i])
            drawables.append((boxes[i], class_name, conf))

            if label is None:
                cname = class_name.lower()
                if cname == "fire" and conf >= self.min_confidence:
                    label, label_conf = "Fire", conf
                elif cname == "smoke" and conf >= self.smoke_confidence:
                    label, label_conf = "Smoke", conf

        self.last_confidence = label_conf
        return FireResult(label=label, confidence=label_conf, drawables=drawables)

    # ------------------------------------------------------------------
    # Visualization (single final pass)
    # ------------------------------------------------------------------
    def draw(self, frame: np.ndarray, result: FireResult) -> None:
        """Render all detection boxes onto `frame` in place.
        The bottom status bar (Status / Conf / IoU) is intentionally omitted to
        keep the lower frame margin completely clean."""
        for box, class_name, conf in result.drawables:
            self._draw_box(frame, box, class_name, conf)

    def _draw_box(self, frame, box, class_name, confidence) -> None:
        x1, y1, x2, y2 = box
        color = self.colors.get(class_name.lower(), (0, 255, 0))
        text = f"{class_name}: {confidence:.2f}"

        label_height = 30
        text_y = (y2 + label_height) if y1 < label_height else (y1 - 5)

        # Translucent fill
        overlay = frame.copy()
        cv2.rectangle(overlay, (x1, y1), (x2, y2), color, -1)
        cv2.addWeighted(overlay, 0.2, frame, 0.8, 0, frame)

        cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2)

        # Accent corners
        cl, th = 20, 2
        cv2.line(frame, (x1, y1), (x1 + cl, y1), color, th)
        cv2.line(frame, (x1, y1), (x1, y1 + cl), color, th)
        cv2.line(frame, (x2, y1), (x2 - cl, y1), color, th)
        cv2.line(frame, (x2, y1), (x2, y1 + cl), color, th)
        cv2.line(frame, (x1, y2), (x1 + cl, y2), color, th)
        cv2.line(frame, (x1, y2), (x1, y2 - cl), color, th)
        cv2.line(frame, (x2, y2), (x2 - cl, y2), color, th)
        cv2.line(frame, (x2, y2), (x2, y2 - cl), color, th)

        cvzone.putTextRect(
            frame, text, (x1, text_y), scale=1.5, thickness=2,
            colorR=color, colorT=(255, 255, 255),
            font=cv2.FONT_HERSHEY_SIMPLEX, offset=5, border=2, colorB=(0, 0, 0),
        )

    def _add_frame_info(self, frame: np.ndarray, detection: Optional[str]) -> None:
        h, w = frame.shape[:2]
        bar_h = 40

        overlay = frame[h - bar_h:h, 0:w].copy()
        cv2.rectangle(frame, (0, h - bar_h), (w, h), (0, 0, 0), -1)
        cv2.addWeighted(overlay, 0.2, frame[h - bar_h:h, 0:w], 0.8, 0,
                        frame[h - bar_h:h, 0:w])

        status_text = f"Status: {detection if detection else 'No Detection'}"
        cv2.putText(frame, status_text, (10, h - 15),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)

        conf_text = f"Conf: {self.min_confidence:.2f} | IOU: {self.iou_threshold:.2f}"
        tw = cv2.getTextSize(conf_text, cv2.FONT_HERSHEY_SIMPLEX, 0.6, 2)[0][0]
        cv2.putText(frame, conf_text, (w - tw - 10, h - 15),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
