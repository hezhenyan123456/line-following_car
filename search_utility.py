# OpenMV H7 adaptive line tracking script
# Target: OpenMV H7 / H7 Plus
# Usage: copy to main.py or run from OpenMV IDE.

import sensor
import image
import time
from pyb import LED, UART

# ---------------- User tuning ----------------
# "dark" tracks black/dark tape on a brighter floor.
# "bright" tracks white/bright tape on a darker floor.
LINE_POLARITY = "dark"

FRAME_SIZE = sensor.QQVGA      # 160 x 120, fast and enough for line following
FRAME_W = 160
FRAME_H = 120

# UART output is required when OpenMV controls the MSPM0 car.
ENABLE_UART = True
UART_PORT = 3
UART_BAUD = 115200

# Leave the camera idle after reset so OpenMV IDE can connect reliably.
# This delay is intentionally before camera/UART setup.
IDE_CONNECT_DELAY_MS = 1000

# PID gains for steering error normalized to roughly -100..100.
KP = 0.85
KI = 0.00
KD = 0.18

# Dark-line thresholding.
CONTRAST_K = 0.55
MIN_THRESHOLD_GAP = 12
DARK_EXTRA_MARGIN = 8
BRIGHT_EXTRA_MARGIN = 4
MAX_DARK_THRESHOLD = 115

# Line-shape and edge-contrast filters. A real black tape line should be a
# compact dark stripe with brighter floor on both sides.
PIXELS_THRESHOLD = 35
AREA_THRESHOLD = 35
MERGE_MARGIN = 4
MIN_BLOB_DENSITY = 0.30
MIN_BLOB_WIDTH = 5
MAX_BLOB_WIDTH = 56
MIN_BLOB_HEIGHT = 5
MAX_BLOB_AREA_RATIO = 0.42
EDGE_SAMPLE_MARGIN = 5
EDGE_SAMPLE_WIDTH = 12
MIN_EDGE_CONTRAST = 14
MAX_LINE_MEAN = 108
MIN_VISIBLE_ROIS = 1
MAX_CENTER_DELTA = 60

# ROIs are (x, y, w, h, weight). Lower image rows are more important.
ROIS = (
    (0, 92, 160, 28, 0.55),
    (0, 62, 160, 24, 0.30),
    (0, 36, 160, 20, 0.15),
)

# ------------------------------------------------

red_led = LED(1)
green_led = LED(2)
blue_led = LED(3)

# Keep the USB/IDE connection window open after reset.
red_led.off()
green_led.off()
blue_led.on()
time.sleep_ms(IDE_CONNECT_DELAY_MS)
blue_led.off()

uart = None
if ENABLE_UART:
    uart = UART(UART_PORT, UART_BAUD, timeout_char=1000)


def clamp(value, low, high):
    if value < low:
        return low
    if value > high:
        return high
    return value


def value_of(obj, name):
    value = getattr(obj, name)
    if callable(value):
        return value()
    return value


def draw_center_mark(img, cx, cy, color=255):
    img.draw_line((cx - 5, cy, cx + 5, cy), color=color)
    img.draw_line((cx, cy - 5, cx, cy + 5), color=color)


def configure_camera():
    sensor.reset()
    sensor.set_pixformat(sensor.GRAYSCALE)
    sensor.set_framesize(FRAME_SIZE)
    sensor.set_hmirror(True)
    sensor.set_vflip(True)
    sensor.skip_frames(time=500)

    # Keep automatic exposure/gain enabled so the H7 can adapt when moving
    # between dim and bright areas. White balance is irrelevant in grayscale.
    sensor.set_auto_exposure(True)
    sensor.set_auto_gain(True)
    sensor.skip_frames(time=300)


def adaptive_threshold(img, roi):
    stats = img.get_statistics(roi=roi)
    mean = value_of(stats, "mean")
    stdev = value_of(stats, "stdev")

    # The threshold is computed from local ROI statistics instead of one fixed
    # number, which helps when the floor brightness changes across the scene.
    gap = max(MIN_THRESHOLD_GAP, int(stdev * CONTRAST_K))

    if LINE_POLARITY == "bright":
        low = clamp(int(mean + gap + BRIGHT_EXTRA_MARGIN), 0, 255)
        return [(low, 255)]

    high = clamp(int(mean - gap - DARK_EXTRA_MARGIN), 0, 255)
    high = min(high, MAX_DARK_THRESHOLD)
    return [(0, high)]


def roi_mean(img, roi):
    stats = img.get_statistics(roi=roi)
    return value_of(stats, "mean")


