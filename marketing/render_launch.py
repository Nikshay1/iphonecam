#!/usr/bin/env python3
"""Render the launch film from vector motion graphics and the public demo panel.

Requires Python's cairo module and FFmpeg. No browser, network, camera, or
live desktop capture is used. The panel interaction is an illustrative demo.
"""
from __future__ import annotations

import argparse
from array import array
import json
import math
from pathlib import Path
import random
import subprocess
import wave

import cairo

ROOT = Path(__file__).resolve().parents[1]
WIDTH, HEIGHT, FPS, DURATION = 1080, 1350, 30, 26
BG, INK, MUTED, ACCENT = '#0c0b0c', '#fafcfb', '#93918f', '#e4c6a1'
FONT = 'JetBrains Mono Nerd Font'
BOLD = cairo.FONT_WEIGHT_BOLD
HERO = cairo.ImageSurface.create_from_png(str(ROOT / 'marketing/assets/cinematic-hardware.png'))


def rgb(value):
    return tuple(int(value[i:i + 2], 16) / 255 for i in (1, 3, 5))


def color(ctx, value, alpha=1):
    ctx.set_source_rgba(*rgb(value), alpha)


def clamp(v):
    return max(0., min(1., v))


def ease(v):
    return 1 - (1 - clamp(v)) ** 4


def smooth(v):
    v = clamp(v)
    return v * v * (3 - 2 * v)


def rect(ctx, x, y, w, h, fill=None, stroke=None, line=1, alpha=1):
    ctx.rectangle(x, y, w, h)
    if fill:
        color(ctx, fill, alpha)
        ctx.fill_preserve()
    if stroke:
        color(ctx, stroke, alpha)
        ctx.set_line_width(line)
        ctx.stroke()
    else:
        ctx.new_path()


def line(ctx, points, value=INK, width=2, alpha=1):
    ctx.move_to(*points[0])
    for point in points[1:]:
        ctx.line_to(*point)
    color(ctx, value, alpha)
    ctx.set_line_width(width)
    ctx.set_line_cap(cairo.LINE_CAP_SQUARE)
    ctx.stroke()


def circle(ctx, x, y, radius, value=INK, fill=False, width=2, alpha=1):
    ctx.arc(x, y, radius, 0, math.tau)
    color(ctx, value, alpha)
    if fill:
        ctx.fill()
    else:
        ctx.set_line_width(width)
        ctx.stroke()


def text(ctx, value, x, y, size=32, ink=INK, bold=False, alpha=1, max_width=None):
    ctx.select_font_face(FONT, cairo.FONT_SLANT_NORMAL, BOLD if bold else cairo.FONT_WEIGHT_NORMAL)
    ctx.set_font_size(size)
    extent = ctx.text_extents(value)
    if max_width and extent.x_advance > max_width:
        ctx.set_font_size(size * max_width / extent.x_advance)
    color(ctx, ink, alpha)
    ctx.move_to(x, y)
    ctx.show_text(value)


def entrance(ctx, value, x, y, t, delay, size, ink=INK, bold=False):
    amount = ease((t - delay) / .65)
    text(ctx, value, x, y + (1 - amount) * 42, size, ink, bold, amount, WIDTH - x - 64)


def camera(ctx, x, y, size, value=INK):
    ctx.save()
    ctx.translate(x, y)
    ctx.scale(size / 48, size / 48)
    rect(ctx, 3, 12, 42, 29, stroke=value, line=3)
    line(ctx, [(12, 12), (16, 6), (30, 6), (34, 12)], value, 3)
    circle(ctx, 24, 26, 8, value, width=3)
    ctx.restore()


def backdrop(ctx, t):
    color(ctx, BG)
    ctx.paint()
    # Quiet technical grid; the native panel remains the visual focus.
    for x in range(72, WIDTH - 40, 78):
        line(ctx, [(x, 160), (x, HEIGHT - 120)], INK, 1, .025)
    for y in range(190, HEIGHT - 90, 78):
        line(ctx, [(72, y), (WIDTH - 72, y)], INK, 1, .025)


