#-------------------------------------------------
#
# Project created by QtCreator 2018-08-08T16:07:08
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets serialport

TARGET = Havana3
TEMPLATE = app

CONFIG += c++14
debug {
CONFIG += console
}
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


win32 {
INCLUDEPATH += $$PWD/include

LIBS += $$PWD/lib/PX14_64.lib # \
#        $$PWD/lib/ATSApi.lib
LIBS += $$PWD/lib/AxsunOCTControl_LW.lib \
        $$PWD/lib/AxsunOCTCapture.lib
LIBS += $$PWD/lib/NIDAQmx.lib
CONFIG(debug, debug|release) {
LIBS += $$PWD/lib/opencv_world3416d.lib
} else {
LIBS += $$PWD/lib/opencv_world3416.lib
}
LIBS += $$PWD/lib/intel64_win/ippcore.lib \
        $$PWD/lib/intel64_win/ippi.lib \
        $$PWD/lib/intel64_win/ipps.lib \
        $$PWD/lib/intel64_win/ippvm.lib
LIBS += $$PWD/lib/intel64_win/vc14/tbb_debug.lib \
        $$PWD/lib/intel64_win/vc14/tbb.lib
LIBS += $$PWD/lib/intel64_win/mkl_core.lib \
        $$PWD/lib/intel64_win/mkl_tbb_thread.lib \
        $$PWD/lib/intel64_win/mkl_intel_lp64.lib
debug {
    LIBS += $$PWD/lib/vld.lib
}
}
macx {
INCLUDEPATH += /opt/intel/oneapi/ipp/2021.7.0/include \
            /opt/intel/oneapi/tbb/2021.8.0/include \
            /opt/intel/oneapi/mkl/2021.7.0/include
INCLUDEPATH += /opt/homebrew/Cellar/opencv@3/3.4.16_4/include

IPPLIBS = -lippch -lippcore -lippi -lipps -lippvm
MKLLIBS = -lmkl_core -lmkl_tbb_thread -lmkl_intel_lp64

LIBS += -L/opt/intel/oneapi/ipp/2021.7.0/lib $$IPPLIBS
CONFIG(debug, debug|release) {
LIBS += -L/opt/intel/oneapi/tbb/2021.8.0/lib -ltbb_debug
} else {
LIBS += -L/opt/intel/oneapi/tbb/2021.8.0/lib -ltbb_debug \
        -L/opt/intel/oneapi/tbb/2021.8.0/lib -ltbb
}
LIBS += -L/opt/intel/oneapi/mkl/2021.7.0/lib $$MKLLIBS

LIBS += -L/opt/homebrew/Cellar/opencv@3/3.4.16_4/lib -lopencv_core \
        -L/opt/homebrew/Cellar/opencv@3/3.4.16_4/lib -lopencv_ml
}



SOURCES += Havana3/Havana3.cpp \
    Havana3/HvnSqlDataBase.cpp \
    Havana3/MainWindow.cpp \
    Havana3/QHomeTab.cpp \    
    Havana3/QPatientSelectionTab.cpp \
    Havana3/QPatientSummaryTab.cpp \
    Havana3/QViewTab.cpp \
    Havana3/QStreamTab.cpp \
    Havana3/QResultTab.cpp \
    Havana3/Viewer/QScope.cpp \
    Havana3/Viewer/QImageView.cpp \
    Havana3/Dialog/AddPatientDlg.cpp \
    Havana3/Dialog/IvusViewerDlg.cpp \
    Havana3/Dialog/SettingDlg.cpp \
    Havana3/Dialog/ViewOptionTab.cpp \
    Havana3/Dialog/DeviceOptionTab.cpp \
    Havana3/Dialog/FlimCalibTab.cpp \
    Havana3/Dialog/PulseReviewTab.cpp \
    Havana3/Dialog/ExportDlg.cpp

