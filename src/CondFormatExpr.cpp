// src/CondFormatExpr.cpp
// bk3

#include "CondFormatExpr.h"

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QVariant>


bool CondFormatExpr::isExpression(const QString& expression)
{
    return expression.trimmed().startsWith("expr:", Qt::CaseInsensitive);
}


bool CondFormatExpr::evaluate(
    const QString& expression,
    const QAbstractTableModel* model,
    const QModelIndex& index)
{
    if (!model || !index.isValid())
        return false;

    QString expr = expression.trimmed();

    if (!isExpression(expr))
        return false;

    // Remove "expr:"
    expr = expr.mid(5).trimmed();

    // NOT(expression)
    if (expr.startsWith("NOT(", Qt::CaseInsensitive))
        return evaluateNot(expr, model, index);

    // DUPLICATE([column])
    if (expr.startsWith("DUPLICATE(", Qt::CaseInsensitive))
        return evaluateDuplicate(expr, model, index);

    // FIRST_DUPLICATE([column])
    if (expr.startsWith("FIRST_DUPLICATE(", Qt::CaseInsensitive))
        return evaluateFirstDuplicate(expr, model, index);

    // CONTAINS([column], 'value')
    if (expr.startsWith("CONTAINS(", Qt::CaseInsensitive))
        return evaluateContains(expr, model, index);

    // STARTS_WITH([column], 'value')
    if (expr.startsWith("STARTS_WITH(", Qt::CaseInsensitive))
        return evaluateStartsWith(expr, model, index);

    // [column] operator value
    return evaluateComparison(expr, model, index);
}

// [列名] の列番号を取得
int CondFormatExpr::findColumn(
    const QString& columnName,
    const QAbstractTableModel* model)
{
    if (!model)
        return -1;

    for (int column = 0; column < model->columnCount(); ++column)
    {
        QVariant header = model->headerData(
            column,
            Qt::Horizontal,
            Qt::EditRole);

        if (header.toString().compare(columnName, Qt::CaseInsensitive) == 0)
            return column;
    }

    return -1;
}

// DUPLICATE([列名])
bool CondFormatExpr::evaluateDuplicate(
    const QString& expression,
    const QAbstractTableModel* model,
    const QModelIndex& index)
{
    const QString prefix = "DUPLICATE(";
    const QString suffix = ")";

    if (!expression.startsWith(prefix, Qt::CaseInsensitive) ||
        !expression.endsWith(suffix))
    {
        return false;
    }

    QString columnName = expression.mid(
        prefix.length(),
        expression.length() - prefix.length() - suffix.length());

    columnName = columnName.trimmed();

    // [column] の形式でなければ無効
    if (!columnName.startsWith('[') ||
        !columnName.endsWith(']'))
    {
        return false;
    }

    // [ と ] を削除
    columnName = columnName.mid(
        1,
        columnName.length() - 2);

    columnName = columnName.trimmed();

    if (columnName.isEmpty())
        return false;

    int column = findColumn(columnName, model);

    if (column < 0)
        return false;

    // 現在行の値
    QVariant currentValue = model->data(
        model->index(index.row(), column),
        Qt::EditRole);

    // NULL は重複判定の対象外
    if (!currentValue.isValid() || currentValue.isNull())
        return false;

    // 空文字も重複判定の対象外
    if (currentValue.toString().isEmpty())
        return false;

    // 同じ列の他の行を検索
    for (int row = 0; row < model->rowCount(); ++row)
    {
        // 自分自身は除外
        if (row == index.row())
            continue;

        QVariant value = model->data(
            model->index(row, column),
            Qt::EditRole);

        if (value == currentValue)
            return true;
    }

    return false;
}

