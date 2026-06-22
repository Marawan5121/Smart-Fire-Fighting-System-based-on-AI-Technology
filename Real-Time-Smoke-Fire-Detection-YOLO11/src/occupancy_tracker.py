"""
SFFS Occupancy Tracker — People Counting & Face Recognition
================================================================
Uses YOLOv11 with persistent ID tracking to count the number of
occupants inside a monitored zone and face_recognition to identify names.
"""

import cv2
from ultralytics import YOLO
import numpy as np
import logging
import face_recognition
import os
from pathlib import Path


class OccupancyTracker:
    """
    Tracks people entering and exiting a monitored zone using
    YOLO persistent ID tracking, and recognizes face names from known_faces folder.
    """

    SIDE_OUTSIDE = "outside"
    SIDE_INSIDE  = "inside"

    def __init__(self, model_path, line_x=300, confidence=0.5, device="cpu"):
        """
        Initialize the occupancy tracker and load known faces.
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

        # --- Face Recognition Storage ---
        self.known_face_encodings = []
        self.known_face_names = []
        self.track_name_cache = {}  # Cache to remember names for track_ids to save CPU
        self._load_known_faces()

        self.logger.info(
            f"[Occupancy] Tracker initialized | Line x={line_x} | "
            f"Device={'GPU' if device != 'cpu' else 'CPU'}"
        )

    def _load_known_faces(self):
        """Load and encode face images from the known_faces directory."""
        # Adjust path to find known_faces folder relative to this file location
        faces_dir = Path(__file__).parent / "known_faces"
        
        if faces_dir.exists():
            for filename in os.listdir(faces_dir):
                if filename.lower().endswith((".jpg", ".png", ".jpeg")):
                    try:
                        img = face_recognition.load_image_file(str(faces_dir / filename))
                        encodings = face_recognition.face_encodings(img)
                        if encodings:
                            self.known_face_encodings.append(encodings[0])
                            name = os.path.splitext(filename)[0]
                            self.known_face_names.append(name)
                    except Exception as e:
                        self.logger.error(f"[Occupancy] Failed loading face {filename}: {e}")
            self.logger.info(f"[Occupancy] Loaded {len(self.known_face_names)} known faces: {self.known_face_names}")
        else:
            self.logger.warning(f"[Occupancy] 'known_faces' directory not found at {faces_dir.resolve()}")

    def update(self, frame):
        """
        Process a single video frame: run person tracking, face recognition, and update occupancy.
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

                # --- Dynamic Face Recognition Inside Bounding Box ---
                person_name = self.track_name_cache.get(tid, None)
                
                if person_name is None:
                    # Safely clamp bounding box coordinates within frame boundaries
                    h_max, w_max = frame.shape[:2]
                    x1_clamped = max(0, int(x1))
                    y1_clamped = max(0, int(y1))
                    x2_clamped = min(w_max, int(x2))
                    y2_clamped = min(h_max, int(y2))
                    
                    # Extract the cropped image for the person
                    person_crop = frame[y1_clamped:y2_clamped, x1_clamped:x2_clamped]
                    
                    # Verify the crop is valid and non-empty to prevent processing crashes
                    if person_crop.size > 0 and person_crop.shape[0] > 0 and person_crop.shape[1] > 0:
                        # Convert color channels and ensure standard 8-bit unsigned integer type configuration
                        rgb_crop = cv2.cvtColor(person_crop, cv2.COLOR_BGR2RGB)
                        rgb_crop = rgb_crop.astype(np.uint8)
                        
                        # Execute face location detection on the verified safe array structure
                        face_locations = face_recognition.face_locations(rgb_crop)
                        
                        if face_locations:
                            face_encodings = face_recognition.face_encodings(rgb_crop, face_locations)
                            for face_encoding in face_encodings:
                                matches = face_recognition.compare_faces(self.known_face_encodings, face_encoding, tolerance=0.6)
                                if True in matches:
                                    first_match_index = matches.index(True)
                                    person_name = self.known_face_names[first_match_index]
                                    self.track_name_cache[tid] = person_name  # Save to cache
                                    break
                    
                    if person_name is None:
                        person_name = f"ID:{tid}"  # Fallback to ID if unknown or face not clear yet

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
                                f"[Occupancy] {person_name} ENTERED "
                                f"(inside: {self.inside_count})"
                                )

                        elif (previous_side == self.SIDE_INSIDE and
                              current_side == self.SIDE_OUTSIDE):
                            # Exiting the monitored zone
                            self.inside_count = max(0, self.inside_count - 1)
                            self.total_exits += 1
                            self.logger.info(
                                f"[Occupancy] {person_name} EXITED "
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
                        frame, person_name, (x1, y1 - 10),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2
                    )

        # Draw the boundary line and HUD overlay
        self._draw_overlay(frame)

        return frame, self.inside_count

    def _draw_overlay(self, frame):
        """Draw overlay layout on the frame."""
        h = frame.shape[0]
        cv2.line(frame, (self.line_x, 0), (self.line_x, h), (0, 0, 255), 2)
        cv2.putText(frame, "OUTSIDE", (self.line_x - 120, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
        cv2.putText(frame, "INSIDE", (self.line_x + 20, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
        cv2.putText(frame, f"Inside: {self.inside_count}", (50, 50), cv2.FONT_HERSHEY_SIMPLEX, 1.2, (0, 0, 0), 3)  # Black for legibility on bright feeds

    def get_count(self):
        return self.inside_count

    def get_analytics(self):
        return {
            "inside_count": self.inside_count,
            "total_entries": self.total_entries,
            "total_exits": self.total_exits,
            "tracked_ids": len(self.person_side)
        }

    def reset(self):
        self.inside_count = 0
        self.total_entries = 0
        self.total_exits = 0
        self.person_side.clear()
        self.track_name_cache.clear()
        self.logger.info("[Occupancy] Tracker state reset.")