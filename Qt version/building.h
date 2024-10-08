#ifndef BUILDING_H
#define BUILDING_H

#include "qwidget.h"

QT_BEGIN_NAMESPACE
class QScrollArea;
class QScrollBar;
QT_END_NAMESPACE


class buildingwindow : public QWidget
{
    Q_OBJECT

private:

public slots:

protected:
    void mousePressEvent (QMouseEvent *event) override;


};

#endif // BUILDING_H
