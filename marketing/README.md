# iPhone Camera launch kit

## Current cut: native Remotion rebuild

The current launch film is the [4:3 Remotion project](remotion/README.md), rebuilt with Liam Johnston’s Motion Graphics and Cinematic Camera skills. It uses the real native panel, a shared camera world, action-led timing, and an independent visual review gate. Render output: `build/launch-remotion/iphonecam-remotion-v3.mp4`. The earlier photographic/Cairo version below is retained as a draft, not the recommended cut.

## Earlier draft

A 26-second cinematic launch film for X, designed to work with audio muted. The visual style uses near-black, warm metallic highlights, monospaced typography, a slow hardware reveal, and a close-up of the actual native Omarchy panel.

## Deliverables

Generated exports live in `build/launch/` and are intentionally excluded from Git:

- `iphonecam-x-cinematic.mp4` — 1080×1350, 4:5, 30 fps, H.264 High / YUV420P, AAC stereo, fast-start MP4.
- `iphonecam-poster.png` — matching cover image.
- `iphonecam-captions.srt` — accessibility captions; key messages are also visible in the video itself.
- `scene-01.png` through `scene-06.png` — review frames.
- `soundtrack.wav` — original synthesized score and interface sound accents.
- `video-metadata.json` — FFprobe export details.

[X-POSTS.md](X-POSTS.md) contains three launch-post options, two optional replies, and an accessible description. No post is published by these scripts.

## Render or edit

Requires FFmpeg, Python with `cairo`, and a system monospaced font. On Arch, the Python binding is provided by `python-cairo`; the preferred font is JetBrains Mono Nerd Font. Fonts are not bundled. `cairo` and FFmpeg are already available on the maintainer's laptop.

```bash
python marketing/render_launch.py --stills-only
python marketing/render_launch.py
```

Change copy, timing, colors, and motion in [render_launch.py](render_launch.py). It renders locally without a browser, network requests, or camera access. The script streams frames straight to FFmpeg instead of storing hundreds of full-resolution frames. Audio is generated with the Python standard library and normalized to approximately −18 LUFS; no licensed stock music is used.

## Storyboard

| Time | Scene |
| --- | --- |
| 0–3.6 s | Hardware reveal: “Your iPhone. Now a webcam.” |
| 3.6–7.2 s | USB connection: “Plug in. Take control.” |
| 7.2–14 s | Real panel screenshot with an illustrative toggle animation and lens-control highlight |
| 14–20 s | USB, no OBS, open source, and the verified 720p/30 fps output |
| 20–26 s | Product name, public repository, and AI-agent installation call to action |

The panel image is the repository's existing fictional demo screenshot, `docs/panel.png`. The animated toggle is a UI illustration. No personal camera frames, device IDs, or live desktop recordings are used. DroidCam is identified as required and the lens-control scene carries the Pro qualification. Do not present the animation as a successful physical lens-switch test.

## Generated artwork provenance

[assets/cinematic-hardware.png](assets/cinematic-hardware.png) was created using the built-in image-generation tool for this launch film and copied into the project. It is an illustrative studio image, not a photograph from the hardware acceptance test. No image-generation API key or external asset account was used. The original prompt is preserved in [assets/IMAGE-PROMPT.md](assets/IMAGE-PROMPT.md).

The underlying app's MIT license applies to the launch source code and original synthesized score. The hero is generated artwork; there is no assertion that it is an original real-world product photograph or that the depicted laptop is a particular brand.

## Export checks

```bash
ffprobe -v error -show_entries stream=codec_name,width,height,pix_fmt,r_frame_rate \
  -show_entries format=duration,size -of json build/launch/iphonecam-x-cinematic.mp4
ffmpeg -v error -i build/launch/iphonecam-x-cinematic.mp4 -f null -
```

The export uses the H.264/AAC MP4 combination described in [X's Media Studio guidance](https://help.x.com/en/using-x/media-studio-faqs). Account upload limits can differ; see [X's video help](https://help.x.com/en/using-x/x-videos).
