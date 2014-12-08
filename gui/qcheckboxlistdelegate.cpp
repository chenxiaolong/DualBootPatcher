/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "qcheckboxlistdelegate.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>


QCheckBoxListDelegate::QCheckBoxListDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}


void QCheckBoxListDelegate::paint(QPainter *painter,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
    bool checked = index.data(Qt::UserRole).toBool();
    QString text = index.data(Qt::DisplayRole).toString();

    QStyleOptionButton buttonOption;
    buttonOption.state |= checked ? QStyle::State_On : QStyle::State_Off;
    buttonOption.state |= QStyle::State_Enabled;
    buttonOption.text = text;
    buttonOption.rect = option.rect;

    QApplication::style()->drawControl(
            QStyle::CE_CheckBox, &buttonOption, painter);
}

QWidget * QCheckBoxListDelegate::createEditor(QWidget *parent,
                                              const QStyleOptionViewItem &option,
                                              const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    return new QCheckBox(parent);
}

void QCheckBoxListDelegate::setEditorData(QWidget *editor,
                                          const QModelIndex &index) const
{
    if (QCheckBox *checkBox = qobject_cast<QCheckBox *>(editor)) {
        checkBox->setText(index.data(Qt::DisplayRole).toString());
        checkBox->setChecked(index.data(Qt::UserRole).toBool());
    } else {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void QCheckBoxListDelegate::setModelData(QWidget *editor,
                                         QAbstractItemModel *model,
                                         const QModelIndex &index) const
{
    if (QCheckBox *checkBox = qobject_cast<QCheckBox *>(editor)) {
        QMap<int, QVariant> data;
        data.insert(Qt::DisplayRole, checkBox->text());
        data.insert(Qt::UserRole, checkBox->isChecked());
        model->setItemData(index, data);
    } else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

void QCheckBoxListDelegate::updateEditorGeometry(QWidget *editor,
                                                 const QStyleOptionViewItem &option,
                                                 const QModelIndex &index) const
{
    Q_UNUSED(index);

    editor->setGeometry(option.rect);
}
