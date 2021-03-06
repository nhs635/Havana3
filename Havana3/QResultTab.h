#ifndef QRESULTTAB_H
#define QRESULTTAB_H

#include <QtWidgets>
#include <QtCore>

#include <Havana3/QPatientSummaryTab.h>


class MainWindow;
class Configuration;
class HvnSqlDataBase;

class QViewTab;
class SettingDlg;
class ExportDlg;

class DataProcessing;

class QResultTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QResultTab(QString record_id, QWidget *parent = nullptr);
	virtual ~QResultTab();

// Methods //////////////////////////////////////////////
protected:
	void keyPressEvent(QKeyEvent *);

public:
	inline MainWindow* getMainWnd() const { return m_pMainWnd; }
    inline RecordInfo getRecordInfo() { return m_recordInfo; }
    inline DataProcessing* getDataProcessing() { return m_pDataProcessing; }
    inline QViewTab* getViewTab() const { return m_pViewTab; }

private:
    void createResultReviewWidgets();

public:
    void readRecordData();	
	void loadRecordInfo();
	void loadPatientInfo();
	void updatePreviewImage();

private slots:
    void changeVesselInfo(int);
    void changeProcedureInfo(int);
	void openContainingFolder();
	void createCommentDlg();
	void updateComment();
    void createSettingDlg();
    void deleteSettingDlg();
    void createExportDlg();
    void deleteExportDlg();

signals:
	void getCapture(QByteArray &);

// Variables ////////////////////////////////////////////
private:
	MainWindow* m_pMainWnd;
	Configuration* m_pConfig;
    HvnSqlDataBase* m_pHvnSqlDataBase;

    DataProcessing* m_pDataProcessing;
		
private:
    RecordInfo m_recordInfo;

private:
    // Review tab control widget
    QGroupBox *m_pGroupBox_ResultReview;

    QLabel *m_pLabel_PatientInformation;
    QLabel *m_pLabel_RecordInformation;
    QComboBox *m_pComboBox_Vessel;
    QComboBox *m_pComboBox_Procedure;

	QPushButton *m_pPushButton_OpenFolder;
	QPushButton *m_pPushButton_Comment;
    QPushButton *m_pPushButton_Setting;
    QPushButton *m_pPushButton_Export;

    QViewTab* m_pViewTab;

    // Dialogs
    SettingDlg *m_pSettingDlg;
    ExportDlg *m_pExportDlg;
};

#endif // QRESULTTAB_H
