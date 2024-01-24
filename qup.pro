macx {
dmg.commands = make install && hdiutil create Qup.d.dmg -srcfolder Qup.d
}

unix {
doxygen.commands = doxygen qup.doxygen
purge.commands   = find . -name '*~' -exec rm {} \\;
}

CONFIG	    += qt release warn_on
LANGUAGE    = C++
QMAKE_CLEAN += Qup
QT	    += gui network widgets

freebsd-* {
QMAKE_CXXFLAGS_RELEASE += -Wall \
                          -Wcast-align \
                          -Wcast-qual \
                          -Wdouble-promotion \
                          -Werror \
                          -Wextra \
                          -Wformat=2 \
                          -Woverloaded-virtual \
                          -Wpointer-arith \
                          -Wstack-protector \
                          -Wstrict-overflow=5 \
                          -Wundef \
                          -fPIE \
                          -fstack-protector-all \
                          -fwrapv \
                          -pedantic \
                          -std=c++17
} else:macx {
QMAKE_CXXFLAGS_RELEASE += -Wall \
                          -Wcast-align \
                          -Wcast-qual \
                          -Wdouble-promotion \
                          -Wenum-compare \
                          -Wextra \
                          -Wformat=2 \
                          -Woverloaded-virtual \
                          -Wpointer-arith \
                          -Wstack-protector \
                          -Wstrict-overflow=5 \
                          -Wundef \
                          -fPIE \
                          -fstack-protector-all \
                          -fwrapv \
                          -pedantic \
                          -std=c++17
} else:win32 {
QMAKE_CXXFLAGS_RELEASE += -Wall \
                          -Wcast-align \
                          -Wcast-qual \
                          -Wdouble-promotion \
                          -Wenum-compare \
                          -Wextra \
                          -Wformat=2 \
                          -Wl,-z,relro \
                          -Wno-class-memaccess \
                          -Wno-deprecated-copy \
                          -Woverloaded-virtual \
                          -Wpointer-arith \
                          -Wstack-protector \
                          -Wstrict-overflow=5 \
                          -Wundef \
                          -fPIE \
                          -fwrapv \
                          -pedantic \
                          -pie \
                          -std=c++17
} else {
QMAKE_CXXFLAGS_RELEASE += -Wall \
                          -Wcast-qual \
                          -Wdouble-promotion \
                          -Wenum-compare \
                          -Wextra \
                          -Wfloat-equal \
                          -Wformat=2 \
                          -Wl,-z,relro \
                          -Wlogical-op \
                          -Wno-class-memaccess \
                          -Wno-deprecated-copy \
                          -Woverloaded-virtual \
                          -Wpointer-arith \
                          -Wstack-protector \
                          -Wstrict-overflow=1 \
                          -Wundef \
                          -fPIE \
                          -fstack-protector-all \
                          -fwrapv \
                          -pedantic \
                          -pie \
                          -std=c++17
}

QMAKE_DISTCLEAN     += -r .qmake* \
                       -r html \
                       -r latex \
                       -r temporary

macx {
LIBS                           += -framework AppKit -framework Cocoa
OBJECTIVE_HEADERS              += source/CocoaInitializer.h
OBJECTIVE_SOURCES              += source/CocoaInitializer.mm
QMAKE_DISTCLEAN                += -r Qup.d
QMAKE_EXTRA_TARGETS            += dmg
QMAKE_MACOSX_DEPLOYMENT_TARGET = 11.0
}

unix {
QMAKE_EXTRA_TARGETS += doxygen purge
}

FORMS       += ui/qup.ui
HEADERS     += source/qup.h
INCLUDEPATH += source
MOC_DIR     = temporary/moc
OBJECTS_DIR = temporary/obj
PROJECTNAME = Qup
RCC_DIR     = temporary/rcc
RESOURCES   = images/images.qrc
SOURCES     += source/qup.cc \
               source/qup_main.cc
TARGET      = Qup
TEMPLATE    = app
UI_DIR      = temporary/ui
