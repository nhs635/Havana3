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
class IvusViewerDlg;

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
	void closeEvent(QCloseEvent *);
	void keyPressEvent(QKeyEvent *);

public:
	inline MainWindow* getMainWnd() const { return m_pMainWnd; }
    inline RecordInfo getRecordInfo() { return m_recordInfo; }
    inline DataProcessing* getDataProcessing() { return m_pDataProcessing; }
    inline QViewTab* getViewTab() const { return m_pViewTab; }
	inline SettingDlg* getSettingDlg() const { return m_pSettingDlg; }
	inline IvusViewerDlg* getIvusViewerDlg() const { return m_pIvusViewerDlg; }

private:
    void createResultReviewWidgets();

public:
    void readRecordData();	
	void loadRecordInfo();
	void loadPatientInfo();
	void updatePreviewImage();

private slots:
	void setVisibleState(bool);
    void changeVesselInfo(int);
    void changeProcedureInfo(int);
	void openContainingFolder();
	void createCommentDlg();
	void updateComment();
    void createSettingDlg();
    void deleteSettingDlg();
    void createExportDlg();
    void deleteExportDlg();
	void createIvusViewerDlg();
	void deleteIvusViewerDlg();

signals:
	void getCapture(QByteArray &);

// Variables ////////////////////////////////////////////
private:
	MainWindow* m_pMainWnd;
	Configuration* m_pConfig;
    HvnSqlDataBase* m_pHvnSqlDataBase;

    DataProcessing* m_pDataProcessing;
		
private:
	bool m_bIsDataLoaded;
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
    QPushButton *m_pPushButton_Export;
	QPushButton *m_pPushButton_Setting;
	QPushButton *m_pPushButton_IvusViewer;

    QViewTab* m_pViewTab;

	QProgressBar* m_pProgressBar;
	
    // Dialogs
    SettingDlg *m_pSettingDlg;
    ExportDlg *m_pExportDlg;	
	IvusViewerDlg *m_pIvusViewerDlg;
};

#endif // QRESULTTAB_H
