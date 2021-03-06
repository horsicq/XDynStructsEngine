INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += \
    $$PWD/xdynstructsengine.h

SOURCES += \
    $$PWD/xdynstructsengine.cpp

contains(XCONFIG, use_xwiniodriver) {
    DEFINES += USE_XWINIODRIVER
}

!contains(XCONFIG, xformats) {
    XCONFIG += xformats
    include($$PWD/../Formats/xformats.pri)
}

!contains(XCONFIG, xprocess) {
    XCONFIG += xprocess
    include($$PWD/../XProcess/xprocess.pri)
}

!contains(XCONFIG, dialogtextinfo) {
    XCONFIG += dialogtextinfo
    include($$PWD/../FormatDialogs/dialogtextinfo.pri)
}

win32 {
    contains(XCONFIG, use_xwiniodriver) {
        DEFINES += USE_XWINIODRIVER
        !contains(XCONFIG, xwiniodriver) {
            XCONFIG += xwiniodriver
            include($$PWD/../XWinIODriver/xwiniodriver.pri)
        }
    }
}

# TODO cmake
DISTFILES += \
    $$PWD/LICENSE \
    $$PWD/README.md
