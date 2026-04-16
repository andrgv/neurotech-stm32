import json
import os
from pathlib import Path

# Single source of truth is config/constants.json
_config_path = Path(__file__).parent.parent / "config" / "constants.json"
_config = json.loads(_config_path.read_text())


def _env_truthy(name: str) -> bool:
    return os.environ.get(name, "").strip().lower() in ("1", "true", "yes")


### Mode
# JSON default; override with SIMULATE=1 or EEG_SIM=1 (legacy) for local/Docker without editing the file.
if "EEG_SIM" in os.environ:
    SIMULATE: bool = _env_truthy("EEG_SIM")
elif "SIMULATE" in os.environ:
    SIMULATE: bool = _env_truthy("SIMULATE")
else:
    SIMULATE: bool = bool(_config["SIMULATE"])

### Haptics
if "HAPTICS_ENABLE" in os.environ:
    HAPTICS_ENABLE: bool = _env_truthy("HAPTICS_ENABLE")
else:
    HAPTICS_ENABLE: bool = bool(_config.get("HAPTICS_ENABLE", False))

HAPTICS_DEVICE_NAME: str = os.environ.get(
    "HAPTICS_DEVICE_NAME", str(_config.get("HAPTICS_DEVICE_NAME", "NeuroHaptic"))
)
HAPTICS_SERVICE_UUID: str = os.environ.get(
    "HAPTICS_SERVICE_UUID",
    str(_config.get("HAPTICS_SERVICE_UUID", "19B10010-E8F2-537E-4F6C-D104768A1214")),
)
HAPTICS_CHARACTERISTIC_UUID: str = os.environ.get(
    "HAPTICS_CHARACTERISTIC_UUID",
    str(_config.get("HAPTICS_CHARACTERISTIC_UUID", "19B10011-E8F2-537E-4F6C-D104768A1214")),
)
HAPTICS_SCAN_TIMEOUT_SECONDS: float = float(
    os.environ.get(
        "HAPTICS_SCAN_TIMEOUT_SECONDS",
        str(_config.get("HAPTICS_SCAN_TIMEOUT_SECONDS", 6.0)),
    )
)
HAPTICS_SEND_MIN_INTERVAL_SECONDS: float = float(
    os.environ.get(
        "HAPTICS_SEND_MIN_INTERVAL_SECONDS",
        str(_config.get("HAPTICS_SEND_MIN_INTERVAL_SECONDS", 3.0)),
    )
)
HAPTICS_MIN_INTENSITY_DELTA: int = int(
    os.environ.get(
        "HAPTICS_MIN_INTENSITY_DELTA",
        str(_config.get("HAPTICS_MIN_INTENSITY_DELTA", 10)),
    )
)
HAPTICS_PRESET_CALM: int = int(
    os.environ.get("HAPTICS_PRESET_CALM", str(_config.get("HAPTICS_PRESET_CALM", 1)))
)
HAPTICS_PRESET_FOCUS: int = int(
    os.environ.get("HAPTICS_PRESET_FOCUS", str(_config.get("HAPTICS_PRESET_FOCUS", 2)))
)
HAPTICS_PRESET_DEEP_FOCUS: int = int(
    os.environ.get(
        "HAPTICS_PRESET_DEEP_FOCUS", str(_config.get("HAPTICS_PRESET_DEEP_FOCUS", 3))
    )
)
HAPTICS_PRESET_HYPE: int = int(
    os.environ.get("HAPTICS_PRESET_HYPE", str(_config.get("HAPTICS_PRESET_HYPE", 2)))
)

### Signal Processing
N_CHANNELS:       int = _config["N_CHANNELS"]
SAMPLE_RATE:      int = _config["SAMPLE_RATE"]
WINDOW_SIZE:      int = _config["WINDOW_SIZE"]

# Theta/beta mean ratio → focus in [0,1]; typical resting EEG often sits between these.
FOCUS_THETA_BETA_LOW: float = float(_config.get("FOCUS_THETA_BETA_LOW", 0.10))
FOCUS_THETA_BETA_HIGH: float = float(_config.get("FOCUS_THETA_BETA_HIGH", 0.42))
# Simulated-EEG demo: seconds per calm / focus / hype segment before rotating.
SIM_PHASE_SECONDS: float = float(_config.get("SIM_PHASE_SECONDS", 30))

