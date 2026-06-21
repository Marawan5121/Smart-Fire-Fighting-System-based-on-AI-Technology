import os
import cv2
from ultralytics import YOLO

base_dir = os.path.dirname(os.path.abspath(__file__))
model_path = os.path.join(base_dir, "best_nano_111.pt")
model = YOLO(model_path)

# --- التعديل الإلزامي لحل مشكلة الـ Timeout ---
# إجبار محرك FFmpeg على استخدام بروتوكول TCP المستقر لمنع سقوط الحزم
os.environ["OPENCV_FFMPEG_CAPTURE_OPTIONS"] = "rtsp_transport;tcp"
# -----------------------------------------------

# تأكد من كتابة الـ Verification Code (رمز التحقق) الحقيقي مكان XYZABC بأحرف كبيرة
rtsp_url = "rtsp://admin:LGZUCP@192.168.1.2:554/h264/ch1/sub/av_stream"
cap = cv2.VideoCapture(rtsp_url)

# تقليل زمن الانتظار الداخلي في OpenCV
cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)