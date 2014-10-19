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

#include "patcherpaths.h"
#include "patcherpaths_p.h"

#include "device.h"
#include "device_p.h"

#include "patchinfo.h"
#include "patchinfo_p.h"

#include "patcherinterface.h"

// Patchers
#include "patchers/multiboot/multibootpatcher.h"
#include "patchers/primaryupgrade/primaryupgradepatcher.h"
#include "patchers/syncdaemonupdate/syncdaemonupdatepatcher.h"
#include "autopatchers/jflte/jfltepatcher.h"
#include "autopatchers/noobdev/noobdevpatcher.h"
#include "autopatchers/patchfile/patchfilepatcher.h"
#include "autopatchers/standard/standardpatcher.h"
#include "ramdiskpatchers/bacon/baconramdiskpatcher.h"
#include "ramdiskpatchers/d800/d800ramdiskpatcher.h"
#include "ramdiskpatchers/falcon/falconramdiskpatcher.h"
#include "ramdiskpatchers/hammerhead/hammerheadramdiskpatcher.h"
#include "ramdiskpatchers/hlte/hlteramdiskpatcher.h"
#include "ramdiskpatchers/jflte/jflteramdiskpatcher.h"
#include "ramdiskpatchers/klte/klteramdiskpatcher.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QStringBuilder>
#include <QtCore/QXmlStreamReader>

static const QString BinariesDirName = QStringLiteral("binaries");
static const QString InitsDirName = QStringLiteral("inits");
static const QString PatchesDirName = QStringLiteral("patches");
static const QString PatchInfosDirName = QStringLiteral("patchinfos");
static const QString ScriptsDirName = QStringLiteral("scripts");
static const QChar Sep = QLatin1Char('/');

PatcherPathsPrivate::~PatcherPathsPrivate()
{
    for (Device *device : devices) {
        delete device;
    }
}

// --------------------------------

const QString PatcherPaths::PatchInfoTagPatchinfo = QLatin1String("patchinfo");
const QString PatcherPaths::PatchInfoTagMatches = QLatin1String("matches");
const QString PatcherPaths::PatchInfoTagNotMatched = QLatin1String("not-matched");
const QString PatcherPaths::PatchInfoTagName = QLatin1String("name");
const QString PatcherPaths::PatchInfoTagRegex = QLatin1String("regex");
const QString PatcherPaths::PatchInfoTagExcludeRegex = QLatin1String("exclude-regex");
const QString PatcherPaths::PatchInfoTagRegexes = QLatin1String("regexes");
const QString PatcherPaths::PatchInfoTagHasBootImage = QLatin1String("has-boot-image");
const QString PatcherPaths::PatchInfoTagRamdisk = QLatin1String("ramdisk");
const QString PatcherPaths::PatchInfoTagPatchedInit = QLatin1String("patched-init");
const QString PatcherPaths::PatchInfoTagAutopatchers = QLatin1String("autopatchers");
const QString PatcherPaths::PatchInfoTagAutopatcher = QLatin1String("autopatcher");
const QString PatcherPaths::PatchInfoTagDeviceCheck = QLatin1String("device-check");
const QString PatcherPaths::PatchInfoTagPartconfigs = QLatin1String("partconfigs");
const QString PatcherPaths::PatchInfoTagInclude = QLatin1String("include");
const QString PatcherPaths::PatchInfoTagExclude = QLatin1String("exclude");

const QString PatcherPaths::PatchInfoAttrRegex = QLatin1String("regex");

const QString PatcherPaths::XmlTextTrue = QLatin1String("true");
const QString PatcherPaths::XmlTextFalse = QLatin1String("false");

PatcherPaths::PatcherPaths() : d_ptr(new PatcherPathsPrivate())
{
    Q_D(PatcherPaths);

    loadDefaultDevices();
    loadDefaultPatchers();

    d->patchinfoIncludeDirs << QStringLiteral("Google_Apps");
    d->patchinfoIncludeDirs << QStringLiteral("Other");

    d->version = QStringLiteral(LIBDBP_VERSION);
}

PatcherPaths::~PatcherPaths()
{
    // Destructor so d_ptr is destroyed

    Q_D(PatcherPaths);

    // Clean up devices
    for (Device *device : d->devices) {
        delete device;
    }
    d->devices.clear();

    // Clean up patchinfos
    for (PatchInfo *info : d->patchInfos) {
        delete info;
    }
    d->patchInfos.clear();

    // Clean up partconfigs
    for (PartitionConfig *config : d->partConfigs) {
        delete config;
    }
    d->partConfigs.clear();
}

PatcherError::Error PatcherPaths::error() const
{
    Q_D(const PatcherPaths);

    return d->errorCode;
}

QString PatcherPaths::errorString() const
{
    Q_D(const PatcherPaths);

    return d->errorString;
}

