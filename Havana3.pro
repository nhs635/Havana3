#-------------------------------------------------
#
# Project created by QtCreator 2018-08-08T16:07:08
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets serialport

TARGET = Havana3
TEMPLATE = app

CONFIG += console
RC_FILE += Havana3.rc

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0




INCLUDEPATH += $$PWD/include

LIBS += $$PWD/lib/PX14_64.lib
LIBS += $$PWD/lib/AxsunOCTCapture.lib
LIBS += $$PWD/lib/NIDAQmx.lib
LIBS += $$PWD/lib/intel64_win/ippcore.lib \
        $$PWD/lib/intel64_win/ippi.lib \
        $$PWD/lib/intel64_win/ipps.lib \
        $$PWD/lib/intel64_win/ippvm.lib
debug {
    LIBS += $$PWD/lib/intel64_win/vc14/tbb_debug.lib
    LIBS += $$PWD/lib/opencv_world320d.lib
}
release {
    LIBS += $$PWD/lib/intel64_win/vc14/tbb.lib
    LIBS += $$PWD/lib/opencv_world320.lib
}
LIBS += $$PWD/lib/intel64_win/mkl_core.lib \
        $$PWD/lib/intel64_win/mkl_tbb_thread.lib \
        $$PWD/lib/intel64_win/mkl_intel_lp64.lib



SOURCES += Havana3/Havana3.cpp \
    Havana3/MainWindow.cpp \
    Havana3/QStreamTab.cpp \
    Havana3/QResultTab.cpp \
    Havana3/QOperationTab.cpp \
    Havana3/QDeviceControlTab.cpp \
    Havana3/QVisualizationTab.cpp \
    Havana3/QProcessingTab.cpp \
    Havana3/Viewer/QScope.cpp \
    Havana3/Viewer/QImageView.cpp \
    Havana3/Dialog/FlimCalibDlg.cpp \
    Havana3/Dialog/SaveResultDlg.cpp \
    Havana3/Dialog/PulseReviewDlg.cpp \
    Havana3/Dialog/LongitudinalViewDlg.cpp

SOURCES += DataAcquisition/SignatecDAQ/SignatecDAQ.cpp \
    DataAcquisition/FLImProcess/FLImProcess.cpp \
    DataAcquisition/AxsunCapture/AxsunCapture.cpp \
    DataAcquisition/ThreadManager.cpp \
    DataAcquisition/DataAcquisition.cpp

SOURCES += MemoryBuffer/MemoryBuffer.cpp

SOURCES += DeviceControl/FreqDivider/FreqDivider.cpp \
    DeviceControl/PmtGainControl/PmtGainControl.cpp \
    DeviceControl/ElforlightLaser/ElforlightLaser.cpp \
    DeviceControl/AxsunControl/AxsunControl.cpp \
    DeviceControl/FaulhaberMotor/FaulhaberMotor.cpp \
    DeviceControl/FaulhaberMotor/PullbackMotor.cpp \
    DeviceControl/FaulhaberMotor/RotaryMotor.cpp \


HEADERS += Havana3/Configuration.h \
    Havana3/MainWindow.h \
    Havana3/QStreamTab.h \
    Havana3/QResultTab.h \
    Havana3/QOperationTab.h \
    Havana3/QDeviceControlTab.h \
    Havana3/QVisualizationTab.h \
    Havana3/QProcessingTab.h \
    Havana3/Viewer/QScope.h \
    Havana3/Viewer/QImageView.h \
    Havana3/Dialog/FlimCalibDlg.h \
    Havana3/Dialog/SaveResultDlg.h \
    Havana3/Dialog/PulseReviewDlg.h \
    Havana3/Dialog/LongitudinalViewDlg.h

HEADERS += DataAcquisition/SignatecDAQ/SignatecDAQ.h \
    DataAcquisition/FLImProcess/FLImProcess.h \
    DataAcquisition/AxsunCapture/AxsunCapture.h \
    DataAcquisition/ThreadManager.h \
    DataAcquisition/DataAcquisition.h

HEADERS += MemoryBuffer/MemoryBuffer.h

HEADERS += DeviceControl/FreqDivider/FreqDivider.h \
    DeviceControl/PmtGainControl/PmtGainControl.h \
    DeviceControl/ElforlightLaser/ElforlightLaser.h \
    DeviceControl/AxsunControl/AxsunControl.h \
    DeviceControl/FaulhaberMotor/FaulhaberMotor.h \
    DeviceControl/FaulhaberMotor/PullbackMotor.h \
    DeviceControl/FaulhaberMotor/RotaryMotor.h \
    DeviceControl/QSerialComm.h


FORMS   += Havana3/MainWindow.ui
