from __future__ import annotations

import argparse
import ctypes
from ctypes import wintypes
from pathlib import Path
import subprocess
import time

import cv2
import numpy as np
from PIL import ImageGrab


WM_CLOSE = 0x0010
WM_KEYDOWN = 0x0100
WM_KEYUP = 0x0101
WM_MOUSEMOVE = 0x0200
WM_LBUTTONDOWN = 0x0201
WM_LBUTTONUP = 0x0202
WM_MBUTTONDOWN = 0x0207
WM_MBUTTONUP = 0x0208
MK_LBUTTON = 0x0001
MK_MBUTTON = 0x0010
MOUSEEVENTF_WHEEL = 0x0800
SW_RESTORE = 9

user32 = ctypes.windll.user32
user32.SetProcessDPIAware()


class Point(ctypes.Structure):
    _fields_ = [("x", wintypes.LONG), ("y", wintypes.LONG)]


class Rect(ctypes.Structure):
    _fields_ = [
        ("left", wintypes.LONG),
        ("top", wintypes.LONG),
        ("right", wintypes.LONG),
        ("bottom", wintypes.LONG),
    ]


def long_parameter(x: int, y: int) -> int:
    return ((y & 0xFFFF) << 16) | (x & 0xFFFF)


def find_process_window(process_id: int, timeout_seconds: float = 10.0) -> int:
    callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    deadline = time.perf_counter() + timeout_seconds

    while time.perf_counter() < deadline:
        matching_windows: list[int] = []

        @callback_type
        def collect_window(window: int, _: int) -> bool:
            window_process_id = wintypes.DWORD()
            user32.GetWindowThreadProcessId(window, ctypes.byref(window_process_id))
            if window_process_id.value == process_id and user32.IsWindowVisible(window):
                matching_windows.append(window)
            return True

        user32.EnumWindows(collect_window, 0)
        if matching_windows:
            return matching_windows[0]
        time.sleep(0.1)

    raise RuntimeError("DentalViz did not create a visible window within 10 seconds.")


def client_to_screen(window: int, x: int, y: int) -> Point:
    point = Point(x, y)
    if not user32.ClientToScreen(window, ctypes.byref(point)):
        raise RuntimeError("Could not convert DentalViz client coordinates.")
    return point


def move_mouse(window: int, x: int, y: int, pressed_buttons: int = 0) -> None:
    screen_point = client_to_screen(window, x, y)
    user32.SetCursorPos(screen_point.x, screen_point.y)
    user32.SendMessageW(window, WM_MOUSEMOVE, pressed_buttons, long_parameter(x, y))


def click(window: int, x: int, y: int) -> None:
    move_mouse(window, x, y)
    position = long_parameter(x, y)
    user32.SendMessageW(window, WM_LBUTTONDOWN, MK_LBUTTON, position)
    time.sleep(0.08)
    user32.SendMessageW(window, WM_LBUTTONUP, 0, position)


def key_press(window: int, virtual_key: int) -> None:
    user32.SendMessageW(window, WM_KEYDOWN, virtual_key, 0)
    time.sleep(0.12)
    user32.SendMessageW(window, WM_KEYUP, virtual_key, 0)


def mouse_wheel(window: int, x: int, y: int, delta: int) -> None:
    user32.SetForegroundWindow(window)
    move_mouse(window, x, y)
    user32.mouse_event(MOUSEEVENTF_WHEEL, 0, 0, delta, 0)


def update_drag(
    window: int,
    elapsed: float,
    start_time: float,
    end_time: float,
    start: tuple[int, int],
    end: tuple[int, int],
    button_down: int,
    button_up: int,
    button_mask: int,
    state: dict[str, bool],
    name: str,
) -> None:
    if start_time <= elapsed < end_time:
        if not state.get(f"{name}_started"):
            move_mouse(window, *start)
            user32.SendMessageW(
                window, button_down, button_mask, long_parameter(*start)
            )
            state[f"{name}_started"] = True
        progress = (elapsed - start_time) / (end_time - start_time)
        x = round(start[0] + (end[0] - start[0]) * progress)
        y = round(start[1] + (end[1] - start[1]) * progress)
        move_mouse(window, x, y, button_mask)
    elif elapsed >= end_time and state.get(f"{name}_started") and not state.get(
        f"{name}_ended"
    ):
        move_mouse(window, *end, pressed_buttons=button_mask)
        user32.SendMessageW(window, button_up, 0, long_parameter(*end))
        state[f"{name}_ended"] = True


def caption_for_time(elapsed: float) -> str:
    if elapsed < 4.0:
        return "DentalViz | C++20 + OpenGL 3.3 Core"
    if elapsed < 10.0:
        return "Orbit camera keeps the dental mesh centered"
    if elapsed < 16.0:
        return "Pan and zoom use the same DPI-aware viewer coordinates"
    if elapsed < 18.0:
        return "F restores a bounds-based fitted view"
    if elapsed < 22.0:
        return "Wireframe exposes the indexed triangle topology"
    if elapsed < 26.0:
        return "Normal Color validates surface orientation"
    if elapsed < 30.0:
        return "Blinn-Phong solid rendering"
    if elapsed < 34.0:
        return "Ray picking selects the closest mesh triangle"
    if elapsed < 39.0:
        return "Two surface points produce a 3D straight-line distance"
    if elapsed < 45.0:
        return "Model-space Clipping Preview updates immediately"
    return "Camera movement does not move the clipping plane"