QString PatcherPaths::binariesDirectory() const
{
    Q_D(const PatcherPaths);

    if (d->binariesDir.isNull()) {
        return dataDirectory() % Sep % BinariesDirName;
    } else {
        return d->binariesDir;
    }
}

QString PatcherPaths::dataDirectory() const
{
    Q_D(const PatcherPaths);

    return d->dataDir;
}

QString PatcherPaths::initsDirectory() const
{
    Q_D(const PatcherPaths);

    if (d->initsDir.isNull()) {
        return dataDirectory() % Sep % InitsDirName;
    } else {
        return d->initsDir;
    }
}

QString PatcherPaths::patchesDirectory() const
{
    Q_D(const PatcherPaths);

    if (d->patchesDir.isNull()) {
        return dataDirectory() % Sep % PatchesDirName;
    } else {
        return d->patchesDir;
    }
}

QString PatcherPaths::patchInfosDirectory() const
{
    Q_D(const PatcherPaths);

    if (d->patchInfosDir.isNull()) {
        return dataDirectory() % Sep % PatchInfosDirName;
    } else {
        return d->patchInfosDir;
    }
}

QString PatcherPaths::scriptsDirectory() const
{
    Q_D(const PatcherPaths);

    if (d->scriptsDir.isNull()) {
        return dataDirectory() % Sep % ScriptsDirName;
    } else {
        return d->scriptsDir;
    }
}

void PatcherPaths::setBinariesDirectory(const QString &path)
{
    Q_D(PatcherPaths);

    d->binariesDir = path;
}

void PatcherPaths::setDataDirectory(const QString &path)
{
    Q_D(PatcherPaths);

    d->dataDir = path;
}

void PatcherPaths::setInitsDirectory(const QString &path)
{
    Q_D(PatcherPaths);

    d->initsDir = path;
}

void PatcherPaths::setPatchesDirectory(const QString &path)
{
    Q_D(PatcherPaths);

    d->patchesDir = path;
}

void PatcherPaths::setPatchInfosDirectory(const QString &path)
{
    Q_D(PatcherPaths);

    d->patchInfosDir = path;
}

void PatcherPaths::setScriptsDirectory(const QString &path)
{
    Q_D(PatcherPaths);

    d->scriptsDir = path;
}

void PatcherPaths::reset()
{
    Q_D(PatcherPaths);

    // Paths
    d->dataDir.clear();
    d->initsDir.clear();
    d->patchesDir.clear();
    d->patchInfosDir.clear();

    for (Device *device : d->devices) {
        delete device;
    }

    d->devices.clear();
}

QString PatcherPaths::version() const
{
    Q_D(const PatcherPaths);

    return d->version;
}

QList<Device *> PatcherPaths::devices() const
{
    Q_D(const PatcherPaths);

    return d->devices;
}

Device * PatcherPaths::deviceFromCodename(const QString &codename) const
{
    Q_D(const PatcherPaths);

    for (Device * device : d->devices) {
        if (device->codename() == codename) {
            return device;
        }
    }

    return nullptr;
}

QList<PatchInfo *> PatcherPaths::patchInfos(const Device * const device) const
{
    Q_D(const PatcherPaths);

    QList<PatchInfo *> l;

    for (PatchInfo *info : d->patchInfos) {
        if (info->path().startsWith(device->codename())) {
            l << info;
            continue;
        }

        for (const QString &include : d->patchinfoIncludeDirs) {
            if (info->path().startsWith(include)) {
                l << info;
                break;
            }
        }
    }

    return l;
}

void PatcherPaths::loadDefaultDevices()
{
    Q_D(PatcherPaths);

    Device *device;

    // Samsung Galaxy S 4
    device = new Device();
    device->setCodename(QStringLiteral("jflte"));
    device->setName(QStringLiteral("Samsung Galaxy S 4"));
    device->setSelinux(Device::SelinuxPermissive);
    device->setPartition(Device::SystemPartition, QStringLiteral("mmcblk0p16"));
    device->setPartition(Device::CachePartition, QStringLiteral("mmcblk0p18"));
    device->setPartition(Device::DataPartition, QStringLiteral("mmcblk0p29"));
    d->devices << device;

    // Samsung Galaxy S 5
    device = new Device();
    device->setCodename(QStringLiteral("klte"));
    device->setName(QStringLiteral("Samsung Galaxy S 5"));
    device->setSelinux(Device::SelinuxPermissive);
    device->setPartition(Device::SystemPartition, QStringLiteral("mmcblk0p23"));
    device->setPartition(Device::CachePartition, QStringLiteral("mmcblk0p24"));
    device->setPartition(Device::DataPartition, QStringLiteral("mmcblk0p26"));
    d->devices << device;

    // Samsung Galaxy Note 3
    device = new Device();
    device->setCodename(QStringLiteral("hlte"));
    device->setName(QStringLiteral("Samsung Galaxy Note 3"));
    device->setSelinux(Device::SelinuxPermissive);
    device->setPartition(Device::SystemPartition, QStringLiteral("mmcblk0p23"));
    device->setPartition(Device::CachePartition, QStringLiteral("mmcblk0p24"));
    device->setPartition(Device::DataPartition, QStringLiteral("mmcblk0p26"));
    d->devices << device;

    // Google/LG Nexus 5
    device = new Device();
    device->setCodename(QStringLiteral("hammerhead"));
    device->setName(QStringLiteral("Google/LG Nexus 5"));
    device->setSelinux(Device::SelinuxUnchanged);
    d->devices << device;

    // OnePlus One
    device = new Device();
    device->setCodename(QStringLiteral("bacon"));
    device->setName(QStringLiteral("OnePlus One"));
    device->setSelinux(Device::SelinuxUnchanged);
    d->devices << device;

    // LG G2
    device = new Device();
    device->setCodename(QStringLiteral("d800"));
    device->setName(QStringLiteral("LG G2"));
    device->setSelinux(Device::SelinuxUnchanged);
    d->devices << device;

    // Falcon
    device = new Device();
    device->setCodename(QStringLiteral("falcon"));
    device->setName(QStringLiteral("Motorola Moto G"));
    device->setSelinux(Device::SelinuxUnchanged);
    d->devices << device;
}

