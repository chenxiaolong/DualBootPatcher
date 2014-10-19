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

#ifndef PATCHERPATHS_H
#define PATCHERPATHS_H

#include "libdbp_global.h"

#include "cpiofile.h"
#include "device.h"
#include "fileinfo.h"
#include "partitionconfig.h"
#include "patchererror.h"
#include "patchinfo.h"

#include <QtCore/QScopedPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QXmlStreamReader>

class Patcher;
class AutoPatcher;
class RamdiskPatcher;

class PatcherPathsPrivate;

class LIBDBPSHARED_EXPORT PatcherPaths
{
public:
    PatcherPaths();
    ~PatcherPaths();

    PatcherError::Error error() const;
    QString errorString() const;

    QString binariesDirectory() const;
    QString dataDirectory() const;
    QString initsDirectory() const;
    QString patchesDirectory() const;
    QString patchInfosDirectory() const;
    QString scriptsDirectory() const;

    void setBinariesDirectory(const QString &path);
    void setDataDirectory(const QString &path);
    void setInitsDirectory(const QString &path);
    void setPatchesDirectory(const QString &path);
    void setPatchInfosDirectory(const QString &path);
    void setScriptsDirectory(const QString &path);

    void reset();

    QString version() const;
    QList<Device *> devices() const;
    Device * deviceFromCodename(const QString &codename) const;
    QList<PatchInfo *> patchInfos(const Device * const device) const;

    void loadDefaultDevices();
    void loadDefaultPatchers();

    QStringList patchers() const;
    QStringList autoPatchers() const;
    QStringList ramdiskPatchers() const;

    QString patcherName(const QString &id) const;

    QSharedPointer<Patcher> createPatcher(const QString &id) const;
    QSharedPointer<AutoPatcher> createAutoPatcher(const QString &id,
                                                  const FileInfo * const info,
                                                  const PatchInfo::AutoPatcherArgs &args) const;
    QSharedPointer<RamdiskPatcher> createRamdiskPatcher(const QString &id,
                                                        const FileInfo * const info,
                                                        CpioFile * const cpio) const;

    QList<PartitionConfig *> partitionConfigs() const;
    PartitionConfig * partitionConfig(const QString &id) const;

    QStringList initBinaries() const;

    bool loadPatchInfos();

private:
    // Tags for patchinfo files
    static const QString PatchInfoTagPatchinfo;
    static const QString PatchInfoTagMatches;
    static const QString PatchInfoTagNotMatched;
    static const QString PatchInfoTagName;
    static const QString PatchInfoTagRegex;
    static const QString PatchInfoTagExcludeRegex;
    static const QString PatchInfoTagRegexes;
    static const QString PatchInfoTagHasBootImage;
    static const QString PatchInfoTagRamdisk;
    static const QString PatchInfoTagPatchedInit;
    static const QString PatchInfoTagAutopatchers;
    static const QString PatchInfoTagAutopatcher;
    static const QString PatchInfoTagDeviceCheck;
    static const QString PatchInfoTagPartconfigs;
    static const QString PatchInfoTagInclude;
    static const QString PatchInfoTagExclude;

    static const QString PatchInfoAttrRegex;

    static const QString XmlTextTrue;
    static const QString XmlTextFalse;

    // XML parsing functions for the patchinfo files
    bool loadPatchInfoXml(const QString &path, const QString &pathId);
    void parsePatchInfoTagPatchinfo(QXmlStreamReader &xml, PatchInfo * const info);
    void parsePatchInfoTagMatches(QXmlStreamReader &xml, PatchInfo * const info);
    void parsePatchInfoTagNotMatched(QXmlStreamReader &xml, PatchInfo * const info);
    void parsePatchInfoTagName(QXmlStreamReader &xml, PatchInfo * const info);
    void parsePatchInfoTagRegex(QXmlStreamReader &xml, PatchInfo * const info);
    void parsePatchInfoTagExcludeRegex(QXmlStreamReader &xml, PatchInfo * const info);
    void parsePatchInfoTagRegexes(QXmlStreamReader &xml, PatchInfo * const info);
    void parsePatchInfoTagHasBootImage(QXmlStreamReader &xml, PatchInfo * const info, const QString &type);
    void parsePatchInfoTagRamdisk(QXmlStreamReader &xml, PatchInfo * const info, const QString &type);
    void parsePatchInfoTagPatchedInit(QXmlStreamReader &xml, PatchInfo * const info, const QString &type);
    void parsePatchInfoTagAutopatchers(QXmlStreamReader &xml, PatchInfo * const info, const QString &type);
    void parsePatchInfoTagAutopatcher(QXmlStreamReader &xml, PatchInfo * const info, const QString &type);
    void parsePatchInfoTagDeviceCheck(QXmlStreamReader &xml, PatchInfo * const info, const QString &type);
    void parsePatchInfoTagPartconfigs(QXmlStreamReader &xml, PatchInfo * const info, const QString &type);
    void parsePatchInfoTagInclude(QXmlStreamReader &xml, PatchInfo * const info, const QString &type);
    void parsePatchInfoTagExclude(QXmlStreamReader &xml, PatchInfo * const info, const QString &type);

    const QScopedPointer<PatcherPathsPrivate> d_ptr;
    Q_DECLARE_PRIVATE(PatcherPaths)
};

#endif // PATCHERPATHS_H