def chrome(ctx, t):
    camera(ctx, 72, 68, 32)
    text(ctx, 'iphonecam / omarchy', 120, 95, 22, MUTED)
    text(ctx, 'OPEN SOURCE', 820, 95, 19, ACCENT)
    line(ctx, [(72, 124), (1008, 124)], INK, 1, .15)
    line(ctx, [(72, 1290), (1008, 1290)], INK, 2, .15)
    line(ctx, [(72, 1290), (72 + 936 * clamp(t / DURATION), 1290)], ACCENT, 3)


def hero_plate(ctx, local, closer=False):
    # A generated studio photograph is used as an illustrative hardware plate.
    # Gentle camera movement and exposure reveal are composited for the film.
    base = max(WIDTH / HERO.get_width(), HEIGHT / HERO.get_height())
    zoom = (1.10 if closer else 1.0) + .045 * smooth(local / 4)
    scale = base * zoom
    ctx.save()
    ctx.translate((WIDTH - HERO.get_width() * scale) / 2, HEIGHT - HERO.get_height() * scale)
    ctx.scale(scale, scale)
    ctx.set_source_surface(HERO, 0, 0)
    ctx.get_source().set_filter(cairo.FILTER_BEST)
    ctx.paint_with_alpha(.55 + .45 * ease(local / 1.8))
    ctx.restore()
    gradient = cairo.LinearGradient(0, 1090, 0, HEIGHT)
    gradient.add_color_stop_rgba(0, 0, 0, 0, 0)
    gradient.add_color_stop_rgba(1, 0, 0, 0, .92)
    ctx.set_source(gradient)
    ctx.rectangle(0, 1090, WIDTH, HEIGHT - 1090)
    ctx.fill()


def intro(ctx, local, panel):
    hero_plate(ctx, local)
    text(ctx, 'YOUR NEXT WEBCAM IS ALREADY IN YOUR POCKET.', 72, 223, 24, MUTED, max_width=936)
    entrance(ctx, 'Your iPhone.', 72, 366, local, .08, 94, bold=True)
    entrance(ctx, 'Now a webcam.', 72, 478, local, .3, 94, ACCENT, True)
    entrance(ctx, 'Built for Omarchy. Connected by USB.', 72, 1225, local, .8, 29, INK)


def connect(ctx, local, panel):
    hero_plate(ctx, local, closer=True)
    entrance(ctx, 'Plug in.', 72, 300, local, 0, 92, bold=True)
    entrance(ctx, 'Take control.', 72, 410, local, .2, 92, ACCENT, True)
    amount = ease((local - .5) / .8)
    rect(ctx, 480, 485, 528 * amount, 1, ACCENT, alpha=amount)
    text(ctx, 'USB / NATIVE CONTROLS', 480, 535, 26, MUTED, alpha=amount)
    text(ctx, 'DroidCam on iPhone. Our app on Linux.', 72, 1225, 27, INK)


def cursor(ctx, x, y, scale=1):
    ctx.save()
    ctx.translate(x, y)
    ctx.scale(scale, scale)
    ctx.move_to(0, 0)
    for point in [(0, 31), (8, 23), (15, 40), (22, 36), (15, 20), (28, 20), (0, 0)]:
        ctx.line_to(*point)
    color(ctx, INK)
    ctx.fill_preserve()
    color(ctx, BG)
    ctx.set_line_width(2)
    ctx.stroke()
    ctx.restore()