void PatcherPaths::loadDefaultPatchers()
{
    Q_D(PatcherPaths);

    d->partConfigs << MultiBootPatcher::partConfigs();
    d->partConfigs << PrimaryUpgradePatcher::partConfigs();
}

QStringList PatcherPaths::patchers() const
{
    return QStringList()
            << MultiBootPatcher::Id
            << PrimaryUpgradePatcher::Id
            << SyncdaemonUpdatePatcher::Id;
}

QStringList PatcherPaths::autoPatchers() const
{
    return QStringList()
            << JflteDalvikCachePatcher::Id
            << JflteGoogleEditionPatcher::Id
            << JflteSlimAromaBundledMount::Id
            << JflteImperiumPatcher::Id
            << JflteNegaliteNoWipeData::Id
            << JflteTriForceFixAroma::Id
            << JflteTriForceFixUpdate::Id
            << NoobdevMultiBoot::Id
            << NoobdevSystemProp::Id
            << PatchFilePatcher::Id
            << StandardPatcher::Id;
}

QStringList PatcherPaths::ramdiskPatchers() const
{
    return QStringList()
            << BaconRamdiskPatcher::Id
            << D800RamdiskPatcher::Id
            << FalconRamdiskPatcher::Id
            << HammerheadAOSPRamdiskPatcher::Id
            << HammerheadNoobdevRamdiskPatcher::Id
            << HlteAOSPRamdiskPatcher::Id
            << JflteAOSPRamdiskPatcher::Id
            << JflteGoogleEditionRamdiskPatcher::Id
            << JflteNoobdevRamdiskPatcher::Id
            << JflteTouchWizRamdiskPatcher::Id
            << KlteAOSPRamdiskPatcher::Id
            << KlteTouchWizRamdiskPatcher::Id;
}

QSharedPointer<Patcher> PatcherPaths::createPatcher(const QString &id) const
{
    if (id == MultiBootPatcher::Id) {
        return QSharedPointer<Patcher>(new MultiBootPatcher(this));
    } else if (id == PrimaryUpgradePatcher::Id) {
        return QSharedPointer<Patcher>(new PrimaryUpgradePatcher(this));
    } else if (id == SyncdaemonUpdatePatcher::Id) {
        return QSharedPointer<Patcher>(new SyncdaemonUpdatePatcher(this));
    }

    return QSharedPointer<Patcher>();
}

QSharedPointer<AutoPatcher> PatcherPaths::createAutoPatcher(const QString &id,
                                                            const FileInfo * const info,
                                                            const PatchInfo::AutoPatcherArgs &args) const
{
    if (id == JflteDalvikCachePatcher::Id) {
        return QSharedPointer<AutoPatcher>(new JflteDalvikCachePatcher(this, info));
    } else if (id == JflteGoogleEditionPatcher::Id) {
        return QSharedPointer<AutoPatcher>(new JflteGoogleEditionPatcher(this, info));
    } else if (id == JflteSlimAromaBundledMount::Id) {
        return QSharedPointer<AutoPatcher>(new JflteSlimAromaBundledMount(this, info));
    } else if (id == JflteImperiumPatcher::Id) {
        return QSharedPointer<AutoPatcher>(new JflteImperiumPatcher(this, info));
    } else if (id == JflteNegaliteNoWipeData::Id) {
        return QSharedPointer<AutoPatcher>(new JflteNegaliteNoWipeData(this, info));
    } else if (id == JflteTriForceFixAroma::Id) {
        return QSharedPointer<AutoPatcher>(new JflteTriForceFixAroma(this, info));
    } else if (id == JflteTriForceFixUpdate::Id) {
        return QSharedPointer<AutoPatcher>(new JflteTriForceFixUpdate(this, info));
    } else if (id == NoobdevMultiBoot::Id) {
        return QSharedPointer<AutoPatcher>(new NoobdevMultiBoot(this, info));
    } else if (id == NoobdevSystemProp::Id) {
        return QSharedPointer<AutoPatcher>(new NoobdevSystemProp(this, info));
    } else if (id == PatchFilePatcher::Id) {
        return QSharedPointer<AutoPatcher>(new PatchFilePatcher(this, info, args));
    } else if (id == StandardPatcher::Id) {
        return QSharedPointer<AutoPatcher>(new StandardPatcher(this, info, args));
    }

    return QSharedPointer<AutoPatcher>();
}

