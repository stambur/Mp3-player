#-------------------------------------------------
#
# Project created by QtCreator 2016-07-25T19:00:09
#
#-------------------------------------------------

QT       += core gui \
            multimedia \
            multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

LIBS += -L/usr/include/lirc -l lirc_client
LIBS += -L/usr/include/taglib -ltag
LIBS += -L/usr/local/lib -lwiringPi
LIBS += -L/usr/local/lib -lwiringPiDev

TARGET = QMedPlayer_test
TEMPLATE = app

CONFIG += c++11

SOURCES += main.cpp\
        dialog.cpp \
    lirc.cpp \
    getpath.cpp

HEADERS  += dialog.h \
    lirc.h \
    getpath.h

FORMS    += dialog.ui

OTHER_FILES +=