// FIRST_DUPLICATE([列名])
bool CondFormatExpr::evaluateFirstDuplicate(
    const QString& expression,
    const QAbstractTableModel* model,
    const QModelIndex& index)
{
    const QString prefix = "FIRST_DUPLICATE(";
    const QString suffix = ")";

    if (!expression.startsWith(prefix, Qt::CaseInsensitive) ||
        !expression.endsWith(suffix))
    {
        return false;
    }

    QString columnName = expression.mid(
        prefix.length(),
        expression.length() - prefix.length() - suffix.length());

    columnName = columnName.trimmed();

    // [column] の形式でなければ無効
    if (!columnName.startsWith('[') ||
        !columnName.endsWith(']'))
    {
        return false;
    }

    // [ と ] を削除
    columnName = columnName.mid(
        1,
        columnName.length() - 2);

    columnName = columnName.trimmed();

    if (columnName.isEmpty())
        return false;

    int column = findColumn(columnName, model);

    if (column < 0)
        return false;

    // 現在行の値
    QVariant currentValue = model->data(
        model->index(index.row(), column),
        Qt::EditRole);

    // NULL は対象外
    if (!currentValue.isValid() || currentValue.isNull())
        return false;

    // 空文字も対象外
    if (currentValue.toString().isEmpty())
        return false;

    // 現在行より前に同じ値が存在するか確認
    for (int row = 0; row < index.row(); ++row)
    {
        QVariant value = model->data(
            model->index(row, column),
            Qt::EditRole);

        if (value == currentValue)
            return false;
    }

    // 現在行より後ろに同じ値が存在するか確認
    for (int row = index.row() + 1; row < model->rowCount(); ++row)
    {
        QVariant value = model->data(
            model->index(row, column),
            Qt::EditRole);

        if (value == currentValue)
            return true;
    }

    return false;
}

// NOT(expression)
bool CondFormatExpr::evaluateNot(
    const QString& expression,
    const QAbstractTableModel* model,
    const QModelIndex& index)
{
    const QString prefix = "NOT(";
    const QString suffix = ")";

    if (!expression.startsWith(prefix, Qt::CaseInsensitive) ||
        !expression.endsWith(suffix))
    {
        return false;
    }

    QString innerExpression = expression.mid(
        prefix.length(),
        expression.length() - prefix.length() - suffix.length());

    innerExpression = innerExpression.trimmed();

    if (innerExpression.isEmpty())
        return false;

    return !evaluate("expr:" + innerExpression, model, index);
}

// CONTAINS([列名], '文字列')
bool CondFormatExpr::evaluateContains(
    const QString& expression,
    const QAbstractTableModel* model,
    const QModelIndex& index)
{
    const QString prefix = "CONTAINS(";
    const QString suffix = ")";

    if (!expression.startsWith(prefix, Qt::CaseInsensitive) ||
        !expression.endsWith(suffix))
    {
        return false;
    }

    QString arguments = expression.mid(
        prefix.length(),
        expression.length() - prefix.length() - suffix.length());

    arguments = arguments.trimmed();

    // カンマで列名と検索文字列を分離
    int comma = arguments.indexOf(',');

    if (comma < 0)
        return false;

    QString columnName = arguments.left(comma).trimmed();
    QString condition = arguments.mid(comma + 1).trimmed();

    // [column] の形式でなければ無効
    if (!columnName.startsWith('[') ||
        !columnName.endsWith(']'))
    {
        return false;
    }

    // [ と ] を削除
    columnName = columnName.mid(
        1,
        columnName.length() - 2);

    columnName = columnName.trimmed();

    if (columnName.isEmpty())
        return false;

    // 'value' の形式でなければ無効
    if (condition.length() < 2 ||
        !condition.startsWith('\'') ||
        !condition.endsWith('\''))
    {
        return false;
    }

    condition = condition.mid(
        1,
        condition.length() - 2);

    // SQL-style escaped single quote
    condition.replace("''", "'");

    int column = findColumn(columnName, model);

    if (column < 0)
        return false;

    QVariant currentValue = model->data(
        model->index(index.row(), column),
        Qt::DisplayRole);

    // NULL は対象外
    if (!currentValue.isValid() || currentValue.isNull())
        return false;

    QString current = currentValue.toString();

    return current.contains(condition);
}

// STARTS_WITH([列名], '文字列')
bool CondFormatExpr::evaluateStartsWith(
    const QString& expression,
    const QAbstractTableModel* model,
    const QModelIndex& index)
{
    const QString prefix = "STARTS_WITH(";
    const QString suffix = ")";

    if (!expression.startsWith(prefix, Qt::CaseInsensitive) ||
        !expression.endsWith(suffix))
    {
        return false;
    }

    QString arguments = expression.mid(
        prefix.length(),
        expression.length() - prefix.length() - suffix.length());

    arguments = arguments.trimmed();

    // カンマで列名と検索文字列を分離
    int comma = arguments.indexOf(',');

    if (comma < 0)
        return false;

    QString columnName = arguments.left(comma).trimmed();
    QString condition = arguments.mid(comma + 1).trimmed();

    // [column] の形式でなければ無効
    if (!columnName.startsWith('[') ||
        !columnName.endsWith(']'))
    {
        return false;
    }

    // [ と ] を削除
    columnName = columnName.mid(
        1,
        columnName.length() - 2);

    columnName = columnName.trimmed();

    if (columnName.isEmpty())
        return false;

    // 'value' の形式でなければ無効
    if (condition.length() < 2 ||
        !condition.startsWith('\'') ||
        !condition.endsWith('\''))
    {
        return false;
    }

    condition = condition.mid(
        1,
        condition.length() - 2);

    // SQL-style escaped single quote
    condition.replace("''", "'");

    int column = findColumn(columnName, model);

    if (column < 0)
        return false;

    QVariant currentValue = model->data(
        model->index(index.row(), column),
        Qt::DisplayRole);

    // NULL は対象外
    if (!currentValue.isValid() || currentValue.isNull())
        return false;

    QString current = currentValue.toString();

    return current.startsWith(condition);
}