QSharedPointer<RamdiskPatcher> PatcherPaths::createRamdiskPatcher(const QString &id,
                                                                  const FileInfo * const info,
                                                                  CpioFile * const cpio) const
{
    if (id == BaconRamdiskPatcher::Id) {
        return QSharedPointer<RamdiskPatcher>(new BaconRamdiskPatcher(this, info, cpio));
    } else if (id == D800RamdiskPatcher::Id) {
        return QSharedPointer<RamdiskPatcher>(new D800RamdiskPatcher(this, info, cpio));
    } else if (id == FalconRamdiskPatcher::Id) {
        return QSharedPointer<RamdiskPatcher>(new FalconRamdiskPatcher(this, info, cpio));
    } else if (id == HammerheadAOSPRamdiskPatcher::Id) {
        return QSharedPointer<RamdiskPatcher>(new HammerheadAOSPRamdiskPatcher(this, info, cpio));
    } else if (id == HammerheadNoobdevRamdiskPatcher::Id) {
        return QSharedPointer<RamdiskPatcher>(new HammerheadNoobdevRamdiskPatcher(this, info, cpio));
    } else if (id == HlteAOSPRamdiskPatcher::Id) {
        return QSharedPointer<RamdiskPatcher>(new HlteAOSPRamdiskPatcher(this, info, cpio));
    } else if (id == JflteAOSPRamdiskPatcher::Id) {
        return QSharedPointer<RamdiskPatcher>(new JflteAOSPRamdiskPatcher(this, info, cpio));
    } else if (id == JflteGoogleEditionRamdiskPatcher::Id) {
        return QSharedPointer<RamdiskPatcher>(new JflteGoogleEditionRamdiskPatcher(this, info, cpio));
    } else if (id == JflteNoobdevRamdiskPatcher::Id) {
        return QSharedPointer<RamdiskPatcher>(new JflteNoobdevRamdiskPatcher(this, info, cpio));
    } else if (id == JflteTouchWizRamdiskPatcher::Id) {
        return QSharedPointer<RamdiskPatcher>(new JflteTouchWizRamdiskPatcher(this, info, cpio));
    } else if (id == KlteAOSPRamdiskPatcher::Id) {
        return QSharedPointer<RamdiskPatcher>(new KlteAOSPRamdiskPatcher(this, info, cpio));
    } else if (id == KlteTouchWizRamdiskPatcher::Id) {
        return QSharedPointer<RamdiskPatcher>(new KlteTouchWizRamdiskPatcher(this, info, cpio));
    }

    return QSharedPointer<RamdiskPatcher>();
}

QString PatcherPaths::patcherName(const QString &id) const
{
    if (id == MultiBootPatcher::Id) {
        return MultiBootPatcher::Name;
    } else if (id == PrimaryUpgradePatcher::Id) {
        return PrimaryUpgradePatcher::Name;
    } else if (id == SyncdaemonUpdatePatcher::Id) {
        return SyncdaemonUpdatePatcher::Name;
    }

    return QString();
}

QList<PartitionConfig *> PatcherPaths::partitionConfigs() const
{
    Q_D(const PatcherPaths);

    return d->partConfigs;
}

QStringList PatcherPaths::initBinaries() const
{
    Q_D(const PatcherPaths);

    QStringList inits;

    QDir dir(initsDirectory());
    if (!dir.exists() || !dir.isReadable()) {
        return QStringList();
    }

    QDirIterator iter(dir.absolutePath(),
                      QStringList(QStringLiteral("*")),
                      QDir::Files, QDirIterator::Subdirectories);
    while (iter.hasNext()) {
        const QString &name = iter.next();
        inits << dir.relativeFilePath(name);
    }

    inits.sort();
    return inits;
}

PartitionConfig * PatcherPaths::partitionConfig(const QString &id) const
{
    Q_D(const PatcherPaths);

    for (PartitionConfig *config : d->partConfigs) {
        if (config->id() == id) {
            return config;
        }
    }

    return nullptr;
}

