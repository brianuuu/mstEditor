#ifndef MYTREEWIDGET_H
#define MYTREEWIDGET_H

#include <QTreeWidget>
#include <QDebug>

class MyTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit MyTreeWidget(QWidget* parent = nullptr);
    void dropEvent(QDropEvent *event) override;

signals:
    void itemMoved(int from, int to);
};

#endif // MYTREEWIDGET_H