### Spotify / neuro feature mapping (tune on real EEG recordings)
ENERGY_ALPHA_SUP_PERCENT_LOW: float = float(_config.get("ENERGY_ALPHA_SUP_PERCENT_LOW", -10))
ENERGY_ALPHA_SUP_PERCENT_HIGH: float = float(_config.get("ENERGY_ALPHA_SUP_PERCENT_HIGH", 45))
ENERGY_RAW_CLIP_LOW: float = float(_config.get("ENERGY_RAW_CLIP_LOW", -50))
ENERGY_RAW_CLIP_HIGH: float = float(_config.get("ENERGY_RAW_CLIP_HIGH", 100))
ENERGY_BLEND_ABSOLUTE_WEIGHT: float = float(_config.get("ENERGY_BLEND_ABSOLUTE_WEIGHT", 0.55))
ENERGY_HISTORY_MAX: int = int(_config.get("ENERGY_HISTORY_MAX", 96))
GAMMA_AROUSAL_WEIGHT: float = float(_config.get("GAMMA_AROUSAL_WEIGHT", 0.14))
ENERGY_SLOW_ALPHA: float = float(_config.get("ENERGY_SLOW_ALPHA", 0.045))
ENERGY_FAST_WEIGHT: float = float(_config.get("ENERGY_FAST_WEIGHT", 0.62))

### Mood buckets (2D energy × focus + d_energy); tune with real data
MOOD_HYPE_E_EFF_MIN: float = float(_config.get("MOOD_HYPE_E_EFF_MIN", 0.68))
MOOD_CALM_E_MAX: float = float(_config.get("MOOD_CALM_E_MAX", 0.33))
MOOD_CALM_F_MAX: float = float(_config.get("MOOD_CALM_F_MAX", 0.42))
MOOD_DEEP_FOCUS_E_MAX: float = float(_config.get("MOOD_DEEP_FOCUS_E_MAX", 0.43))
MOOD_DEEP_FOCUS_F_MIN: float = float(_config.get("MOOD_DEEP_FOCUS_F_MIN", 0.52))
MOOD_DISTRACT_HYPE_E_MIN: float = float(_config.get("MOOD_DISTRACT_HYPE_E_MIN", 0.52))
MOOD_DISTRACT_HYPE_F_MAX: float = float(_config.get("MOOD_DISTRACT_HYPE_F_MAX", 0.38))
MOOD_D_ENERGY_SCALE: float = float(_config.get("MOOD_D_ENERGY_SCALE", 0.85))

### BioSemi Hardware
# BIOSEMI_HOST can be overridden by environment variable (used by Docker)
BIOSEMI_HOST:     str = os.environ.get("BIOSEMI_HOST", _config["BIOSEMI_HOST"])
BIOSEMI_PORT:     int = _config["BIOSEMI_PORT"]
BYTES_PER_SAMPLE: int = _config["BYTES_PER_SAMPLE"]
WS_HOST:          str = _config["WS_HOST"]
WS_PORT:          int = _config["WS_PORT"]
DASHBOARD_PORT:   int = _config["DASHBOARD_PORT"]

### Spotify API
SPOTIFY_CLIENT_ID:      str = _config["SPOTIFY_CLIENT_ID"]
SPOTIFY_CLIENT_SECRET:  str = _config["SPOTIFY_CLIENT_SECRET"]
SPOTIFY_PLAYLIST_CALM:  str = _config["SPOTIFY_PLAYLIST_CALM"]
SPOTIFY_PLAYLIST_FOCUS: str = _config["SPOTIFY_PLAYLIST_FOCUS"]
SPOTIFY_PLAYLIST_HYPE:  str = _config["SPOTIFY_PLAYLIST_HYPE"]
# Optional extra contexts for ``deep_focus`` mood (comma-separated URIs). Empty = use code fallback to ``focus`` URIs only.
SPOTIFY_PLAYLIST_DEEP_FOCUS: str = str(_config.get("SPOTIFY_PLAYLIST_DEEP_FOCUS", "") or "")