bool PatcherPaths::loadPatchInfos()
{
    Q_D(PatcherPaths);

    QDir dir(patchInfosDirectory());
    if (!dir.exists() || !dir.isReadable()) {
        d->errorCode = PatcherError::DirectoryNotExistError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(patchInfosDirectory());
        return false;
    }

    QDirIterator iter(dir.absolutePath(),
                      QStringList(QStringLiteral("*.xml")),
                      QDir::Files, QDirIterator::Subdirectories);
    while (iter.hasNext()) {
        QString name = iter.next();
        QString id = dir.relativeFilePath(name.left(name.size() - 4));
        if (!loadPatchInfoXml(name, id)) {
            d->errorCode = PatcherError::XmlParseFileError;
            d->errorString = PatcherError::errorString(d->errorCode).arg(name);
            return false;
        }
    }

    return true;
}

bool PatcherPaths::loadPatchInfoXml(const QString &path,
                                    const QString &pathId)
{
    Q_D(PatcherPaths);

    QFile file(path);
    if (!file.open(QFile::ReadOnly)) {
        d->errorCode = PatcherError::FileOpenError;
        d->errorString = PatcherError::errorString(d->errorCode).arg(path);
        return false;
    }

    QXmlStreamReader xml(&file);

    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartDocument) {
            continue;
        }

        if (token == QXmlStreamReader::StartElement) {
            if (xml.name() == PatchInfoTagPatchinfo) {
                PatchInfo *info = new PatchInfo();
                parsePatchInfoTagPatchinfo(xml, info);

                if (xml.hasError()) {
                    delete info;
                } else {
                    info->d_func()->path = pathId;
                    d->patchInfos << info;
                }
            } else {
                qWarning() << "Unknown tag:" << xml.name();
            }
        }
    }

    if (xml.hasError()) {
        qWarning() << "Failed to parse XML:" << xml.errorString();
        xml.clear();
        file.close();
        return false;
    }

    xml.clear();

    file.close();

    return true;
}

void PatcherPaths::parsePatchInfoTagPatchinfo(QXmlStreamReader &xml,
                                              PatchInfo * const info)
{
    if (xml.tokenType() != QXmlStreamReader::StartElement
            || xml.name() != PatchInfoTagPatchinfo) {
        return;
    }

    xml.readNext();

    while (!xml.hasError()
            && !(xml.tokenType() == QXmlStreamReader::EndElement
            && xml.name() == PatchInfoTagPatchinfo)) {
        if (xml.tokenType() == QXmlStreamReader::StartElement) {
            if (xml.name() == PatchInfoTagPatchinfo) {
                qWarning() << "Nested <patchinfo> is not allowed";
                xml.skipCurrentElement();
            } else if (xml.name() == PatchInfoTagMatches) {
                parsePatchInfoTagMatches(xml, info);
            } else if (xml.name() == PatchInfoTagNotMatched) {
                parsePatchInfoTagNotMatched(xml, info);
            } else if (xml.name() == PatchInfoTagName) {
                parsePatchInfoTagName(xml, info);
            } else if (xml.name() == PatchInfoTagRegex) {
                parsePatchInfoTagRegex(xml, info);
            } else if (xml.name() == PatchInfoTagRegexes) {
                parsePatchInfoTagRegexes(xml, info);
            } else if (xml.name() == PatchInfoTagHasBootImage) {
                parsePatchInfoTagHasBootImage(xml, info, PatchInfo::Default);
            } else if (xml.name() == PatchInfoTagRamdisk) {
                parsePatchInfoTagRamdisk(xml, info, PatchInfo::Default);
            } else if (xml.name() == PatchInfoTagPatchedInit) {
                parsePatchInfoTagPatchedInit(xml, info, PatchInfo::Default);
            } else if (xml.name() == PatchInfoTagAutopatchers) {
                parsePatchInfoTagAutopatchers(xml, info, PatchInfo::Default);
            } else if (xml.name() == PatchInfoTagDeviceCheck) {
                parsePatchInfoTagDeviceCheck(xml, info, PatchInfo::Default);
            } else if (xml.name() == PatchInfoTagPartconfigs) {
                parsePatchInfoTagPartconfigs(xml, info, PatchInfo::Default);
            } else {
                qWarning() << "Unrecognized tag within <patchinfo>:" << xml.name();
                xml.skipCurrentElement();
            }
        }

        xml.readNext();
    }
}

