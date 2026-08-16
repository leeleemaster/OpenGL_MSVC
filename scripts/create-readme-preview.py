from __future__ import annotations

import argparse
from pathlib import Path

import cv2
from PIL import Image


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Create the compact DentalViz README GIF.")
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--start", type=float, default=18.0)
    parser.add_argument("--end", type=float, default=38.0)
    parser.add_argument("--fps", type=float, default=6.0)
    parser.add_argument("--width", type=int, default=720)
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    if arguments.start < 0.0 or arguments.end <= arguments.start:
        raise ValueError("Preview time range must be positive and ordered.")
    if arguments.fps <= 0.0 or arguments.width <= 0:
        raise ValueError("Preview FPS and width must be positive.")

    capture = cv2.VideoCapture(str(arguments.input))
    if not capture.isOpened():
        raise RuntimeError(f"Could not open demo video: {arguments.input}")

    frames: list[Image.Image] = []
    sample_time = arguments.start
    try:
        while sample_time < arguments.end:
            capture.set(cv2.CAP_PROP_POS_MSEC, sample_time * 1000.0)
            succeeded, frame = capture.read()
            if not succeeded:
                raise RuntimeError(f"Could not decode demo frame at {sample_time:.3f}s.")

            height, width = frame.shape[:2]
            output_height = max(1, round(height * arguments.width / width))
            resized = cv2.resize(
                frame,
                (arguments.width, output_height),
                interpolation=cv2.INTER_AREA,
            )
            rgb = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB)
            frames.append(
                Image.fromarray(rgb).quantize(
                    colors=96,
                    method=Image.Quantize.MEDIANCUT,
                    dither=Image.Dither.FLOYDSTEINBERG,
                )
            )
            sample_time += 1.0 / arguments.fps
    finally:
        capture.release()

    if not frames:
        raise RuntimeError("Preview did not contain any frames.")

    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    frame_duration = round(1000.0 / arguments.fps)
    frames[0].save(
        arguments.output,
        save_all=True,
        append_images=frames[1:],
        duration=frame_duration,
        loop=0,
        disposal=2,
        optimize=True,
    )
    print(f"README preview: {arguments.output} ({len(frames)} frames)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
