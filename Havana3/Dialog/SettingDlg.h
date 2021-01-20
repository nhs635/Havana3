#ifndef SETTINGDLG_H
#define SETTINGDLG_H

#include <QtWidgets>
#include <QtCore>


class Configuration;
class QPatientSummaryTab;
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
    explicit SettingDlg(QWidget *parent = 0);
    virtual ~SettingDlg();

// Methods //////////////////////////////////////////////
private:
	void keyPressEvent(QKeyEvent *e);

public:
	inline QPatientSummaryTab* getPatientSummaryTab() const { return m_pPatientSummaryTab; }
    inline QStreamTab* getStreamTab() const { return m_pStreamTab; }
    inline QResultTab* getResultTab() const { return m_pResultTab; }
	inline QTabWidget* getTabWidget() const { return m_pTabWidget_Setting; }
    inline ViewOptionTab* getViewOptionTab() const { return m_pViewOptionTab; }
    inline DeviceOptionTab* getDeviceOptionTab() const { return m_pDeviceOptionTab; }
    inline FlimCalibTab* getFlimCalibTab() const { return m_pFlimCalibTab; }
    inline PulseReviewTab* getPulseReviewTab() const { return m_pPulseReviewTab; }

// Variables ////////////////////////////////////////////
private:	
	Configuration* m_pConfig;
	QPatientSummaryTab* m_pPatientSummaryTab;
    QStreamTab* m_pStreamTab;
    QResultTab* m_pResultTab;
    QViewTab* m_pViewTab;

private:
	CustomTabStyle *m_pCusomTabstyle;
    QTabWidget *m_pTabWidget_Setting;
	
    ViewOptionTab *m_pViewOptionTab;
    DeviceOptionTab *m_pDeviceOptionTab;
    FlimCalibTab *m_pFlimCalibTab;
    PulseReviewTab *m_pPulseReviewTab;

	QListWidget *m_pListWidget_Log;

    QPushButton *m_pPushButton_Close;
};

#endif // PULSEREVIEWDLG_H
