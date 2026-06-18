QT       += core gui widgets concurrent svg network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17 windows

TARGET   = WSLTool
TEMPLATE = app

# Windows version targeting
DEFINES += WINVER=0x0601 _WIN32_WINNT=0x0601

# Windows libraries
LIBS += -ladvapi32 -lshell32 -luser32 -lole32 -lwbemuuid

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/core/systemdetector.cpp \
    src/core/wslmanager.cpp \
    src/core/diskmanager.cpp \
    src/core/migrationworker.cpp \
    src/ui/dashboardpage.cpp \
    src/ui/distributionpage.cpp \
    src/ui/migrationdialog.cpp \
    src/ui/migrationprogressdialog.cpp \
    src/ui/disclaimerdialog.cpp \
    src/ui/elevationdialog.cpp \
    src/ui/widgets/distrocard.cpp \
    src/ui/widgets/diskusagebar.cpp \
    src/ui/widgets/infocard.cpp \
    src/ui/widgets/sidebarbutton.cpp

HEADERS += \
    src/mainwindow.h \
    src/core/systemdetector.h \
    src/core/wslmanager.h \
    src/core/diskmanager.h \
    src/core/migrationworker.h \
    src/models/wsldistribution.h \
    src/models/diskinfo.h \
    src/models/systeminfo.h \
    src/ui/dashboardpage.h \
    src/ui/distributionpage.h \
    src/ui/migrationdialog.h \
    src/ui/migrationprogressdialog.h \
    src/ui/disclaimerdialog.h \
    src/ui/elevationdialog.h \
    src/ui/widgets/distrocard.h \
    src/ui/widgets/diskusagebar.h \
    src/ui/widgets/infocard.h \
    src/ui/widgets/sidebarbutton.h

RESOURCES += \
    resources/resources.qrc

RC_FILE = resources/app.rc

# Windows application manifest for UAC
# The manifest is embedded in the RC file

DESTDIR = $$PWD/build/release

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
