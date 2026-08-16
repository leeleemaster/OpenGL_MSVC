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
INPUT_KEYBOARD = 1
KEYEVENTF_KEYUP = 0x0002
KEYEVENTF_UNICODE = 0x0004
VK_CONTROL = 0x11
VK_A = 0x41

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


class KeyboardInput(ctypes.Structure):
    _fields_ = [
        ("virtual_key", wintypes.WORD),
        ("scan_code", wintypes.WORD),
        ("flags", wintypes.DWORD),
        ("time", wintypes.DWORD),
        ("extra_info", wintypes.WPARAM),
    ]


class MouseInput(ctypes.Structure):
    _fields_ = [
        ("x", wintypes.LONG),
        ("y", wintypes.LONG),
        ("mouse_data", wintypes.DWORD),
        ("flags", wintypes.DWORD),
        ("time", wintypes.DWORD),
        ("extra_info", wintypes.WPARAM),
    ]


class HardwareInput(ctypes.Structure):
    _fields_ = [
        ("message", wintypes.DWORD),
        ("parameter_low", wintypes.WORD),
        ("parameter_high", wintypes.WORD),
    ]


class InputValue(ctypes.Union):
    _fields_ = [
        ("keyboard", KeyboardInput),
        ("mouse", MouseInput),
        ("hardware", HardwareInput),
    ]


class Input(ctypes.Structure):
    _anonymous_ = ("value",)
    _fields_ = [("type", wintypes.DWORD), ("value", InputValue)]


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


def keyboard_input(
    virtual_key: int = 0,
    scan_code: int = 0,
    flags: int = 0,
) -> Input:
    result = Input()
    result.type = INPUT_KEYBOARD
    result.keyboard = KeyboardInput(virtual_key, scan_code, flags, 0, 0)
    return result


def send_inputs(inputs: list[Input]) -> None:
    input_array = (Input * len(inputs))(*inputs)
    sent = user32.SendInput(len(input_array), input_array, ctypes.sizeof(Input))
    if sent != len(input_array):
        raise RuntimeError(f"Windows SendInput sent {sent} of {len(input_array)} events.")


def set_editor_text(window: int, text: str) -> None:
    click(window, 170, 645)
    user32.SetForegroundWindow(window)
    time.sleep(0.2)
    send_inputs(
        [
            keyboard_input(virtual_key=VK_CONTROL),
            keyboard_input(virtual_key=VK_A),
            keyboard_input(virtual_key=VK_A, flags=KEYEVENTF_KEYUP),
            keyboard_input(virtual_key=VK_CONTROL, flags=KEYEVENTF_KEYUP),
        ]
    )
    unicode_inputs: list[Input] = []
    for character in text:
        unicode_inputs.append(
            keyboard_input(scan_code=ord(character), flags=KEYEVENTF_UNICODE)
        )
        unicode_inputs.append(
            keyboard_input(
                scan_code=ord(character),
                flags=KEYEVENTF_UNICODE | KEYEVENTF_KEYUP,
            )
        )
    send_inputs(unicode_inputs)
    time.sleep(0.4)


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
    if elapsed < 50.0:
        return "Camera movement does not move the clipping plane"
    if elapsed < 57.0:
        return "MiniShader compiles a bounded material source at runtime"
    if elapsed < 66.0:
        return "Edit one expression, then explicitly Compile & Apply"
    if elapsed < 75.0:
        return "Invalid source reports an error; Last Known Good stays active"
    return "Context-free compiler core | Move-only OpenGL resources | 63 tests"


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
    duration_seconds = 84.0
    frames_per_second = 12.0
    output_path.parent.mkdir(parents=True, exist_ok=True)

    viewer_process = subprocess.Popen(
        [str(executable), "--smoke-seconds", "100"],
        cwd=executable.parent,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    writer: cv2.VideoWriter | None = None
    try:
        window = find_process_window(viewer_process.pid)
        user32.ShowWindow(window, SW_RESTORE)
        user32.BringWindowToTop(window)
        foreground_deadline = time.perf_counter() + 5.0
        while user32.GetForegroundWindow() != window:
            user32.SetForegroundWindow(window)
            if time.perf_counter() >= foreground_deadline:
                raise RuntimeError("DentalViz could not become the foreground window.")
            time.sleep(0.1)
        time.sleep(2.0)

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
            (49.0, "disable_clipping", lambda: click(window, 31, 517)),
            (49.4, "fit_after_clipping", lambda: key_press(window, 0x46)),
            (50.0, "minishader_tab", lambda: click(window, 277, 485)),
            (50.8, "minishader_tab_again", lambda: click(window, 270, 485)),
            (51.6, "minishader_tab_final", lambda: click(window, 270, 485)),
            (52.8, "default_compile", lambda: click(window, 110, 570)),
            (
                58.0,
                "modified_source",
                lambda: set_editor_text(
                    window,
                    "material Dental { let n = normalize(normal); let l = normalize(lightDir); "
                    "let diffuse = max(dot(n, l), 0.0); let intensity = 0.55 + diffuse; "
                    "output = baseColor * intensity; }",
                ),
            ),
            (61.5, "modified_compile", lambda: click(window, 110, 570)),
            (
                66.5,
                "invalid_source",
                lambda: set_editor_text(
                    window,
                    "material Broken { output = missing; }",
                ),
            ),
            (69.5, "invalid_compile", lambda: click(window, 110, 570)),
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
    standard_output, standard_error = viewer_process.communicate()
    success_count = standard_output.count("MiniShader revision")
    if success_count != 2:
        raise RuntimeError(
            f"Expected two successful MiniShader applies, found {success_count}."
        )
    if "Unknown identifier: missing." not in standard_error:
        raise RuntimeError("The invalid MiniShader source did not report its semantic error.")
    print(f"Demo: {output_path}")
    print(f"Duration: {duration_seconds:.0f} seconds")
    print(f"Frames: {round(duration_seconds * frames_per_second)}")
    print("MiniShader: two applies and one Last Known Good rejection verified")


def parse_arguments() -> argparse.Namespace:
    repository_root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description="Capture the final DentalViz portfolio demo.")
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
        / "DentalViz-v0.8-portfolio-demo.mp4",
    )
    return parser.parse_args()


if __name__ == "__main__":
    arguments = parse_arguments()
    if not arguments.executable.is_file():
        raise FileNotFoundError(f"DentalViz executable was not found: {arguments.executable}")
    capture_demo(arguments.executable.resolve(), arguments.output.resolve())