void PatcherPaths::parsePatchInfoTagMatches(QXmlStreamReader &xml,
                                            PatchInfo * const info)
{
    if (xml.tokenType() != QXmlStreamReader::StartElement
            || xml.name() != PatchInfoTagMatches) {
        return;
    }

    QXmlStreamAttributes attrs = xml.attributes();
    if (!attrs.hasAttribute(PatchInfoAttrRegex)) {
        qWarning() << "<matches> element has no 'regex' attribute";
        xml.skipCurrentElement();
        return;
    }

    const QString regex = attrs.value(PatchInfoAttrRegex).toString();

    info->d_func()->condRegexes.append(regex);

    xml.readNext();

    while (!xml.hasError()
            && !(xml.tokenType() == QXmlStreamReader::EndElement
            && xml.name() == PatchInfoTagMatches)) {
        if (xml.tokenType() == QXmlStreamReader::StartElement) {
            if (xml.name() == PatchInfoTagMatches) {
                qWarning() << "Nested <matches> is not allowed";
                xml.skipCurrentElement();
            } else if (xml.name() == PatchInfoTagHasBootImage) {
                parsePatchInfoTagHasBootImage(xml, info, regex);
            } else if (xml.name() == PatchInfoTagRamdisk) {
                parsePatchInfoTagRamdisk(xml, info, regex);
            } else if (xml.name() == PatchInfoTagPatchedInit) {
                parsePatchInfoTagPatchedInit(xml, info, regex);
            } else if (xml.name() == PatchInfoTagAutopatchers) {
                parsePatchInfoTagAutopatchers(xml, info, regex);
            } else if (xml.name() == PatchInfoTagDeviceCheck) {
                parsePatchInfoTagDeviceCheck(xml, info, regex);
            } else if (xml.name() == PatchInfoTagPartconfigs) {
                parsePatchInfoTagPartconfigs(xml, info, regex);
            } else {
                qWarning() << "Unrecognized tag within <matches>:" << xml.name();
                xml.skipCurrentElement();
            }
        }

        xml.readNext();
    }
}

void PatcherPaths::parsePatchInfoTagNotMatched(QXmlStreamReader &xml,
                                               PatchInfo * const info)
{
    if (xml.tokenType() != QXmlStreamReader::StartElement
            || xml.name() != PatchInfoTagNotMatched) {
        return;
    }

    info->d_func()->hasNotMatchedElement = true;

    xml.readNext();

    while (!xml.hasError()
            && !(xml.tokenType() == QXmlStreamReader::EndElement
            && xml.name() == PatchInfoTagNotMatched)) {
        if (xml.tokenType() == QXmlStreamReader::StartElement) {
            if (xml.name() == PatchInfoTagNotMatched) {
                qWarning() << "Nested <not-matched> is not allowed";
                xml.skipCurrentElement();
            } else if (xml.name() == PatchInfoTagHasBootImage) {
                parsePatchInfoTagHasBootImage(xml, info, PatchInfo::NotMatched);
            } else if (xml.name() == PatchInfoTagRamdisk) {
                parsePatchInfoTagRamdisk(xml, info, PatchInfo::NotMatched);
            } else if (xml.name() == PatchInfoTagPatchedInit) {
                parsePatchInfoTagPatchedInit(xml, info, PatchInfo::NotMatched);
            } else if (xml.name() == PatchInfoTagAutopatchers) {
                parsePatchInfoTagAutopatchers(xml, info, PatchInfo::NotMatched);
            } else if (xml.name() == PatchInfoTagDeviceCheck) {
                parsePatchInfoTagDeviceCheck(xml, info, PatchInfo::NotMatched);
            } else if (xml.name() == PatchInfoTagPartconfigs) {
                parsePatchInfoTagPartconfigs(xml, info, PatchInfo::NotMatched);
            } else {
                qWarning() << "Unrecognized tag within <not-matched>:" << xml.name();
                xml.skipCurrentElement();
            }
        }

        xml.readNext();
    }
}

void PatcherPaths::parsePatchInfoTagName(QXmlStreamReader &xml,
                                         PatchInfo * const info)
{
    if (xml.tokenType() != QXmlStreamReader::StartElement
            || xml.name() != PatchInfoTagName) {
        return;
    }

    xml.readNext();

    if (xml.tokenType() == QXmlStreamReader::Characters) {
        if (info->d_func()->name.isNull()) {
            info->d_func()->name = xml.text().toString();
        } else {
            qWarning() << "Ignoring additional <name> elements";
        }
    } else {
        qWarning() << "<name> tag has no text";
    }

    xml.skipCurrentElement();
}

void PatcherPaths::parsePatchInfoTagRegex(QXmlStreamReader &xml,
                                          PatchInfo * const info)
{
    if (xml.tokenType() != QXmlStreamReader::StartElement
            || xml.name() != PatchInfoTagRegex) {
        return;
    }

    xml.readNext();

    if (xml.tokenType() == QXmlStreamReader::Characters) {
        info->d_func()->regexes.append(xml.text().toString());
    } else {
        qWarning() << "<regex> tag has no text";
    }

    xml.skipCurrentElement();
}

void PatcherPaths::parsePatchInfoTagExcludeRegex(QXmlStreamReader &xml,
                                                 PatchInfo * const info)
{
    if (xml.tokenType() != QXmlStreamReader::StartElement
            || xml.name() != PatchInfoTagExcludeRegex) {
        return;
    }

    xml.readNext();

    if (xml.tokenType() == QXmlStreamReader::Characters) {
        info->d_func()->excludeRegexes.append(xml.text().toString());
    } else {
        qWarning() << "<exclude-regex> tag has no text";
    }

    xml.skipCurrentElement();
}

