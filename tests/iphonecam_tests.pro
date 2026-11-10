QT += core gui qml quick quickcontrols2 network dbus testlib
CONFIG += c++17 testcase console
TARGET = iphonecam_tests
TEMPLATE = app
SOURCES += iphonecam_tests.cpp ../src/protocol.cpp ../src/backend.cpp ../src/tray.cpp ../src/theme.cpp ../src/translator.cpp
HEADERS += ../src/protocol.h ../src/backend.h ../src/tray.h ../src/theme.h ../src/translator.h
INCLUDEPATH += ../src
RESOURCES += ../src/resources.qrc
