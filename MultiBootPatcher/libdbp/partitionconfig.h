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

#ifndef PARTITIONCONFIG_H
#define PARTITIONCONFIG_H

#include "libdbp_global.h"

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QString>

class PartitionConfigPrivate;

class LIBDBPSHARED_EXPORT PartitionConfig : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QString description READ description WRITE setDescription)
    Q_PROPERTY(QString kernel READ kernel WRITE setKernel)
    Q_PROPERTY(QString id READ id WRITE setId)
    Q_PROPERTY(QString targetSystem READ targetSystem WRITE setTargetSystem)
    Q_PROPERTY(QString targetCache READ targetCache WRITE setTargetCache)
    Q_PROPERTY(QString targetData READ targetData WRITE setTargetData)
    Q_PROPERTY(QString targetSystemPartition READ targetSystemPartition WRITE setTargetSystemPartition)
    Q_PROPERTY(QString targetCachePartition READ targetCachePartition WRITE setTargetCachePartition)
    Q_PROPERTY(QString targetDataPartition READ targetDataPartition WRITE setTargetDataPartition)

    static const QString ReplaceLine;

    // Device and mount point for the system, cache, and data filesystems
    static const QString DevSystem;
    static const QString DevCache;
    static const QString DevData;

    static const QString System;
    static const QString Cache;
    static const QString Data;

    PartitionConfig();
    ~PartitionConfig();

    QString name() const;
    void setName(const QString &name);

    QString description() const;
    void setDescription(const QString &description);

    QString kernel() const;
    void setKernel(const QString &kernel);

    QString id() const;
    void setId(const QString &id);

    QString targetSystem() const;
    void setTargetSystem(const QString &path);

    QString targetCache() const;
    void setTargetCache(const QString &path);

    QString targetData() const;
    void setTargetData(const QString &path);

    QString targetSystemPartition() const;
    void setTargetSystemPartition(const QString &partition);

    QString targetCachePartition() const;
    void setTargetCachePartition(const QString &partition);

    QString targetDataPartition() const;
    void setTargetDataPartition(const QString &partition);

    bool replaceShellLine(QByteArray *contents,
                          bool targetPathOnly = false) const;

private:
    const QScopedPointer<PartitionConfigPrivate> d_ptr;
    Q_DECLARE_PRIVATE(PartitionConfig)
};

#endif // PARTITIONCONFIG_H
