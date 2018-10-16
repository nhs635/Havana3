#ifndef QSCOPE2_H
#define QSCOPE2_H

#include <QDialog>
#include <QtCore>
#include <QtWidgets>

#include "QScope.h"

class QRenderArea2;

class QScope2 : public QDialog
{
    Q_OBJECT

private:
    explicit QScope2(QWidget *parent = 0);

public:
    explicit QScope2(QRange x_range, QRange y_range,
                    int num_x_ticks = 2, int num_y_ticks = 2,
                    double x_interval = 1, double y_interval = 1, double x_offset = 0, double y_offset = 0,
                    QString x_unit = "", QString y_unit = "", QWidget *parent = 0);
	virtual ~QScope2();

public:
	void setAxis(QRange x_range, QRange y_range,
                 int num_x_ticks = 2, int num_y_ticks = 2,
                 double x_interval = 1, double y_interval = 1, double x_offset = 0, double y_offset = 0,
                 QString x_unit = "", QString y_unit = "");
    void resetAxis(QRange x_range, QRange y_range,
                   double x_interval = 1, double y_interval = 1, double x_offset = 0, double y_offset = 0,
                   QString x_unit = "", QString y_unit = "");

public slots:
    void drawData(float* pData1, float* pData2);

private:
    QGridLayout *m_pGridLayout;

    QRenderArea2 *m_pRenderArea;

    QVector<QLabel *> m_pLabelVector_x;
    QVector<QLabel *> m_pLabelVector_y;
};

class QRenderArea2 : public QWidget
{
    Q_OBJECT

public:
    explicit QRenderArea2(QWidget *parent = 0);

protected:
    void paintEvent(QPaintEvent *);

public:
    float* m_pData1;
	float* m_pData2;

    QRange m_xRange;
    QRange m_yRange;
    QSizeF m_sizeGraph;
};


#endif // QSCOPE_H
