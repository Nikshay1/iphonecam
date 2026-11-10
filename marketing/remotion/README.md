# iPhone Camera — native Remotion launch film

Replacement for the earlier Cairo/photographic launch cut. Built with Liam Johnston's **motion-graphics**, **cinematic-camera**, and **terminal-inserts** skills, plus the official Remotion API skills.

## Locked brief

- 4:3, 1440×1080, 30 fps, 15 seconds.
- Clean dark, selected by the user.
- The product interface is the main scene, not an image beneath a title slide.
- One connected world and one focal-x/focal-y/zoom timeline.
- Existing panel toggle → actual virtual-camera diagnostic output → public repository.
- No generated hardware image, lighting effect, particles, fake call footage, or lens-switch claim.

## Build and render

Use a supported Node version (the maintainer used Node 24).

```bash
npm ci
npx tsc --noEmit
npx remotion render src/index.ts IPhoneCameraLaunch ../../build/launch-remotion/iphonecam-remotion-v3.mp4 \
  --concurrency=4 --codec=h264 --crf=16 --audio-codec=aac --audio-bitrate=192k
```

Remotion uses its own bundled headless browser. Run `npm run dev` to edit the timeline interactively in Remotion Studio. `src/Composition.tsx` exposes the camera keyframe arrays and separates the native panel, cursor, command output, connection, and repository destination into components.

The default render has original synthesized, loudness-normalized sound effects/bed in `public/sound.wav`. The no-audio version can be exported with Remotion's `--muted` option. No stock-music license or external media account is needed.

## Authentic assets and boundaries

- `public/research/panel-off.png` is the existing, real native Omarchy demo-panel screenshot from `docs/panel.png`.
- `public/research/iphonecam.svg` is the exact project icon from `pkgbuild/iphonecam.svg`.
- `public/research/doctor.json` is actual local diagnostic output, with no device IDs or camera frames.
- The on-state is a faithful **UI demonstration** of the existing toggle and status label. It does not assert that a live call or physical lens change was recorded.
- The footer keeps the DroidCam requirement and possible Pro restriction visible.
- The GitHub mark comes from the chosen skill's bundled Simple Icons asset; it is shown unmodified on a neutral white backing for contrast.
- JetBrains Mono is bundled with its OFL license from its official repository.

The parent `marketing/.promptible/` directory contains the local, hash-locked research/reference/candidate/critic evidence used by the selected skill's build and delivery gates. It is excluded from Git because manifests contain machine-specific paths. The reference mechanism is `kickbacks-same-run-editor`: close authentic working surface → action → reveal of the related result. The film and contact sheets must pass an independent visual critic before delivery.

## Review points

Watch the entire clip, not just the poster. Inspect the opening, camera destinations, toggle state change, output, pullback, and final hold. Essential identity and controls must remain readable, the pointer must cause the existing toggle action, and all views must stay in the same connected world. A clean render is not by itself visual approval.


## Accepted render

Independent review passed with no hard failures; the selected skill's delivery gate printed `DELIVERY AUTHORIZED` after the revision. The first candidate was rejected for cropped native identity and small requirements text, both corrected before delivery. Full review: [review.json](review.json).

- Final video: `build/launch-remotion/iphonecam-remotion-v3.mp4`.
- Cover: `build/launch-remotion/iphonecam-remotion-cover.png`.
- Captions: `build/launch-remotion/iphonecam-remotion-v3.srt`.
- Video timeline: exactly 450 frames / 15 seconds at 30 fps; AAC container padding may report approximately 15.019 seconds.
- Placement: post the 1440×1080 file directly as a 4:3 X video. In a vertical edit, preserve its 4:3 frame as an insert rather than cropping its controls or qualifications.

Accepted render SHA-256: `6a75c466be57c3c158c4b3704e05421e118d984e0069a798b3a445969b877975`.
