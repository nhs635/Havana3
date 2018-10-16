#ifndef QPROCESSINGTAB_H
#define QPROCESSINGTAB_H

#include <QDialog>
#include <QtWidgets>
#include <QtCore>

#include <Common/array.h>
#include <Common/callback.h>
#include <Common/SyncObject.h>

class QResultTab;
class Configuration;

class PulseReviewDlg;
class SaveResultDlg;

class FLImProcess;


class QProcessingTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QProcessingTab(QWidget *parent = nullptr);
    virtual ~QProcessingTab();

// Methods //////////////////////////////////////////////
public:
    inline QVBoxLayout* getLayout() const { return m_pVBoxLayout; }
    inline QResultTab* getResultTab() const { return m_pResultTab; }
	inline FLImProcess* getFLImProcess() const { return m_pFLIm; }
	inline SaveResultDlg* getSaveResultDlg() const { return m_pSaveResultDlg; }
	inline PulseReviewDlg* getPulseReviewDlg() const { return m_pPulseReviewDlg; }
	inline QProgressBar* getProgressBar() const { return m_pProgressBar_PostProcessing; }
	
public:
	void setWidgetsStatus();

private slots: 
	void createSaveResultDlg();
	void deleteSaveResultDlg();
	void createPulseReviewDlg();
	void deletePulseReviewDlg();

	///void enableUserDefinedAlines(bool);
	void setWidgetsEnabled(bool enabled, Configuration* pConfig);

	void startProcessing();
	
signals:
	void processedSingleFrame(int);
	void setWidgets(bool enabled, Configuration* pConfig);
	
private: 
	void loadingRawData(QFile*, Configuration*);
	void deinterleaving(Configuration*);
	void flimProcessing(FLImProcess*, Configuration*);

private:
	void getOctProjection(std::vector<np::Uint8Array2>& vecImg, np::Uint8Array2& octProj, int offset = 0);

// Variables ////////////////////////////////////////////
private:	
    QResultTab* m_pResultTab;
    Configuration* m_pConfig;
	FLImProcess* m_pFLIm;
	
	SaveResultDlg *m_pSaveResultDlg;
	PulseReviewDlg *m_pPulseReviewDlg;
	
public:
	QString m_path;

private: // for threading operation
	SyncObject<uint8_t> m_syncDeinterleaving;
	SyncObject<uint16_t> m_syncFlimProcessing;
	
private:
	// Layout
	QVBoxLayout *m_pVBoxLayout;
	
	// Data loading & writing widgets
	QPushButton *m_pPushButton_StartProcessing;
	QPushButton *m_pPushButton_SaveResults;
	QPushButton *m_pPushButton_PulseReview;

	///QCheckBox *m_pCheckBox_UserDefinedAlines;
	///QLineEdit *m_pLineEdit_UserDefinedAlines;

	QProgressBar *m_pProgressBar_PostProcessing;
};

#endif // QOPERATIONTAB_H
