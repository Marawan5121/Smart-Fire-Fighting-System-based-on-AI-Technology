import cv2
from ultralytics import YOLO
import numpy as np

# --- إعدادات ---
model = YOLO("yolo11n.pt")  # ممكن تغيره للنموذج بتاعك
cap = cv2.VideoCapture(0)   # أو مسار فيديو
line_y = 300                # الإحداثي y للخط اللي هيتحسب عنده العبور
counted_ids = set()         # عشان الشخص ما يتعدش أكتر من مرة
total_count = 0

# --- تشغيل التتبع والعد ---
while cap.isOpened():
    success, frame = cap.read()
    if not success:
        break

    # تتبع الأشخاص فقط (class 0)
    results = model.track(frame, persist=True, classes=[0], conf=0.5)

    if results[0].boxes.id is not None:
        boxes = results[0].boxes.xyxy.cpu().numpy()
        track_ids = results[0].boxes.id.cpu().numpy().astype(int)

        for box, track_id in zip(boxes, track_ids):
            x1, y1, x2, y2 = map(int, box)
            center_y = (y1 + y2) // 2

            # رسم المربع والمعرف
            cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
            cv2.putText(frame, f"ID: {track_id}", (x1, y1-10),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0,255,0), 2)

            # آلية العد: لو مركز الشخص عبر الخط من فوق لتحت
            if center_y < line_y and track_id not in counted_ids:
                counted_ids.add(track_id)
                total_count += 1

    # رسم خط العد
    cv2.line(frame, (0, line_y), (frame.shape[1], line_y), (0, 0, 255), 2)
    cv2.putText(frame, f"Count: {total_count}", (50, 50),
                cv2.FONT_HERSHEY_SIMPLEX, 1, (0,0,255), 2)

    cv2.imshow("People Counter", frame)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()