void PatcherPaths::parsePatchInfoTagRegexes(QXmlStreamReader &xml,
                                            PatchInfo * const info)
{
    if (xml.tokenType() != QXmlStreamReader::StartElement
            || xml.name() != PatchInfoTagRegexes) {
        return;
    }

    xml.readNext();

    while (!xml.hasError()
            && !(xml.tokenType() == QXmlStreamReader::EndElement
            && xml.name() == PatchInfoTagRegexes)) {
        if (xml.tokenType() == QXmlStreamReader::StartElement) {
            if (xml.name() == PatchInfoTagRegexes) {
                qWarning() << "Nested <regexes> is not allowed";
                xml.skipCurrentElement();
            } else if (xml.name() == PatchInfoTagRegex) {
                parsePatchInfoTagRegex(xml, info);
            } else if (xml.name() == PatchInfoTagExcludeRegex) {
                parsePatchInfoTagExcludeRegex(xml, info);
            } else {
                qWarning() << "Unrecognized tag within <regexes>:" << xml.name();
                xml.skipCurrentElement();
            }
        }

        xml.readNext();
    }
}

void PatcherPaths::parsePatchInfoTagHasBootImage(QXmlStreamReader &xml,
                                                 PatchInfo * const info,
                                                 const QString &type)
{
    if (xml.tokenType() != QXmlStreamReader::StartElement
            || xml.name() != PatchInfoTagHasBootImage) {
        return;
    }

    xml.readNext();

    if (xml.tokenType() == QXmlStreamReader::Characters) {
        if (xml.text() == XmlTextTrue) {
            info->d_func()->hasBootImage[type] = true;
        } else if (xml.text() == XmlTextFalse) {
            info->d_func()->hasBootImage[type] = false;
        } else {
            qWarning() << "Unknown value for <has-boot-image>:" << xml.text();
        }
    } else {
        qWarning() << "<has-boot-image> tag has no text";
    }

    xml.skipCurrentElement();
}

void PatcherPaths::parsePatchInfoTagRamdisk(QXmlStreamReader &xml,
                                            PatchInfo * const info,
                                            const QString &type)
{
    if (xml.tokenType() != QXmlStreamReader::StartElement
            || xml.name() != PatchInfoTagRamdisk) {
        return;
    }

    xml.readNext();

    if (xml.tokenType() == QXmlStreamReader::Characters) {
        info->d_func()->ramdisk[type] = xml.text().toString();
    } else {
        qWarning() << "<ramdisk> tag has no text";
    }

    xml.skipCurrentElement();
}

void PatcherPaths::parsePatchInfoTagPatchedInit(QXmlStreamReader &xml,
                                                PatchInfo * const info,
                                                const QString &type)
{
    if (xml.tokenType() != QXmlStreamReader::StartElement
            || xml.name() != PatchInfoTagPatchedInit) {
        return;
    }

    xml.readNext();

    if (xml.tokenType() == QXmlStreamReader::Characters) {
        info->d_func()->patchedInit[type] = xml.text().toString();
    } else {
        qWarning() << "<patched-init> tag has no text";
    }

    xml.skipCurrentElement();
}

void PatcherPaths::parsePatchInfoTagAutopatchers(QXmlStreamReader &xml,
                                                 PatchInfo * const info,
                                                 const QString &type)
{
    if (xml.tokenType() != QXmlStreamReader::StartElement
            || xml.name() != PatchInfoTagAutopatchers) {
        return;
    }

    xml.readNext();

    while (!xml.hasError()
            && !(xml.tokenType() == QXmlStreamReader::EndElement
            && xml.name() == PatchInfoTagAutopatchers)) {
        if (xml.tokenType() == QXmlStreamReader::StartElement) {
            if (xml.name() == PatchInfoTagAutopatchers) {
                qWarning() << "Nested <autopatchers> is not allowed";
                xml.skipCurrentElement();
            } else if (xml.name() == PatchInfoTagAutopatcher) {
                parsePatchInfoTagAutopatcher(xml, info, type);
            } else {
                qWarning() << "Unrecognized tag within <autopatchers>:" << xml.name();
                xml.skipCurrentElement();
            }
        }

        xml.readNext();
    }
}

