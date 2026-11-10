#!/usr/bin/env python3
"""Run under dbus-run-session; verify the built app from a separate process."""
import json
import os
from pathlib import Path
import subprocess
import time

root = Path(__file__).resolve().parents[1]
binary = str(root / "build/iphonecam")
env = dict(os.environ, QT_QPA_PLATFORM="offscreen", QT_QUICK_BACKEND="software", QT_QPA_PLATFORMTHEME="", QT_ACCESSIBILITY="0", NO_AT_BRIDGE="1")
app = subprocess.Popen([binary, "--tray"], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
service = "io.github.Nikshay1.iphonecam"


def call(path, interface, *arguments):
    return subprocess.check_output([
        "gdbus", "call", "--session", "--dest", service, "--object-path", path,
        "--method", interface, "--", *arguments,
    ], text=True)


try:
    for _ in range(50):
        reply = subprocess.run([binary, "status"], env=env, capture_output=True, text=True)
        if reply.returncode == 0:
            break
        time.sleep(0.1)
    else:
        raise AssertionError("Application never registered on D-Bus")
    status = json.loads(reply.stdout)
    assert not status["enabled"] and not status["streaming"], status
    layout = call("/Menu", "com.canonical.dbusmenu.GetLayout", "0", "-1", "[]")
    assert "Turn camera on" in layout and "Controls and setup" in layout, layout
    props = call("/StatusNotifierItem", "org.freedesktop.DBus.Properties.GetAll", "org.kde.StatusNotifierItem")
    assert "iphonecam" in props and "ItemIsMenu" in props, props
    subprocess.run([binary, "stop"], env=env, check=True)
    call("/Menu", "com.canonical.dbusmenu.Event", "3", "clicked", "<0>", "0")
    subprocess.run([binary, "quit"], env=env, check=True)
    assert app.wait(timeout=5) == 0
    print("PASS: separate-process D-Bus status, tray layout, properties, event, stop, and quit")
finally:
    if app.poll() is None:
        app.terminate()
        app.wait(timeout=5)
    stdout, stderr = app.communicate()
    if stderr:
        print(stderr)
