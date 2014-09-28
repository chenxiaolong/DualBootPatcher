#ifndef QCOMRAMDISKPATCHER_H
#define QCOMRAMDISKPATCHER_H

#include "libdbp-ramdisk-qcom_global.h"

#include <libdbp/cpiofile.h>
#include <libdbp/plugininterface.h>

#include <QtCore/QObject>
#include <QtCore/QVariant>

class QcomRamdiskPatcherPrivate;

class LIBDBPRAMDISKQCOMSHARED_EXPORT QcomRamdiskPatcher : public RamdiskPatcher
{
public:
    static const QString SystemPartition;
    static const QString CachePartition;
    static const QString DataPartition;
    static const QString ApnhlosPartition;
    static const QString MdmPartition;

    static const QString ArgAdditionalFstabs;
    static const QString ArgForceSystemRw;
    static const QString ArgForceCacheRw;
    static const QString ArgForceDataRw;
    static const QString ArgKeepMountPoints;
    static const QString ArgSystemMountPoint;
    static const QString ArgCacheMountPoint;
    static const QString ArgDataMountPoint;
    static const QString ArgDefaultSystemMountArgs;
    static const QString ArgDefaultSystemVoldArgs;
    static const QString ArgDefaultCacheMountArgs;
    static const QString ArgDefaultCacheVoldArgs;

    explicit QcomRamdiskPatcher(const PatcherPaths * const pp,
                                const FileInfo * const info,
                                CpioFile * const cpio);
    ~QcomRamdiskPatcher();

    virtual PatcherError::Error error() const;
    virtual QString errorString() const;

    virtual QString name() const;

    virtual bool patchRamdisk();

    bool modifyInitRc();
    bool modifyInitQcomRc(QStringList additionalFiles = QStringList());

    bool modifyFstab(// Optional
                     bool *moveApnhlosMount = nullptr,
                     bool *moveMdmMount = nullptr);
    bool modifyFstab(QVariantMap args,
                     // Optional
                     bool *moveApnhlosMount = nullptr,
                     bool *moveMdmMount = nullptr);
    bool modifyFstab(bool *moveApnhlosMount,
                     bool *moveMdmMount,
                     const QStringList &additionalFstabs,
                     bool forceSystemRw,
                     bool forceCacheRw,
                     bool forceDataRw,
                     bool keepMountPoints,
                     const QString &systemMountPoint,
                     const QString &cacheMountPoint,
                     const QString &dataMountPoint,
                     const QString &defaultSystemMountArgs,
                     const QString &defaultSystemVoldArgs,
                     const QString &defaultCacheMountArgs,
                     const QString &defaultCacheVoldArgs);

    bool modifyInitTargetRc(bool insertApnhlos = false,
                            bool insertMdm = false);
    bool modifyInitTargetRc(QString filename,
                            bool insertApnhlos = false,
                            bool insertMdm = false);

private:
    static QString makeWritable(const QString &mountArgs);

    const QScopedPointer<QcomRamdiskPatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QcomRamdiskPatcher)
};

#endif // QCOMRAMDISKPATCHER_H