def product(ctx, local, panel):
    entrance(ctx, 'Right in your bar.', 72, 240, local, 0, 70, bold=True)
    entrance(ctx, 'The Omarchy panel you already know.', 72, 295, local, .15, 27, MUTED)
    zoom = .94 + .06 * ease(local / 1.2)
    panel_w = 820 * zoom * (1 - .065 * ease((local - 3.7) / .5))
    scale = panel_w / panel.get_width()
    x, y = (WIDTH - panel_w) / 2, 345 + 24 * (1 - ease(local / .8))
    ctx.save()
    ctx.translate(x, y)
    ctx.scale(scale, scale)
    ctx.set_source_surface(panel, 0, 0)
    ctx.get_source().set_filter(cairo.FILTER_BEST)
    ctx.paint_with_alpha(ease(local / .6))
    if local >= 2.2:
        # An explicit UI demo animates only the public native panel controls.
        rect(ctx, 70, 46, 320, 22, BG)
        text(ctx, 'USB · CAMERA ON', 70, 63, 14, '#aaa8a6', True)
        rect(ctx, 405, 34, 44, 24, BG)
        rect(ctx, 406, 35, 42, 22, '#3a3937', '#92918f', 1)
        knob = 408 + 20 * smooth((local - 2.2) / .22)
        rect(ctx, knob, 38, 15, 16, INK)
    if local >= 4.1:
        pulse = smooth((local - 4.1) / .25)
        rect(ctx, 25, 104, 430, 41, stroke=ACCENT, line=1, alpha=pulse * .7)
    ctx.restore()
    pointer = ease((local - .8) / 1.2)
    target_x, target_y = x + scale * 430, y + scale * 50
    pointer_x = 1020 + (target_x - 1020) * pointer
    pointer_y = 1140 + (target_y - 1140) * pointer
    if .8 < local < 3.3:
        cursor(ctx, pointer_x, pointer_y, 1.3)
    click = clamp((local - 2.2) / .45)
    if 0 < click < 1:
        circle(ctx, target_x, target_y, 14 + click * 44, ACCENT, width=3, alpha=1 - click)
    if local >= 4.1:
        text(ctx, 'Front / back controls*', 72, 1220, 31, ACCENT, True)
        text(ctx, '*DroidCam Pro may be required.', 72, 1260, 26, MUTED)
    else:
        text(ctx, 'UI demo · DroidCam required on iPhone', 72, 1250, 25, MUTED)


def benefits(ctx, local, panel):
    text(ctx, 'SMALL APP. PROPERLY AT HOME.', 72, 225, 26, MUTED)
    labels = [('USB.', 410, 0), ('No OBS.', 610, .75), ('Open source.', 810, 1.5)]
    for value, y, delay in labels:
        entrance(ctx, value, 120, y, local, delay, 92, ACCENT if value == 'Open source.' else INK, True)
        amount = ease((local - delay) / .7)
        rect(ctx, 72, y - 58, 9, 60 * amount, ACCENT, alpha=amount)
        line(ctx, [(72, y + 54), (1008, y + 54)], INK, 1, .14 * amount)
    entrance(ctx, 'Tested: 720p / 30 fps over USB', 72, 1045, local, 2.1, 31, INK)
    entrance(ctx, 'MIT licensed. Native Omarchy controls.', 72, 1110, local, 2.3, 28, MUTED)
    text(ctx, 'DroidCam handles capture on the iPhone.', 72, 1245, 25, MUTED)


def outro(ctx, local, panel):
    text(ctx, 'MADE FOR THE OMARCHY CROWD.', 72, 225, 26, MUTED)
    entrance(ctx, 'iPhone Camera', 72, 390, local, 0, 88, bold=True)
    entrance(ctx, 'for Omarchy.', 72, 500, local, .15, 88, ACCENT, True)
    entrance(ctx, 'Make your phone your webcam.', 72, 620, local, .4, 31, MUTED)
    rect(ctx, 72, 740, 936, 285, '#151416', '#39373a', 2)
    camera(ctx, 108, 782, 37, ACCENT)
    text(ctx, 'PUBLIC REPO / MIT', 168, 812, 24, MUTED)
    text(ctx, 'github.com/Nikshay1/', 108, 890, 44, INK, True)
    text(ctx, 'iphonecam', 108, 950, 44, ACCENT, True)
    entrance(ctx, 'Install with your AI agent.', 72, 1130, local, .7, 35, INK, True)
    entrance(ctx, 'The README has the whole setup.', 72, 1190, local, .9, 28, MUTED)
    text(ctx, 'Native panel + Arch package', 72, 1250, 24, MUTED)