win32 {
SOURCES += DataAcquisition/SignatecDAQ/SignatecDAQ.cpp \
#    DataAcquisition/AlazarDAQ/AlazarDAQ.cpp \
    DataAcquisition/FLImProcess/FLImProcess.cpp \
    DataAcquisition/OCTProcess/OCTProcess.cpp \
    DataAcquisition/AxsunCapture/AxsunCapture.cpp \
    DataAcquisition/ThreadManager.cpp \
    DataAcquisition/DataAcquisition.cpp \
    DataAcquisition/DataProcessing.cpp \
    DataAcquisition/DataProcessingDotter.cpp
}
macx {
SOURCES += DataAcquisition/FLImProcess/FLImProcess.cpp \
    DataAcquisition/OCTProcess/OCTProcess.cpp \
    DataAcquisition/ThreadManager.cpp \
    DataAcquisition/DataAcquisition.cpp \
    DataAcquisition/DataProcessing.cpp \
    DataAcquisition/DataProcessingDotter.cpp
}

SOURCES += MemoryBuffer/MemoryBuffer.cpp

SOURCES += DeviceControl/FreqDivider/FreqDivider.cpp \
    DeviceControl/PmtGainControl/PmtGainControl.cpp \
    DeviceControl/ElforlightLaser/ElforlightLaser.cpp \
#    DeviceControl/IPGPhotonicsLaser/DigitalInput/DigitalInput.cpp \
#    DeviceControl/IPGPhotonicsLaser/DigitalOutput/DigitalOutput.cpp \
#    DeviceControl/IPGPhotonicsLaser/IPGPhotonicsLaser.cpp \
    DeviceControl/AxsunControl/AxsunControl.cpp \
    DeviceControl/FaulhaberMotor/PullbackMotor.cpp \
    DeviceControl/FaulhaberMotor/RotaryMotor.cpp \
    DeviceControl/DeviceControl.cpp

SOURCES += Common/svm.cpp


HEADERS += Havana3/Configuration.h \
    Havana3/HvnSqlDataBase.h \
    Havana3/MainWindow.h \
    Havana3/QHomeTab.h \
    Havana3/QPatientSelectionTab.h \
    Havana3/QPatientSummaryTab.h \
    Havana3/QViewTab.h \
    Havana3/QStreamTab.h \
    Havana3/QResultTab.h \
    Havana3/Viewer/QScope.h \
    Havana3/Viewer/QImageView.h \
    Havana3/Dialog/AddPatientDlg.h \
    Havana3/Dialog/IvusViewerDlg.h \
    Havana3/Dialog/SettingDlg.h \
    Havana3/Dialog/ViewOptionTab.h \
    Havana3/Dialog/DeviceOptionTab.h \
    Havana3/Dialog/FlimCalibTab.h \
    Havana3/Dialog/PulseReviewTab.h \
    Havana3/Dialog/ExportDlg.h

win32 {
HEADERS += DataAcquisition/SignatecDAQ/SignatecDAQ.h \
#    DataAcquisition/AlazarDAQ/AlazarDAQ.h \    
    DataAcquisition/FLImProcess/FLImProcess.h \
    DataAcquisition/OCTProcess/OCTProcess.cpp \
    DataAcquisition/AxsunCapture/AxsunCapture.h \
    DataAcquisition/ThreadManager.h \
    DataAcquisition/DataAcquisition.h \
    DataAcquisition/DataProcessing.h \
    DataAcquisition/DataProcessingDotter.h
}
macx {
HEADERS += DataAcquisition/FLImProcess/FLImProcess.h \
    DataAcquisition/OCTProcess/OCTProcess.cpp \
    DataAcquisition/ThreadManager.h \
    DataAcquisition/DataAcquisition.h \
    DataAcquisition/DataProcessing.h
}

HEADERS += MemoryBuffer/MemoryBuffer.h

HEADERS += DeviceControl/FreqDivider/FreqDivider.h \
    DeviceControl/PmtGainControl/PmtGainControl.h \
    DeviceControl/ElforlightLaser/ElforlightLaser.h \
#    DeviceControl/IPGPhotonicsLaser/DigitalInput/DigitalInput.h \
#    DeviceControl/IPGPhotonicsLaser/DigitalOutput/DigitalOutput.h \
#    DeviceControl/IPGPhotonicsLaser/IPGPhotonicsLaser.h \
    DeviceControl/AxsunControl/AxsunControl.h \
    DeviceControl/FaulhaberMotor/PullbackMotor.h \
    DeviceControl/FaulhaberMotor/RotaryMotor.h \
    DeviceControl/QSerialComm.h \
    DeviceControl/DeviceControl.h

HEADERS += Common/lumen_detection.h \
    Common/random_forest.h \
    Common/support_vector_machine.h \
    Common/svm.h

FORMS   += Havana3/MainWindow.ui
