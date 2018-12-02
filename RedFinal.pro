TEMPLATE = app
CONFIG += console c++1z
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    main.cpp \
    parse.cpp \
    search_server.cpp \
    profile.cpp \
    test_runner.cpp

HEADERS += \
    iterator_range.h \
    parse.h \
    search_server.h \
    profile.h \
    test_runner.h \
    synchronized.h