// 列の値を比較
bool CondFormatExpr::evaluateComparison(
    const QString& expression,
    const QAbstractTableModel* model,
    const QModelIndex& index)
{
    if (!model || !index.isValid())
        return false;

    QString expr = expression.trimmed();

    /*
     * [column] operator value
     *
     * Examples:
     *
     * [品詞] = '名詞'
     * [出現回数] >= 2
     * [出現回数] < 10
     * [品詞] != '名詞'
     */

    // Find the closing bracket of [column]
    if (!expr.startsWith('['))
        return false;

    int closeBracket = expr.indexOf(']');

    if (closeBracket < 0)
        return false;

    QString columnName = expr.mid(
        1,
        closeBracket - 1);

    columnName = columnName.trimmed();

    if (columnName.isEmpty())
        return false;

    int column = findColumn(columnName, model);

    if (column < 0)
        return false;

    // The part after [column]
    QString condition = expr.mid(closeBracket + 1).trimmed();

    if (condition.isEmpty())
        return false;

    // Determine comparison operator.
    QString op;

    const QStringList operators = {
        ">=",
        "<=",
        "!=",
        "<>",
        "=",
        ">",
        "<"
    };

    for (const QString& candidate : operators)
    {
        if (condition.startsWith(candidate))
        {
            op = candidate;
            condition = condition.mid(candidate.length()).trimmed();
            break;
        }
    }

    if (op.isEmpty() || condition.isEmpty())
        return false;

    // Remove surrounding single quotes from string values.
    bool quoted = false;

    if (condition.length() >= 2 &&
        condition.startsWith('\'') &&
        condition.endsWith('\''))
    {
        quoted = true;
        condition = condition.mid(
            1,
            condition.length() - 2);

        // SQL-style escaped single quote:
        // '名''詞' -> 名'詞
        condition.replace("''", "'");
    }

    QVariant currentValue = model->data(
        model->index(index.row(), column),
        Qt::DisplayRole);

    if (!currentValue.isValid())
        return false;

    // NULL comparison
    if (condition.compare("NULL", Qt::CaseInsensitive) == 0 &&
        !quoted)
    {
        bool isNull = currentValue.isNull();

        if (op == "=")
            return isNull;

        if (op == "!=" || op == "<>")
            return !isNull;

        return false;
    }

    /*
     * If the value is quoted, compare as a string.
     */
    if (quoted)
    {
        QString current = currentValue.toString();

        if (op == "=")
            return current == condition;

        if (op == "!=" || op == "<>")
            return current != condition;

        if (op == ">")
            return current > condition;

        if (op == ">=")
            return current >= condition;

        if (op == "<")
            return current < condition;

        if (op == "<=")
            return current <= condition;

        return false;
    }

    /*
     * Otherwise try numeric comparison first.
     */
    bool conditionOk = false;
    double conditionNumber = condition.toDouble(&conditionOk);

    bool currentOk = false;
    double currentNumber = currentValue.toString().toDouble(&currentOk);

    if (conditionOk && currentOk)
    {
        if (op == "=")
            return currentNumber == conditionNumber;

        if (op == "!=" || op == "<>")
            return currentNumber != conditionNumber;

        if (op == ">")
            return currentNumber > conditionNumber;

        if (op == ">=")
            return currentNumber >= conditionNumber;

        if (op == "<")
            return currentNumber < conditionNumber;

        if (op == "<=")
            return currentNumber <= conditionNumber;

        return false;
    }

    /*
     * If it is not numeric, fall back to string comparison.
     */
    QString current = currentValue.toString();

    if (op == "=")
        return current == condition;

    if (op == "!=" || op == "<>")
        return current != condition;

    if (op == ">")
        return current > condition;

    if (op == ">=")
        return current >= condition;

    if (op == "<")
        return current < condition;

    if (op == "<=")
        return current <= condition;

    return false;
}