// src/CondFormatExpr.h
// bk1

#ifndef CONDFORMATEXPR_H
#define CONDFORMATEXPR_H

#include <QString>

class QAbstractTableModel;
class QModelIndex;

class CondFormatExpr
{
public:
    static bool isExpression(const QString& expression);

    static bool evaluate(
        const QString& expression,
        const QAbstractTableModel* model,
        const QModelIndex& index);

private:
    static bool evaluateComparison(
        const QString& expression,
        const QAbstractTableModel* model,
        const QModelIndex& index);

    static bool evaluateDuplicate(
        const QString& expression,
        const QAbstractTableModel* model,
        const QModelIndex& index);

    static bool evaluateFirstDuplicate(
        const QString& expression,
        const QAbstractTableModel* model,
        const QModelIndex& index);

    static int findColumn(
        const QString& columnName,
        const QAbstractTableModel* model);
};

#endif // CONDFORMATEXPR_H