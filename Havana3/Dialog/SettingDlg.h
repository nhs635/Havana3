#ifndef SETTINGDLG_H
#define SETTINGDLG_H

#include <QtWidgets>
#include <QtCore>


class Configuration;
class QStreamTab;
class QResultTab;
class QViewTab;

class ViewOptionTab;
class DeviceOptionTab;
class FlimCalibTab;
class PulseReviewTab;

class CustomTabStyle : public QProxyStyle
{
public:
    QSize sizeFromContents(ContentsType type, const QStyleOption* option,
                           const QSize& size, const QWidget* widget) const
    {
        QSize s = QProxyStyle::sizeFromContents(type, option, size, widget);
        if (type == QStyle::CT_TabBarTab)
            s.transpose();
        return s;
    }

    void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
    {
        if (element == CE_TabBarTabLabel)
        {
            if (const QStyleOptionTab* tab = qstyleoption_cast<const QStyleOptionTab*>(option))
            {
                QStyleOptionTab opt(*tab);
                opt.shape = QTabBar::RoundedNorth;
                QProxyStyle::drawControl(element, &opt, painter, widget);
                return;
            }
        }
        QProxyStyle::drawControl(element, option, painter, widget);
    }
};

class SettingDlg : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit SettingDlg(bool is_streaming = true, QWidget *parent = 0);
    virtual ~SettingDlg();

// Methods //////////////////////////////////////////////
private:
	void keyPressEvent(QKeyEvent *e);

public:
    inline QStreamTab* getStreamTab() const { return m_pStreamTab; }
    inline QResultTab* getResultTab() const { return m_pResultTab; }
    inline ViewOptionTab* getViewOptionTab() const { return m_pViewOptionTab; }
    inline DeviceOptionTab* getDeviceOptionTab() const { return m_pDeviceOptionTab; }
    inline FlimCalibTab* getFlimCalibTab() const { return m_pFlimCalibTab; }
    inline PulseReviewTab* getPulseReviewTab() const { return m_pPulseReviewTab; }

// Variables ////////////////////////////////////////////
private:	
	Configuration* m_pConfig;
    QStreamTab* m_pStreamTab;
    QResultTab* m_pResultTab;
    QViewTab* m_pViewTab;

private:
    QTabWidget *m_pTabWidget_Setting;
	
    ViewOptionTab *m_pViewOptionTab;
    DeviceOptionTab *m_pDeviceOptionTab;
    FlimCalibTab *m_pFlimCalibTab;
    PulseReviewTab *m_pPulseReviewTab;

    QPushButton *m_pPushButton_Ok;
    QPushButton *m_pPushButton_Cancel;
};

#endif // PULSEREVIEWDLG_H
