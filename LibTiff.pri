
INCLUDEPATH += $$(LIBTIFF_ROOT)/include
LIBS += -L$$(LIBTIFF_ROOT)/lib


win32-msvc*{
    CONFIG(debug, debug|release) {
        LIBS += -ltiffd
    }
    else {
        LIBS += -ltiff
    }
}

unix {
    LIBS += -ltiff
}