void PatcherPaths::parsePatchInfoTagAutopatcher(QXmlStreamReader &xml,
                                                PatchInfo * const info,
                                                const QString &type)
{
    if (xml.tokenType() != QXmlStreamReader::StartElement
            || xml.name() != PatchInfoTagAutopatcher) {
        return;
    }

    QXmlStreamAttributes attrs = xml.attributes();
    PatchInfo::AutoPatcherArgs args;
    for (const QXmlStreamAttribute &attr : attrs) {
        args[attr.name().toString()] = attr.value().toString();
    }

    xml.readNext();

    if (xml.tokenType() == QXmlStreamReader::Characters) {
        PatchInfo::AutoPatcherItem item(xml.text().toString(), args);
        info->d_func()->autoPatchers[type].append(item);
    } else {
        qWarning() << "<autopatcher> tag has no text";
    }

    xml.skipCurrentElement();
}

void PatcherPaths::parsePatchInfoTagDeviceCheck(QXmlStreamReader &xml,
                                                PatchInfo * const info,
                                                const QString &type)
{
    if (xml.tokenType() != QXmlStreamReader::StartElement
            || xml.name() != PatchInfoTagDeviceCheck) {
        return;
    }

    xml.readNext();

    if (xml.tokenType() == QXmlStreamReader::Characters) {
        if (xml.text() == XmlTextTrue) {
            info->d_func()->deviceCheck[type] = true;
        } else if (xml.text() == XmlTextFalse) {
            info->d_func()->deviceCheck[type] = false;
        } else {
            qWarning() << "Unknown value for <device-check>:" << xml.text();
        }
    } else {
        qWarning() << "<device-check> tag has no text";
    }

    xml.skipCurrentElement();
}

void PatcherPaths::parsePatchInfoTagPartconfigs(QXmlStreamReader &xml,
                                                PatchInfo * const info,
                                                const QString &type)
{
    if (xml.tokenType() != QXmlStreamReader::StartElement
            || xml.name() != PatchInfoTagPartconfigs) {
        return;
    }

    if (!info->d_func()->supportedConfigs.contains(type)) {
        info->d_func()->supportedConfigs[type].append(QStringLiteral("all"));
    }

    xml.readNext();

    while (!xml.hasError()
            && !(xml.tokenType() == QXmlStreamReader::EndElement
            && xml.name() == PatchInfoTagPartconfigs)) {
        if (xml.tokenType() == QXmlStreamReader::StartElement) {
            if (xml.name() == PatchInfoTagPartconfigs) {
                qWarning() << "Nested <partconfigs> is not allowed";
                xml.skipCurrentElement();
            } else if (xml.name() == PatchInfoTagExclude) {
                parsePatchInfoTagExclude(xml, info, type);
            } else if (xml.name() == PatchInfoTagInclude) {
                parsePatchInfoTagInclude(xml, info, type);
            } else {
                qWarning() << "Unrecognized tag within <partconfigs>:" << xml.name();
                xml.skipCurrentElement();
            }
        }

        xml.readNext();
    }
}

void PatcherPaths::parsePatchInfoTagExclude(QXmlStreamReader &xml,
                                            PatchInfo * const info,
                                            const QString &type)
{
    if (xml.tokenType() != QXmlStreamReader::StartElement
            || xml.name() != PatchInfoTagExclude) {
        return;
    }

    xml.readNext();

    if (xml.tokenType() == QXmlStreamReader::Characters) {
        int index = -1;
        bool exists = false;

        const QString negated = QLatin1Char('!') % xml.text().toString();

        for (int i = 0; i < info->d_func()->supportedConfigs[type].size(); i++) {
            if (info->d_func()->supportedConfigs[type][i] == xml.text()) {
                index = i;
            }
            if (info->d_func()->supportedConfigs[type][i] == negated) {
                exists = true;
            }
        }

        if (!exists) {
            if (index != -1) {
                info->d_func()->supportedConfigs[type].removeAt(index);
            } else {
                info->d_func()->supportedConfigs[type].append(
                        QLatin1Char('!') % xml.text().toString());
            }
        }
    } else {
        qWarning() << "<exclude> tag has no text";
    }

    xml.skipCurrentElement();
}

void PatcherPaths::parsePatchInfoTagInclude(QXmlStreamReader &xml,
                                            PatchInfo * const info,
                                            const QString &type)
{
    if (xml.tokenType() != QXmlStreamReader::StartElement
            || xml.name() != PatchInfoTagInclude) {
        return;
    }

    xml.readNext();

    if (xml.tokenType() == QXmlStreamReader::Characters) {
        int index = -1;
        bool exists = false;

        const QString negated = QLatin1Char('!') % xml.text().toString();

        for (int i = 0; i < info->d_func()->supportedConfigs[type].size(); i++) {
            if (info->d_func()->supportedConfigs[type][i] == negated) {
                index = i;
            }
            if (info->d_func()->supportedConfigs[type][i] == xml.text()) {
                exists = true;
            }
        }

        if (!exists) {
            if (index != -1) {
                info->d_func()->supportedConfigs[type].removeAt(index);
            } else {
                info->d_func()->supportedConfigs[type].append(xml.text().toString());
            }
        }
    } else {
        qWarning() << "<include> tag has no text";
    }

    xml.skipCurrentElement();
}