def blob_edge_contrast(img, blob, frame_roi):
    fx, fy, fw, fh = frame_roi
    bx = value_of(blob, "x")
    by = value_of(blob, "y")
    bw = value_of(blob, "w")
    bh = value_of(blob, "h")

    left_x = bx - EDGE_SAMPLE_MARGIN - EDGE_SAMPLE_WIDTH
    right_x = bx + bw + EDGE_SAMPLE_MARGIN

    if left_x < fx:
        return None
    if (right_x + EDGE_SAMPLE_WIDTH) > (fx + fw):
        return None

    core_mean = roi_mean(img, (bx, by, bw, bh))
    left_mean = roi_mean(img, (left_x, by, EDGE_SAMPLE_WIDTH, bh))
    right_mean = roi_mean(img, (right_x, by, EDGE_SAMPLE_WIDTH, bh))
    contrast = min(left_mean, right_mean) - core_mean

    return core_mean, contrast


def best_line_blob(img, roi):
    x, y, w, h, weight = roi
    thresholds = adaptive_threshold(img, (x, y, w, h))
    blobs = img.find_blobs(
        thresholds,
        roi=(x, y, w, h),
        pixels_threshold=PIXELS_THRESHOLD,
        area_threshold=AREA_THRESHOLD,
        merge=True,
        margin=MERGE_MARGIN,
    )

    if not blobs:
        return None

    # Prefer compact, dark blobs with bright neighbors on both sides.
    best = None
    best_score = -1
    for blob in blobs:
        blob_w = value_of(blob, "w")
        blob_h = value_of(blob, "h")
        blob_pixels = value_of(blob, "pixels")
        blob_area = value_of(blob, "area")
        density = value_of(blob, "density")

        if density < MIN_BLOB_DENSITY:
            continue
        if (blob_w < MIN_BLOB_WIDTH) or (blob_h < MIN_BLOB_HEIGHT):
            continue
        if blob_w > MAX_BLOB_WIDTH:
            continue
        if blob_area > int(w * h * MAX_BLOB_AREA_RATIO):
            continue

        edge_result = blob_edge_contrast(img, blob, (x, y, w, h))
        if edge_result is None:
            continue
        core_mean, edge_contrast = edge_result
        if core_mean > MAX_LINE_MEAN:
            continue
        if edge_contrast < MIN_EDGE_CONTRAST:
            continue

        density_score = int(density * 100)
        score = (
            blob_pixels +
            density_score * 3 +
            int(edge_contrast) * 6 -
            abs(blob_w - 18) * 2
        )
        if score > best_score:
            best = blob
            best_score = score

    return best


class PID:
    def __init__(self, kp, ki, kd):
        self.kp = kp
        self.ki = ki
        self.kd = kd
        self.integral = 0
        self.last_error = 0

    def update(self, error):
        self.integral = clamp(self.integral + error, -300, 300)
        derivative = error - self.last_error
        self.last_error = error
        return self.kp * error + self.ki * self.integral + self.kd * derivative


configure_camera()
clock = time.clock()
pid = PID(KP, KI, KD)
last_seen_ms = time.ticks_ms()
last_heartbeat_ms = time.ticks_ms()
last_print_ms = time.ticks_ms()
blue_state = False

while True:
    clock.tick()
    img = sensor.snapshot()
    now_ms = time.ticks_ms()

    if time.ticks_diff(now_ms, last_heartbeat_ms) >= 250:
        last_heartbeat_ms = now_ms
        blue_state = not blue_state
        if blue_state:
            blue_led.on()
        else:
            blue_led.off()

    weighted_sum = 0
    weight_sum = 0
    visible_count = 0
    min_cx = FRAME_W
    max_cx = 0

    for roi in ROIS:
        x, y, w, h, weight = roi
        blob = best_line_blob(img, roi)
        img.draw_rectangle((x, y, w, h), color=90)

        if blob:
            visible_count += 1
            cx = value_of(blob, "cx")
            cy = value_of(blob, "cy")
            weighted_sum += cx * weight
            weight_sum += weight
            img.draw_rectangle(value_of(blob, "rect"), color=255)
            draw_center_mark(img, cx, cy, color=255)
            if cx < min_cx:
                min_cx = cx
            if cx > max_cx:
                max_cx = cx

    centers_aligned = (max_cx - min_cx) <= MAX_CENTER_DELTA

    if (visible_count >= MIN_VISIBLE_ROIS) and centers_aligned:
        line_x = weighted_sum / weight_sum
        error = ((line_x - (FRAME_W / 2)) / (FRAME_W / 2)) * 100
        steering = clamp(int(pid.update(error)), -100, 100)
        last_seen_ms = now_ms

        green_led.on()
        red_led.off()

        # Keep the wire protocol minimal and compatible with the MSP parser.
        message = "L:1,E:{}\n".format(int(error))
    else:
        # When line is lost, keep the last derivative state but output a clear
        # lost-line command. Your motor controller can slow down or search.
        green_led.off()
        red_led.on()
        lost_time = time.ticks_diff(now_ms, last_seen_ms)
        message = "L:0,E:0\n"

    if uart:
        uart.write(message)

    # USB/IDE console output is diagnostic only. Throttle it so it cannot
    # monopolize the USB serial channel while UART stays real-time.
    if time.ticks_diff(now_ms, last_print_ms) >= 250:
        last_print_ms = now_ms
        print(message, "fps=", clock.fps())
