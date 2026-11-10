# Development

Keep the source in `/home/nikshay/Projects/iphonecam` and the GitHub repository in sync. After each completed requested change, run `bin/build` and `bin/test`, inspect the diff, commit it, and push to the configured `origin`. Never commit build outputs, credentials, device IDs, or live camera images. Do not make background commits or push unrelated user edits.

Use C++ and Qt Quick with qmake6 for the backend and optional settings window. The primary interface is the native Omarchy panel in `omarchy-plugin/`; use actual `qs.Ui` components rather than reproducing them in a desktop window. After panel changes run `omarchy plugin validate omarchy-plugin`, `node tests/panel_model.test.js`, and `bin/enable-panel` to deploy the tracked source. Keep the application independent of OBS and Omarchy's packaged source. Settings belong to this app; never modify files under `/usr/share/omarchy`.

Preserve the off-by-default behavior. Automatic video startup requires the user's explicit preference. Use only the dedicated, driver-checked `/dev/video42` output and loopback-only USB forwarding. Do not assume camera IDs correspond to front or back. Report DroidCam licensing restrictions honestly.

Run `bin/test` after backend changes. Add regression coverage for state transitions, D-Bus behavior, or protocol failures. Do not present mocked success as hardware validation. Update README, screenshots using `--demo`, package version, and verification notes when behavior changes.

Install with the Arch package. Driver setup requires root; request credentials through the available secret-entry tool when needed. Do not weaken permissions or change sudo policy. Keep subsequent task changes pushed to the user's existing repository.
