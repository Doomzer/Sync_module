QT -= gui
QT += sql
QT += network
QT += xml
QT += xmlpatterns
QT += concurrent

TEMPLATE = app
TARGET = sync-module
INCLUDEPATH += .

win32 {
LIBS += -lws2_32

CONFIG += c++17 console
CONFIG -= app_bundle
}

# The following define makes your compiler warn you if you use any
# feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Input
HEADERS += net.h \
           Ekasui/soapH.h \
           Ekasui/soapJpiServiceSOAPProxy.h \
           Ekasui/soapStub.h \
           Ekasui/stdsoap2.h \
           asuinstruct.h \
           connectorsdo.h \
           dbsettings.h \
           ekasuiinstruct.h \
           instructworker.h \
           sdoworker.h \
           xmlsettings.h
SOURCES += main.cpp \
           asuinstruct.cpp \
           connectorsdo.cpp \
           dbsettings.cpp \
           ekasuiinstruct.cpp \
           instructworker.cpp \
           net.cpp \
           Ekasui/soapC.cpp \
           Ekasui/soapJpiServiceSOAPProxy.cpp \
           Ekasui/stdsoap2.cpp \
           sdoworker.cpp \
           xmlsettings.cpp
