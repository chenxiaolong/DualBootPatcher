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

#include "qcombocheckbox.h"
#include "qcombocheckbox_p.h"

#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QCheckBox>


QComboCheckBox::QComboCheckBox(QWidget *widget)
    : QComboBox(widget), d_ptr(new QComboCheckBoxPrivate())
{
    Q_D(QComboCheckBox);

    d->autoUpdateDisplayText = false;
    d->nothingSelectedText = tr("None");
    d->delegate = new QCheckBoxListDelegate(this);

    view()->setItemDelegate(d->delegate);
    view()->setEditTriggers(QAbstractItemView::CurrentChanged);
    view()->viewport()->installEventFilter(this);

    connect(d->delegate, &QCheckBoxListDelegate::closeEditor,
            this, &QComboCheckBox::updateItems);
}

QComboCheckBox::~QComboCheckBox()
{
}

bool QComboCheckBox::eventFilter(QObject *object, QEvent *event)
{
    // Ignore mouse release after clicking in the popup to prevent it from
    // begin closed
    if (event->type() == QEvent::MouseButtonRelease
            && object == view()->viewport()) {
        return true;
    }

    return QComboBox::eventFilter(object, event);
}

void QComboCheckBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    Q_D(QComboCheckBox);

    // Copied from QCheckBox source, but edited to set the text in the combobox
    QStylePainter painter(this);
    painter.setPen(palette().color(QPalette::Text));

    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    opt.currentText = d->displayText;

    painter.drawComplexControl(QStyle::CC_ComboBox, opt);
    painter.drawControl(QStyle::CE_ComboBoxLabel, opt);
}

void QComboCheckBox::addItem(const QString& text, const QVariant& userData)
{
    Q_D(QComboCheckBox);

    QComboBox::addItem(text, userData);

    updateItems();
}

void QComboCheckBox::addItems(const QStringList& texts)
{
    Q_D(QComboCheckBox);

    QComboBox::addItems(texts);

    updateItems();
}

bool QComboCheckBox::isChecked(int index) const
{
    Q_D(const QComboCheckBox);

    return itemData(index).toBool();
}

void QComboCheckBox::setChecked(int index, bool checked)
{
    Q_D(QComboCheckBox);

    setItemData(index, QVariant(checked));

    updateItems();
}

QString QComboCheckBox::displayText() const
{
    Q_D(const QComboCheckBox);

    return d->displayText;
}

void QComboCheckBox::setDisplayText(const QString &text)
{
    Q_D(QComboCheckBox);

    if (d->autoUpdateDisplayText) {
        qWarning("Display text for QComboCheckBox was set when "
                 "autoUpdateDisplayText was enabled");
    }

    d->displayText = text;
}

bool QComboCheckBox::autoUpdateDisplayText() const
{
    Q_D(const QComboCheckBox);

    return d->autoUpdateDisplayText;
}

void QComboCheckBox::setAutoUpdateDisplayText(bool y)
{
    Q_D(QComboCheckBox);

    d->autoUpdateDisplayText = y;

    if (y) {
        updateItems();
    }
}

QString QComboCheckBox::nothingSelectedText() const
{
    Q_D(const QComboCheckBox);

    return d->nothingSelectedText;
}

void QComboCheckBox::setNothingSelectedText(const QString &text)
{
    Q_D(QComboCheckBox);

    d->nothingSelectedText = text;
}

void QComboCheckBox::updateItems()
{
    Q_D(QComboCheckBox);

    if (d->autoUpdateDisplayText) {
        QStringList selectedItems;

        for (int i = 0; i < count(); i++) {
            QString text = itemText(i);
            bool checked = itemData(i).toBool();

            if (checked) {
                selectedItems << text;
            }
        }

        if (!selectedItems.isEmpty()) {
            d->displayText = selectedItems.join(QStringLiteral(", "));
        } else {
            d->displayText = d->nothingSelectedText;
        }
    }

    emit currentItemsUpdated();
}
