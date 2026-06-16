import os
from dotenv import load_dotenv
import logging
from pathlib import Path

PROJECT_ROOT = Path(__file__).parent.parent
ENV = PROJECT_ROOT / '.env'
load_dotenv(ENV, override=True)


def setup_logging():
    log_dir = PROJECT_ROOT / 'logs'
    log_dir.mkdir(exist_ok=True)

    logging.basicConfig(
        filename=log_dir / 'fire_detection.log',
        level=logging.INFO,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        datefmt='%Y-%m-%d %H:%M:%S'
    )

    # Also log to console
    console_handler = logging.StreamHandler()
    console_handler.setLevel(logging.INFO)
    formatter = logging.Formatter(
        '%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    console_handler.setFormatter(formatter)
    logging.getLogger().addHandler(console_handler)

def _resolve_model_path(project_root):
    # 1. Check the default expected path
    default_path = project_root / 'models' / 'best_nano_111.pt'
    if default_path.exists():
        return default_path

    # 2. Search for any custom .pt file in the models/ directory (excluding base person tracker)
    models_dir = project_root / 'models'
    if models_dir.exists():
        for path in models_dir.glob('**/*.pt'):
            if path.name != 'yolo11n.pt':
                return path

    # 3. Recursively search the project root directory, ignoring common build/cache folders
    ignore_dirs = {'.git', '.venv', 'env', 'venv', 'runs', '__pycache__', 'logs', 'detected_fires'}
    for root, dirs, files in os.walk(project_root):
        dirs[:] = [d for d in dirs if d not in ignore_dirs]
        for f in files:
            if f.endswith('.pt') and f != 'yolo11n.pt':
                if any(kw in f.lower() for kw in ['best', 'fire', 'smoke', 'nano']):
                    return Path(root) / f

    # 4. Fallback to the default path if nothing is found
    return default_path


def _get_env(name, default=None):
    val = os.getenv(name, default)
    if val is not None and isinstance(val, str):
        val = val.split('#')[0].strip()
    return val


class Config:
    PROJECT_ROOT = Path(__file__).parent.parent
    MODEL_PATH = _resolve_model_path(PROJECT_ROOT)
    VIDEO_SOURCE = PROJECT_ROOT / 'data' / 'police_car_fire_ccvt.mp4'
    DETECTED_FIRES_DIR = PROJECT_ROOT / 'detected_fires'

    ALERT_COOLDOWN = 45  # Seconds between alerts

    # MQTT Broker Settings (Mosquitto on localhost)
    MQTT_BROKER_HOST = _get_env('MQTT_BROKER_HOST', 'localhost')
    MQTT_BROKER_PORT = int(_get_env('MQTT_BROKER_PORT', '1883'))

    # Integrated System Settings
    VIDEO_SOURCE_ID = int(_get_env('VIDEO_SOURCE_ID', '0'))  # 0 = default webcam
    OCCUPANCY_LINE_X = int(_get_env('OCCUPANCY_LINE_X', '300'))  # Entry/exit boundary

    @classmethod
    def validate(cls):
        missing_vars = []
        for var in cls.__dict__:
            if not var.startswith('__') and getattr(cls, var) is None:
                missing_vars.append(var)

        if missing_vars:
            raise ValueError(
                f"Missing environment variables: {', '.join(missing_vars)}")

        # Create necessary directories
        cls.DETECTED_FIRES_DIR.mkdir(exist_ok=True)

        # Verify model file exists
        if not cls.MODEL_PATH.exists():
            raise FileNotFoundError(
                f"YOLO model file not found! Checked default and project folders. "
                f"Please ensure your Custom Model (.pt file) is in the project directory."
            )
        else:
            print(f"[Config] Resolved YOLO model path: {cls.MODEL_PATH.resolve()}")

        if not cls.VIDEO_SOURCE.exists():
            raise FileNotFoundError(
                f"Video source missing: {cls.VIDEO_SOURCE}")
