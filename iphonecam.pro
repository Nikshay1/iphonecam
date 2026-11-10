QT += core gui qml quick quickcontrols2 network dbus
CONFIG += c++17 release
TARGET = iphonecam
TEMPLATE = app
SOURCES += src/main.cpp src/backend.cpp src/protocol.cpp src/tray.cpp src/theme.cpp src/translator.cpp
HEADERS += src/backend.h src/protocol.h src/tray.h src/theme.h src/translator.h
RESOURCES += src/resources.qrc
