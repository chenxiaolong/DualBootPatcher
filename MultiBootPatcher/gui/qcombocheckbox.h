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

#ifndef QCOMBOCHECKBOX_H
#define QCOMBOCHECKBOX_H

#include <QtWidgets/QComboBox>
#include <QtCore/QEvent>
#include <QtWidgets/QStylePainter>

class QComboCheckBoxPrivate;

class QComboCheckBox : public QComboBox
{
    Q_OBJECT

public:
    QComboCheckBox(QWidget *widget = 0);
    virtual ~QComboCheckBox();

    bool eventFilter(QObject *object, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    void addItem(const QString &text, const QVariant &userData = QVariant());
    void addItems(const QStringList &texts);

    bool isChecked(int index) const;
    void setChecked(int index, bool checked);

    QString displayText() const;
    void setDisplayText(const QString &text);

    bool autoUpdateDisplayText() const;
    void setAutoUpdateDisplayText(bool y);

    QString nothingSelectedText() const;
    void setNothingSelectedText(const QString &text);

private slots:
    void updateItems();

signals:
    void currentItemsUpdated();

private:
    const QScopedPointer<QComboCheckBoxPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QComboCheckBox)
};

#endif // QCOMBOCHECKBOX_H
