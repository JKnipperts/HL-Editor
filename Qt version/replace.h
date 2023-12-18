#ifndef REPLACE_H
#define REPLACE_H

#include "qwidget.h"

QT_BEGIN_NAMESPACE
class QScrollArea;
class QScrollBar;
QT_END_NAMESPACE


class replacewindow : public QWidget
{
    Q_OBJECT

private:

public slots:

protected:
    void mousePressEvent (QMouseEvent *event) override;
    void closeEvent (QCloseEvent *event) override;


};

#endif // REPLACE_H
