#ifndef PATCHINFO_H
#define PATCHINFO_H

#include "libdbp_global.h"

#include "device.h"

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

class PatchInfoPrivate;
class PatcherPaths;

class LIBDBPSHARED_EXPORT PatchInfo : public QObject
{
    Q_OBJECT

    friend class PatcherPaths;

public:
    static const QString Default;
    static const QString NotMatched;

    explicit PatchInfo(QObject *parent = 0);
    ~PatchInfo();

    typedef QHash<QString, QString> AutoPatcherArgs;
    typedef QPair<QString, AutoPatcherArgs> AutoPatcherItem;
    typedef QList<AutoPatcherItem> AutoPatcherItems;

    QString path() const;
    void setPath(const QString &path);

    QString name() const;
    void setName(const QString &name);

    QString keyFromFilename(const QString &filename) const;

    QStringList regexes() const;
    void setRegexes(const QStringList &regexes);

    QStringList excludeRegexes() const;
    void setExcludeRegexes(const QStringList &regexes);

    // ** For the variables below, use hashmap[Default] to get the default
    //    values and hashmap[regex] with the index in condRegexes to get
    //    the overridden values.
    //
    // NOTE: If the variable is a list, the "overridden" values are used
    //       in addition to the default values

    AutoPatcherItems autoPatchers(const QString &key) const;
    void setAutoPatchers(const QString &key, AutoPatcherItems autoPatchers);

    bool hasBootImage(const QString &key) const;
    void setHasBootImage(const QString &key, bool hasBootImage);

    bool autodetectBootImages(const QString &key) const;
    void setAutoDetectBootImages(const QString &key, bool autoDetect);

    QStringList bootImages(const QString &key) const;
    void setBootImages(const QString &key, const QStringList &bootImages);

    QString ramdisk(const QString &key) const;
    void setRamdisk(const QString &key, const QString &ramdisk);

    QString patchedInit(const QString &key) const;
    void setPatchedInit(const QString &key, const QString &init);

    bool deviceCheck(const QString &key) const;
    void setDeviceCheck(const QString &key, bool deviceCheck);

    QStringList supportedConfigs(const QString &key) const;
    void setSupportedConfigs(const QString &key, const QStringList &configs);

    static PatchInfo * findMatchingPatchInfo(PatcherPaths *pp,
                                             Device *device,
                                             const QString &filename);

private:
    const QScopedPointer<PatchInfoPrivate> d_ptr;
    Q_DECLARE_PRIVATE(PatchInfo)
};

#endif // PATCHINFO_H
