#ifndef QSCOPE_H
#define QSCOPE_H

#include <QDialog>
#include <QtCore>
#include <QtWidgets>

#include <stdarg.h>

#include <Common/callback.h>

struct QRange {
    double min;
    double max;
};

class QRenderArea;

class QScope : public QDialog
{
    Q_OBJECT

private:
    explicit QScope(QWidget *parent = 0);

public:
    explicit QScope(QRange x_range, QRange y_range,
                    int num_x_ticks = 2, int num_y_ticks = 2,
                    double x_interval = 1, double y_interval = 1, double x_offset = 0, double y_offset = 0,
                    QString x_unit = "", QString y_unit = "", bool mask_use = false, bool _64_use = false, QWidget *parent = 0);
	virtual ~QScope();

private:
	void keyPressEvent(QKeyEvent *e);

public:
	inline QRenderArea* getRender() { return m_pRenderArea; }

public:
	void setAxis(QRange x_range, QRange y_range,
                 int num_x_ticks = 2, int num_y_ticks = 2,
                 double x_interval = 1, double y_interval = 1, double x_offset = 0, double y_offset = 0,
                 QString x_unit = "", QString y_unit = "");
    void resetAxis(QRange x_range, QRange y_range,
                   double x_interval = 1, double y_interval = 1, double x_offset = 0, double y_offset = 0,
                   QString x_unit = "", QString y_unit = "");

public:
	void setWindowLine(int len, ...);
	void setMeanDelayLine(int len, ...);
	void setDcLine(float dcLine);

public slots:
    void drawData(const float* pData);
	void drawData(const float* pData, const float* pDataX, bool foo);
	void drawData(const float* pData, const float* pMask);
	void drawData(const double* pData64);

private:
    QGridLayout *m_pGridLayout;

    QRenderArea *m_pRenderArea;

    QVector<QLabel *> m_pLabelVector_x;
    QVector<QLabel *> m_pLabelVector_y;
};

class QRenderArea : public QWidget
{
    Q_OBJECT

public:
    explicit QRenderArea(QWidget *parent = 0);
	virtual ~QRenderArea();

protected:
    void paintEvent(QPaintEvent *);

	void mousePressEvent(QMouseEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void mouseReleaseEvent(QMouseEvent *);

public:
	void setSize(QRange xRange, QRange yRange, int len = 0);
	void setGrid(int nHMajorGrid, int nHMinorGrid, int nVMajorGrid, bool zeroLine = false);

public: // callback
	callback<void> DidMouseEvent;

public:
    float* m_pData;
	float* m_pDataX;
	float* m_pMask;
	double* m_pData64;
	bool m_bMaskUse;
	bool m_b64Use;

    QRange m_xRange;
    QRange m_yRange;
    QSizeF m_sizeGraph;
	int m_buff_len;

	int *m_pWinLineInd;
	float *m_pMdLineInd;
	int m_winLineLen;
	int m_mdLineLen;
	float m_dcLine;

	int m_nHMajorGrid;
	int m_nHMinorGrid;
	int m_nVMajorGrid;

	bool m_bZeroLine;

	bool m_bSelectionAvailable;
	bool m_bMousePressed;
	bool m_bIsLeftButton;
	int m_selected[2];
	int m_start, m_end;
	uint8_t *m_pSelectedRegion;

	bool m_bScattered;
};


#endif // QSCOPE_H
