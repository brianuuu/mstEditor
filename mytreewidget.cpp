#include "mytreewidget.h"

MyTreeWidget::MyTreeWidget(QWidget *parent) : QTreeWidget(parent)
{

}

void MyTreeWidget::dropEvent(QDropEvent *event)
{
    // get the list of the items that are about to be dragged
    QList<QTreeWidgetItem*> dragItems = selectedItems();

    // find out their row numbers before the drag
    QList<int> fromRows;
    QTreeWidgetItem *item;
    foreach(item, dragItems) fromRows.append(indexFromItem(item).row());

    // the default implementation takes care of the actual move inside the tree
    QTreeWidget::dropEvent(event);

    // query the indices of the dragged items again
    QList<int> toRows;
    foreach(item, dragItems) toRows.append(indexFromItem(item).row());

    if (fromRows.size() == 1 && toRows.size() == 1)
    {
        // notify subscribers in some useful way
        emit itemMoved(fromRows.front(), toRows.front());
    }
}
