#ifndef MYSORTFILTERPROXYMODEL_H
#define MYSORTFILTERPROXYMODEL_H

#include "jsontablemodel.h"
#include <QModelIndex>
#include <QObject>
#include <QSortFilterProxyModel>

class MySortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

    public:
    MySortFilterProxyModel(QObject *parent = 0);

    public slots:
    void setFilterString(QString filterString);
    QString filterString() const;

    protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    private:
    QString filterString_;
};

#endif // MYSORTFILTERPROXYMODEL_H
