INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += \
    $$PWD/xdynstructsengine.h

SOURCES += \
    $$PWD/xdynstructsengine.cpp

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
