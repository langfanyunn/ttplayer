#-------------------------------------------------
#
# Project created by QtCreator 2014-04-21T13:03:36
#
#-------------------------------------------------

QT       += core gui multimedia network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MusicWidget
TEMPLATE = app


SOURCES += main.cpp\
        mywidget.cpp \
    mylrc.cpp \
    mypushbutton.cpp \
    myslider.cpp \
    playlist.cpp \
    mylrcwnd.cpp \
    noframewidget.cpp \
    myminimainwnd.cpp \
    myminilrcwnd.cpp \
    dialog.cpp \
    deslrcdialog.cpp \
    brower.cpp \
    downloadthread.cpp \
    downloadtablewidget.cpp \
    downloaddialog.cpp \
    lrcdownloadthread.cpp

HEADERS  += mywidget.h \
    mylrc.h \
    mypushbutton.h \
    myslider.h \
    playlist.h \
    mylrcwnd.h \
    noframewidget.h \
    myminimainwnd.h \
    myminilrcwnd.h \
    dialog.h \
    deslrcdialog.h \
    brower.h \
    downloadthread.h \
    downloadtablewidget.h \
    downloaddialog.h \
    lrcdownloadthread.h

FORMS    += mywidget.ui \
    dialog.ui \
    deslrcdialog.ui \
    downloaddialog.ui

RESOURCES += \
    skin.qrc

OTHER_FILES += \
    skin1.qss \
    skin2.qss
