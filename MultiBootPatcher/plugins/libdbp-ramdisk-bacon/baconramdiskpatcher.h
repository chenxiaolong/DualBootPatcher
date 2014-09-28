#ifndef BACONRAMDISKPATCHER_H
#define BACONRAMDISKPATCHER_H

#include <libdbp/cpiofile.h>
#include <libdbp/plugininterface.h>

#include <QtCore/QObject>


class BaconRamdiskPatcherPrivate;

class BaconRamdiskPatcher : public RamdiskPatcher
{
public:
    explicit BaconRamdiskPatcher(const PatcherPaths * const pp,
                                 const QString &id,
                                 const FileInfo * const info,
                                 CpioFile * const cpio);
    ~BaconRamdiskPatcher();

    static const QString AOSP;

    virtual PatcherError::Error error() const;
    virtual QString errorString() const;

    virtual QString name() const;

    virtual bool patchRamdisk();

private:
    const QScopedPointer<BaconRamdiskPatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(BaconRamdiskPatcher)
};

#endif // BACONRAMDISKPATCHER_H