SCENES = [(0., intro), (3.6, connect), (7.2, product), (14., benefits), (20., outro)]


def frame(t, panel):
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, WIDTH, HEIGHT)
    ctx = cairo.Context(surface)
    ctx.set_antialias(cairo.ANTIALIAS_BEST)
    backdrop(ctx, t)
    for i, (start, draw) in enumerate(SCENES):
        end = SCENES[i + 1][0] if i + 1 < len(SCENES) else DURATION + 1
        if not start - .35 <= t <= end:
            continue
        incoming = smooth((t - (start - .35)) / .35) if start else 1
        outgoing = 1 - smooth((t - (end - .35)) / .35)
        opacity = incoming * outgoing
        ctx.save()
        ctx.translate((1 - incoming) * 28 - (1 - outgoing) * 28, 0)
        ctx.push_group()
        draw(ctx, max(0, t - start), panel)
        layer = ctx.pop_group()
        ctx.set_source(layer)
        ctx.paint_with_alpha(opacity)
        ctx.restore()
    chrome(ctx, t)
    surface.flush()
    return surface


def soundtrack(path):
    """Compose a restrained original electronic bed and interface accents."""
    rate = 48000
    n = int(DURATION * rate)
    samples = array('f', [0.]) * n
    rng = random.Random(17)

    def tone(at, frequency, length, level, decay=4, attack=.012):
        start = int(at * rate)
        for j in range(min(int(length * rate), n - start)):
            time = j / rate
            envelope = min(1, time / attack) * math.exp(-time * decay)
            wave_value = (math.sin(math.tau * frequency * time) + .15 * math.sin(math.tau * frequency * 2 * time)) / 1.15
            samples[start + j] += level * envelope * wave_value

    chords = [
        (0., 7.5, [110., 130.813, 164.814]),
        (7., 7.5, [87.307, 110., 130.813]),
        (13.7, 6.8, [130.813, 164.814, 195.998]),
        (19.7, 6.3, [110., 164.814, 220.]),
    ]
    for at, length, notes in chords:
        start = int(at * rate)
        for j in range(min(int(length * rate), n - start)):
            time = j / rate
            envelope = min(1, time / 1.6) * min(1, (length - time) / 1.4)
            value = sum(math.sin(math.tau * note * time) + .18 * math.sin(math.tau * note * 2.001 * time) for note in notes)
            samples[start + j] += .019 * envelope * value
    beat = 60 / 68
    for step in range(int(DURATION / (2 * beat))):
        at = step * 2 * beat + .1
        tone(at, 55., .95, .075, 5, .008)
        if step % 2 == 1:
            tone(at + beat, 440., .7, .018, 6, .03)
    for at in [3.3, 6.9, 13.7, 19.7]:
        start, previous = int(at * rate), 0.
        for j in range(int(.35 * rate)):
            noise = rng.uniform(-1, 1)
            previous = .92 * previous + .08 * noise
            shape = math.sin(math.pi * j / (.35 * rate)) ** 2
            samples[start + j] += previous * shape * .12
    tone(9.4, 1400, .065, .09, 60, .001)
    tone(20.1, 440, 2, .065, 2.5)
    tone(20.2, 659.255, 2, .055, 2.5)
    pcm = array('h')
    for i, value in enumerate(samples):
        t = i / rate
        fade = min(1, t / .25) * min(1, (DURATION - t) / .7)
        sample = int(max(-.9, min(.9, value * fade)) * 32767)
        pcm.extend((sample, sample))
    with wave.open(str(path), 'wb') as output:
        output.setnchannels(2)
        output.setsampwidth(2)
        output.setframerate(rate)
        output.writeframes(pcm.tobytes())


