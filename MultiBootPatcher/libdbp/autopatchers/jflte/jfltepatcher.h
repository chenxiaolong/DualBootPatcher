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

#ifndef JFLTEPATCHER_H
#define JFLTEPATCHER_H

#include <libdbp/patcherinterface.h>


class JflteBasePatcherPrivate;

class JflteBasePatcher : public AutoPatcher
{
public:
    explicit JflteBasePatcher(const PatcherPaths * const pp,
                              const FileInfo * const info);
    virtual ~JflteBasePatcher();

    virtual PatcherError::Error error() const override;
    virtual QString errorString() const override;

    virtual QString id() const override = 0;

    virtual QStringList newFiles() const override = 0;
    virtual QStringList existingFiles() const override = 0;

    virtual bool patchFile(const QString &file,
                           QByteArray * const contents,
                           const QStringList &bootImages) override = 0;

protected:
    const QScopedPointer<JflteBasePatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(JflteBasePatcher)
};


class JflteDalvikCachePatcher : public JflteBasePatcher
{
public:
    explicit JflteDalvikCachePatcher(const PatcherPaths * const pp,
                                     const FileInfo * const info);

    static const QString Id;

    virtual QString id() const override;

    virtual QStringList newFiles() const override;
    virtual QStringList existingFiles() const override;

    virtual bool patchFile(const QString &file,
                           QByteArray * const contents,
                           const QStringList &bootImages) override;
};


class JflteGoogleEditionPatcher : public JflteBasePatcher
{
public:
    explicit JflteGoogleEditionPatcher(const PatcherPaths * const pp,
                                       const FileInfo * const info);

    static const QString Id;

    virtual QString id() const override;

    virtual QStringList newFiles() const override;
    virtual QStringList existingFiles() const override;

    virtual bool patchFile(const QString &file,
                           QByteArray * const contents,
                           const QStringList &bootImages) override;
};


class JflteSlimAromaBundledMount : public JflteBasePatcher
{
public:
    explicit JflteSlimAromaBundledMount(const PatcherPaths * const pp,
                                        const FileInfo * const info);

    static const QString Id;

    virtual QString id() const override;

    virtual QStringList newFiles() const override;
    virtual QStringList existingFiles() const override;

    virtual bool patchFile(const QString &file,
                           QByteArray * const contents,
                           const QStringList &bootImages) override;
};


class JflteImperiumPatcher : public JflteBasePatcher
{
public:
    explicit JflteImperiumPatcher(const PatcherPaths * const pp,
                                  const FileInfo * const info);

    static const QString Id;

    virtual QString id() const override;

    virtual QStringList newFiles() const override;
    virtual QStringList existingFiles() const override;

    virtual bool patchFile(const QString &file,
                           QByteArray * const contents,
                           const QStringList &bootImages) override;
};


class JflteNegaliteNoWipeData : public JflteBasePatcher
{
public:
    explicit JflteNegaliteNoWipeData(const PatcherPaths * const pp,
                                     const FileInfo * const info);

    static const QString Id;

    virtual QString id() const override;

    virtual QStringList newFiles() const override;
    virtual QStringList existingFiles() const override;

    virtual bool patchFile(const QString &file,
                           QByteArray * const contents,
                           const QStringList &bootImages) override;
};


class JflteTriForceFixAroma : public JflteBasePatcher
{
public:
    explicit JflteTriForceFixAroma(const PatcherPaths * const pp,
                                   const FileInfo * const info);

    static const QString Id;

    virtual QString id() const override;

    virtual QStringList newFiles() const override;
    virtual QStringList existingFiles() const override;

    virtual bool patchFile(const QString &file,
                           QByteArray * const contents,
                           const QStringList &bootImages) override;
};


class JflteTriForceFixUpdate : public JflteBasePatcher
{
public:
    explicit JflteTriForceFixUpdate(const PatcherPaths * const pp,
                                    const FileInfo * const info);

    static const QString Id;

    virtual QString id() const override;

    virtual QStringList newFiles() const override;
    virtual QStringList existingFiles() const override;

    virtual bool patchFile(const QString &file,
                           QByteArray * const contents,
                           const QStringList &bootImages) override;
};

#endif // JFLTEPATCHER_H
