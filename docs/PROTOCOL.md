# USB and camera protocol

The desktop does not access private iOS camera APIs. DroidCam provides the phone-side capture implementation and its licensing rules remain in effect.

1. `idevice_id -l` lists locally attached USB devices. Only validated 40-character hexadecimal or modern 8-16 hexadecimal IDs are accepted. Network device discovery is not enabled.
2. `iproxy --local --source 127.0.0.1 --udid ID PORT:4747` tunnels a dynamically selected loopback port to DroidCam's iPhone TCP port.
3. `GET /v1/phone/name` establishes readiness. `GET /v1/camera/camera_list` returns newline-separated lens labels, with the zero-based line number used as the control ID.
4. FFmpeg reads `GET /video/1280x720` (or the selected resolution), decodes the browser-compatible video feed, preserves its aspect ratio, and writes 30 fps YUV420P to `/dev/video42`.
5. Once FFmpeg reports an output frame, `GET /v1/camera/info` reports the active lens. Successful JSON with a numeric `active` field enables remote controls.
6. `PUT /v1/camera/active/ID` changes the active lens. The UI reads the active lens again after success, rather than assuming a successful change. HTTP 403/550 is treated as a licensing restriction.

References:

- [Vendor USB instructions](https://droidcam.app/obs/usage/)
- [Vendor browser video and Pro remote documentation](https://droidcam.app/help/)
- [Vendor discussion of HTTP controls](https://github.com/dev47apps/droidcam-linux-client/issues/140)
- [oma-cam integration of camera-list/info/active endpoints](https://github.com/saurasanchezpablo/oma-cam)
- [iproxy/libusbmuxd](https://github.com/libimobiledevice/libusbmuxd)
- [v4l2loopback](https://github.com/v4l2loopback/v4l2loopback)

## Verification boundaries

Automated tests exercise actual loopback HTTP requests and replies, including denied Pro controls, lens selection, and stale-session cancellation. They also validate device identity restrictions, QML interactions, translation changes, and tray serialization. A mock endpoint is not evidence of phone-version compatibility.

Hardware acceptance requires an iPhone with DroidCam open, a working USB data cable, and the loopback module installed for the running kernel. Check camera-on output in a real V4L2 consumer, both lenses when Pro allows them, unplug while streaming, replug with automatic activation both disabled/enabled, and off/quit behavior. Record results in this file after performing them.
