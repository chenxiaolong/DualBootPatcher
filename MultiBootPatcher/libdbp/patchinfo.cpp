#include "patchinfo.h"
#include "patchinfo_p.h"

#include "patcherpaths.h"

#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>


const QString PatchInfo::Default = QStringLiteral("default");
const QString PatchInfo::NotMatched = QStringLiteral("not-matched");

PatchInfoPrivate::PatchInfoPrivate()
{
    // No <not-matched> element
    hasNotMatchedElement = false;

    // Default to having a boot image
    hasBootImage[PatchInfo::Default] = true;

    // Default to autodetecting boot images in the zip file
    autodetectBootImages[PatchInfo::Default] = true;

    // Don't remove device checks
    deviceCheck[PatchInfo::Default] = true;

    // Allow all configs
    supportedConfigs[PatchInfo::Default].append(QStringLiteral("all"));
}

// --------------------------------

PatchInfo::PatchInfo(QObject *parent) :
    QObject(parent), d_ptr(new PatchInfoPrivate())
{
}

PatchInfo::~PatchInfo()
{
    // Destructor so d_ptr is destroyed
}

QString PatchInfo::path() const
{
    Q_D(const PatchInfo);

    return d->path;
}

void PatchInfo::setPath(const QString &path)
{
    Q_D(PatchInfo);

    d->path = path;
}

QString PatchInfo::name() const
{
    Q_D(const PatchInfo);

    return d->name;
}

void PatchInfo::setName(const QString &name)
{
    Q_D(PatchInfo);

    d->name = name;
}

QString PatchInfo::keyFromFilename(const QString &filename) const
{
    Q_D(const PatchInfo);

    QString basename = QFileInfo(filename).fileName();

    // The conditional regex is the key if <matches> elements are used
    // in the patchinfo xml files
    for (const QString &regex : d->condRegexes) {
        if (basename.contains(QRegularExpression(regex))) {
            return regex;
        }
    }

    // If none of those are matched, try <not-matched>
    if (d->hasNotMatchedElement) {
        return NotMatched;
    }

    // Otherwise, use the defaults
    return Default;
}

QStringList PatchInfo::regexes() const
{
    Q_D(const PatchInfo);

    return d->regexes;
}

void PatchInfo::setRegexes(const QStringList &regexes)
{
    Q_D(PatchInfo);

    d->regexes = regexes;
}

QStringList PatchInfo::excludeRegexes() const
{
    Q_D(const PatchInfo);

    return d->excludeRegexes;
}

void PatchInfo::setExcludeRegexes(const QStringList &regexes)
{
    Q_D(PatchInfo);

    d->excludeRegexes = regexes;
}

PatchInfo::AutoPatcherItems PatchInfo::autoPatchers(const QString &key) const
{
    Q_D(const PatchInfo);

    const AutoPatcherItems &def = d->autoPatchers.value(Default);

    if (key != Default && d->autoPatchers.keys().contains(key)) {
        return AutoPatcherItems() << def << d->autoPatchers[key];
    }

    return def;
}

void PatchInfo::setAutoPatchers(const QString &key, AutoPatcherItems autoPatchers)
{
    Q_D(PatchInfo);

    d->autoPatchers[key] = autoPatchers;
}

bool PatchInfo::hasBootImage(const QString &key) const
{
    Q_D(const PatchInfo);

    if (d->hasBootImage.keys().contains(key)) {
        return d->hasBootImage[key];
    }

    return d->hasBootImage.value(Default);
}

void PatchInfo::setHasBootImage(const QString &key, bool hasBootImage)
{
    Q_D(PatchInfo);

    d->hasBootImage[key] = hasBootImage;
}

bool PatchInfo::autodetectBootImages(const QString &key) const
{
    Q_D(const PatchInfo);

    if (d->autodetectBootImages.keys().contains(key)) {
        return d->autodetectBootImages[key];
    }

    return d->autodetectBootImages.value(Default);
}

void PatchInfo::setAutoDetectBootImages(const QString &key, bool autoDetect)
{
    Q_D(PatchInfo);

    d->autodetectBootImages[key] = autoDetect;
}

QStringList PatchInfo::bootImages(const QString &key) const
{
    Q_D(const PatchInfo);

    const QStringList &def = d->bootImages.value(Default);

    if (key != Default && d->bootImages.keys().contains(key)) {
        return QStringList() << def << d->bootImages[key];
    }

    return def;
}

void PatchInfo::setBootImages(const QString &key, const QStringList &bootImages)
{
    Q_D(PatchInfo);

    d->bootImages[key] = bootImages;
}

QString PatchInfo::ramdisk(const QString &key) const
{
    Q_D(const PatchInfo);

    if (d->ramdisk.keys().contains(key)) {
        return d->ramdisk[key];
    }

    return d->ramdisk.value(Default);
}

void PatchInfo::setRamdisk(const QString &key, const QString &ramdisk)
{
    Q_D(PatchInfo);

    d->ramdisk[key] = ramdisk;
}

QString PatchInfo::patchedInit(const QString &key) const
{
    Q_D(const PatchInfo);

    if (d->patchedInit.keys().contains(key)) {
        return d->patchedInit[key];
    }

    return d->patchedInit.value(Default);
}

void PatchInfo::setPatchedInit(const QString &key, const QString &init)
{
    Q_D(PatchInfo);

    d->patchedInit[key] = init;
}

bool PatchInfo::deviceCheck(const QString &key) const
{
    Q_D(const PatchInfo);

    if (d->deviceCheck.keys().contains(key)) {
        return d->deviceCheck[key];
    }

    return d->deviceCheck.value(Default);
}

void PatchInfo::setDeviceCheck(const QString &key, bool deviceCheck)
{
    Q_D(PatchInfo);

    d->deviceCheck[key] = deviceCheck;
}

QStringList PatchInfo::supportedConfigs(const QString &key) const
{
    Q_D(const PatchInfo);

    const QStringList &def = d->supportedConfigs.value(Default);

    if (key != Default && d->supportedConfigs.keys().contains(key)) {
        return QStringList() << def << d->supportedConfigs[key];
    }

    return def;
}

void PatchInfo::setSupportedConfigs(const QString &key, const QStringList &configs)
{
    Q_D(PatchInfo);

    d->supportedConfigs[key] = configs;
}

PatchInfo * PatchInfo::findMatchingPatchInfo(PatcherPaths *pp,
                                             Device *device,
                                             const QString &filename)
{
    if (device == nullptr) {
        return nullptr;
    }

    if (filename.isNull()) {
        return nullptr;
    }

    QString basename = QFileInfo(filename).fileName();

    for (PatchInfo *info : pp->patchInfos(device)) {
        for (const QString &regex : info->regexes()) {
            if (basename.contains(QRegularExpression(regex))) {
                bool skipCurInfo = false;

                // If the regex matches, make sure the filename isn't matched
                // by one of the exclusion regexes
                for (const QString &excludeRegex : info->excludeRegexes()) {
                    if (basename.contains(QRegularExpression(excludeRegex))) {
                        skipCurInfo = true;
                        break;
                    }
                }

                if (skipCurInfo) {
                    break;
                }

                return info;
            }
        }
    }

    return nullptr;
}
