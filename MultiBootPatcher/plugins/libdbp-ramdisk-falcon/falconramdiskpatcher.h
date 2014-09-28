#ifndef FALCONRAMDISKPATCHER_H
#define FALCONRAMDISKPATCHER_H

#include <libdbp/cpiofile.h>
#include <libdbp/plugininterface.h>

#include <QtCore/QObject>


class FalconRamdiskPatcherPrivate;

class FalconRamdiskPatcher : public RamdiskPatcher
{
public:
    explicit FalconRamdiskPatcher(const PatcherPaths * const pp,
                                  const QString &id,
                                  const FileInfo * const info,
                                  CpioFile * const cpio);
    ~FalconRamdiskPatcher();

    static const QString AOSP;

    virtual PatcherError::Error error() const;
    virtual QString errorString() const;

    virtual QString name() const;

    virtual bool patchRamdisk();

private:
    const QScopedPointer<FalconRamdiskPatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(FalconRamdiskPatcher)
};

#endif // FALCONRAMDISKPATCHER_H