def add_caption(frame: np.ndarray, caption: str) -> np.ndarray:
    overlay = frame.copy()
    height, width = frame.shape[:2]
    top = height - 58
    cv2.rectangle(overlay, (0, top), (width, height), (4, 12, 16), -1)
    cv2.addWeighted(overlay, 0.82, frame, 0.18, 0.0, frame)
    cv2.putText(
        frame,
        caption,
        (24, height - 21),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.72,
        (224, 246, 248),
        2,
        cv2.LINE_AA,
    )
    return frame


def capture_demo(executable: Path, output_path: Path) -> None:
    duration_seconds = 50.0
    frames_per_second = 12.0
    output_path.parent.mkdir(parents=True, exist_ok=True)

    viewer_process = subprocess.Popen(
        [str(executable), "--smoke-seconds", "65"], cwd=executable.parent
    )
    writer: cv2.VideoWriter | None = None
    try:
        window = find_process_window(viewer_process.pid)
        user32.ShowWindow(window, SW_RESTORE)
        user32.SetForegroundWindow(window)
        time.sleep(1.0)

        client_rectangle = Rect()
        if not user32.GetClientRect(window, ctypes.byref(client_rectangle)):
            raise RuntimeError("Could not read the DentalViz client rectangle.")
        client_origin = client_to_screen(window, 0, 0)
        width = client_rectangle.right - client_rectangle.left
        height = client_rectangle.bottom - client_rectangle.top
        writer = cv2.VideoWriter(
            str(output_path),
            cv2.VideoWriter_fourcc(*"mp4v"),
            frames_per_second,
            (width, height),
        )
        if not writer.isOpened():
            raise RuntimeError("OpenCV could not open the MP4 video writer.")

        state: dict[str, bool] = {}
        actions = [
            (13.2, "zoom_one", lambda: mouse_wheel(window, 830, 350, 120)),
            (13.8, "zoom_two", lambda: mouse_wheel(window, 830, 350, 120)),
            (16.0, "fit", lambda: key_press(window, 0x46)),
            (18.0, "wireframe", lambda: key_press(window, 0x32)),
            (22.0, "normals", lambda: key_press(window, 0x33)),
            (26.0, "solid", lambda: key_press(window, 0x31)),
            (30.0, "point_a", lambda: click(window, 830, 250)),
            (34.0, "point_b", lambda: click(window, 860, 450)),
            (39.0, "reset_measurement", lambda: click(window, 190, 604)),
            (39.5, "clipping_tab", lambda: click(window, 172, 485)),
            (39.8, "clipping_tab_again", lambda: click(window, 172, 485)),
            (40.3, "enable_clipping", lambda: click(window, 31, 517)),
            (42.0, "clip_distance", lambda: click(window, 255, 581)),
        ]

        start_time = time.perf_counter()
        frame_count = round(duration_seconds * frames_per_second)
        for frame_index in range(frame_count):
            target_time = start_time + frame_index / frames_per_second
            remaining = target_time - time.perf_counter()
            if remaining > 0:
                time.sleep(remaining)
            elapsed = time.perf_counter() - start_time

            update_drag(
                window,
                elapsed,
                4.0,
                9.0,
                (760, 350),
                (900, 410),
                WM_LBUTTONDOWN,
                WM_LBUTTONUP,
                MK_LBUTTON,
                state,
                "orbit",
            )
            update_drag(
                window,
                elapsed,
                10.0,
                13.0,
                (820, 360),
                (875, 390),
                WM_MBUTTONDOWN,
                WM_MBUTTONUP,
                MK_MBUTTON,
                state,
                "pan",
            )
            update_drag(
                window,
                elapsed,
                45.0,
                48.0,
                (790, 350),
                (900, 395),
                WM_LBUTTONDOWN,
                WM_LBUTTONUP,
                MK_LBUTTON,
                state,
                "clipped_orbit",
            )

            for action_time, action_name, action in actions:
                if elapsed >= action_time and not state.get(action_name):
                    action()
                    state[action_name] = True

            image = ImageGrab.grab(
                bbox=(
                    client_origin.x,
                    client_origin.y,
                    client_origin.x + width,
                    client_origin.y + height,
                ),
                all_screens=True,
            )
            frame = cv2.cvtColor(np.asarray(image), cv2.COLOR_RGB2BGR)
            writer.write(add_caption(frame, caption_for_time(elapsed)))
    finally:
        if writer is not None:
            writer.release()
        if viewer_process.poll() is None:
            try:
                user32.SendMessageW(window, WM_CLOSE, 0, 0)
            except UnboundLocalError:
                viewer_process.terminate()
            try:
                viewer_process.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                viewer_process.kill()
                viewer_process.wait()

    if viewer_process.returncode != 0:
        raise RuntimeError(f"DentalViz exited with code {viewer_process.returncode}.")
    print(f"Demo: {output_path}")
    print(f"Duration: {duration_seconds:.0f} seconds")
    print(f"Frames: {round(duration_seconds * frames_per_second)}")


def parse_arguments() -> argparse.Namespace:
    repository_root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description="Capture the DentalViz P0 viewer demo.")
    parser.add_argument(
        "--executable",
        type=Path,
        default=repository_root / "out" / "build" / "msvc" / "Release" / "DentalViz.exe",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=repository_root
        / "docs"
        / "demo"
        / "DentalViz-v0.5-viewer-demo.mp4",
    )
    return parser.parse_args()


if __name__ == "__main__":
    arguments = parse_arguments()
    if not arguments.executable.is_file():
        raise FileNotFoundError(f"DentalViz executable was not found: {arguments.executable}")
    capture_demo(arguments.executable.resolve(), arguments.output.resolve())
