"""
SFFS Occupancy Tracker — People Counting via Entry/Exit Tracking
================================================================
Uses YOLOv11 with persistent ID tracking to count the number of
occupants inside a monitored zone. A configurable vertical line
separates "inside" from "outside"; crossing direction determines
entry vs exit.
"""

import cv2
from ultralytics import YOLO
import numpy as np
import logging


class OccupancyTracker:
    """
    Tracks people entering and exiting a monitored zone using
    YOLO persistent ID tracking and a virtual entry/exit boundary line.
    """

    SIDE_OUTSIDE = "outside"
    SIDE_INSIDE  = "inside"

    def __init__(self, model_path, line_x=300, confidence=0.5, device=0):
        """
        Initialize the occupancy tracker.

        Args:
            model_path:  Path to the YOLOv11 person detection model (.pt)
            line_x:      X-coordinate of the vertical entry/exit boundary line
            confidence:  Minimum detection confidence for person class
            device:      Inference device (0 = CUDA GPU, 'cpu' = CPU)
        """
        self.logger = logging.getLogger(__name__)

        # Load YOLO model and move to selected device
        self.model = YOLO(model_path)
        self.model.to(device)
        self.line_x = line_x
        self.confidence = confidence

        # Tracking State
        self.inside_count = 0
        self.person_side = {}       # track_id -> current side ("inside" / "outside")
        self.total_entries = 0      # Cumulative entry count (for analytics)
        self.total_exits = 0        # Cumulative exit count (for analytics)

        self.logger.info(
            f"[Occupancy] Tracker initialized | Line x={line_x} | "
            f"Device={'GPU' if device != 'cpu' else 'CPU'}"
        )

    def update(self, frame):
        """
        Process a single video frame: run person tracking and update occupancy.
        Draws bounding boxes, ID labels, and the entry/exit line on the frame.

        This method MUST be called on every frame to maintain tracking continuity.
        Skipping frames will cause the YOLO tracker to lose persistent IDs.

        Args:
            frame: BGR image (numpy array) from OpenCV

        Returns:
            tuple: (annotated_frame, inside_count)
        """
        # Run YOLO tracking for person class only (class 0 in COCO)
        results = self.model.track(
            frame,
            persist=True,       # Maintain track IDs across frames
            classes=[0],        # Only detect class 0 (person)
            conf=self.confidence,
            verbose=False
        )

        # Process tracked detections
        if results[0].boxes.id is not None:
            boxes     = results[0].boxes.xyxy.cpu().numpy()
            track_ids = results[0].boxes.id.cpu().numpy().astype(int)

            for box, tid in zip(boxes, track_ids):
                x1, y1, x2, y2 = map(int, box)
                center_x = (x1 + x2) // 2

                # Determine which side of the line the person is currently on
                current_side = (
                    self.SIDE_INSIDE if center_x > self.line_x
                    else self.SIDE_OUTSIDE
                )

                # First appearance: register their initial side without counting
                if tid not in self.person_side:
                    self.person_side[tid] = current_side
                else:
                    previous_side = self.person_side[tid]

                    # Crossing detected: side changed between consecutive frames
                    if previous_side != current_side:
                        if (previous_side == self.SIDE_OUTSIDE and
                                current_side == self.SIDE_INSIDE):
                            # Entering the monitored zone
                            self.inside_count += 1
                            self.total_entries += 1
                            self.logger.info(
                                f"[Occupancy] Person ID:{tid} ENTERED "
                                f"(inside: {self.inside_count})"
                            )

                        elif (previous_side == self.SIDE_INSIDE and
                              current_side == self.SIDE_OUTSIDE):
                            # Exiting the monitored zone (floor at 0)
                            self.inside_count = max(0, self.inside_count - 1)
                            self.total_exits += 1
                            self.logger.info(
                                f"[Occupancy] Person ID:{tid} EXITED "
                                f"(inside: {self.inside_count})"
                            )

                        # Update stored side after crossing
                        self.person_side[tid] = current_side

                # Draw person bounding box (green = inside, orange = outside)
                color = (
                    (0, 255, 0) if current_side == self.SIDE_INSIDE
                    else (255, 165, 0)
                )
                cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2)
                cv2.putText(
                    frame, f"ID:{tid}", (x1, y1 - 10),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2
                )

        # Draw the boundary line and HUD overlay
        self._draw_overlay(frame)

        return frame, self.inside_count

    def _draw_overlay(self, frame):
        """
        Draw the vertical entry/exit boundary line, side labels,
        and the current occupancy count on the frame.
        """
        h = frame.shape[0]

        # Vertical boundary line (red)
        cv2.line(frame, (self.line_x, 0), (self.line_x, h), (0, 0, 255), 2)

        # Side labels
        cv2.putText(
            frame, "OUTSIDE", (self.line_x - 120, 30),
            cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2
        )
        cv2.putText(
            frame, "INSIDE", (self.line_x + 20, 30),
            cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2
        )

        # Occupancy count display
        cv2.putText(
            frame, f"Inside: {self.inside_count}", (50, 50),
            cv2.FONT_HERSHEY_SIMPLEX, 1.2, (255, 255, 255), 3
        )

    # ==========================================
    # Public Getters
    # ==========================================

    def get_count(self):
        """Return the current number of people inside the monitored zone."""
        return self.inside_count

    def get_analytics(self):
        """Return cumulative entry/exit analytics."""
        return {
            "inside_count": self.inside_count,
            "total_entries": self.total_entries,
            "total_exits": self.total_exits,
            "tracked_ids": len(self.person_side)
        }

    def reset(self):
        """Reset all tracking state and counters."""
        self.inside_count = 0
        self.total_entries = 0
        self.total_exits = 0
        self.person_side.clear()
        self.logger.info("[Occupancy] Tracker state reset.")