CAPTIONS = [
    (0, 3.6, 'Your iPhone. Now a webcam.\nBuilt for Omarchy.'),
    (3.6, 7.2, 'Plug in. Take control.\nDroidCam on iPhone. USB to Omarchy.'),
    (7.2, 11.3, 'A native Omarchy panel.\nUSB camera on/off from your bar. UI demo.'),
    (11.3, 14, 'Front/back controls.\nDroidCam Pro may be required.'),
    (14, 20, 'USB. No OBS. Open source.\nTested at 720p, 30 fps over USB.'),
    (20, 26, 'iPhone Camera for Omarchy.\ngithub.com/Nikshay1/iphonecam\nInstall with your AI agent.'),
]


def stamp(t):
    milliseconds = int(t * 1000)
    seconds, ms = divmod(milliseconds, 1000)
    minutes, sec = divmod(seconds, 60)
    return f'{minutes // 60:02}:{minutes % 60:02}:{sec:02},{ms:03}'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output-dir', type=Path, default=ROOT / 'build/launch')
    parser.add_argument('--stills-only', action='store_true')
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    panel = cairo.ImageSurface.create_from_png(str(ROOT / 'docs/panel.png'))
    for index, t in enumerate([2.1, 5.9, 9.7, 12.3, 18.2, 23.5]):
        frame(t, panel).write_to_png(str(args.output_dir / f'scene-{index + 1:02}.png'))
    frame(2.1, panel).write_to_png(str(args.output_dir / 'iphonecam-poster.png'))
    captions = '\n\n'.join(f'{i + 1}\n{stamp(a)} --> {stamp(b)}\n{words}' for i, (a, b, words) in enumerate(CAPTIONS))
    (args.output_dir / 'iphonecam-captions.srt').write_text(captions + '\n')
    if args.stills_only:
        print('Rendered six storyboard frames and poster.', flush=True)
        return
    wav = args.output_dir / 'soundtrack.wav'
    soundtrack(wav)
    video = args.output_dir / 'iphonecam-x-cinematic.mp4'
    command = ['ffmpeg', '-hide_banner', '-loglevel', 'warning', '-y', '-f', 'rawvideo', '-pixel_format', 'bgra',
               '-video_size', f'{WIDTH}x{HEIGHT}', '-framerate', str(FPS), '-i', 'pipe:0', '-i', str(wav),
               '-map', '0:v:0', '-map', '1:a:0', '-c:v', 'libx264', '-preset', 'medium', '-crf', '18',
               '-profile:v', 'high', '-pix_fmt', 'yuv420p', '-c:a', 'aac', '-b:a', '192k', '-ar', '48000',
               '-af', 'loudnorm=I=-18:TP=-1.5:LRA=11', '-movflags', '+faststart', '-t', str(DURATION), str(video)]
    process = subprocess.Popen(command, stdin=subprocess.PIPE)
    try:
        for index in range(DURATION * FPS):
            surface = frame(index / FPS, panel)
            process.stdin.write(surface.get_data())
            if index % (3 * FPS) == 0:
                print(f'Rendered {index // FPS:02}/{DURATION}s', flush=True)
        process.stdin.close()
        if process.wait() != 0:
            raise RuntimeError('FFmpeg encoding failed')
    except BaseException:
        process.kill()
        process.wait()
        raise
    metadata = json.loads(subprocess.check_output(['ffprobe', '-v', 'error', '-show_format', '-show_streams', '-of', 'json', str(video)]))
    (args.output_dir / 'video-metadata.json').write_text(json.dumps(metadata, indent=2) + '\n')
    print(f'Finished {video} ({video.stat().st_size / 1_000_000:.1f} MB)', flush=True)


if __name__ == '__main__':
    main()
