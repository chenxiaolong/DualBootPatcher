#ifndef HAMMERHEADRAMDISKPATCHER_H
#define HAMMERHEADRAMDISKPATCHER_H

#include <libdbp/cpiofile.h>
#include <libdbp/plugininterface.h>

#include <QtCore/QObject>


class HammerheadRamdiskPatcherPrivate;

class HammerheadRamdiskPatcher : public RamdiskPatcher
{
public:
    explicit HammerheadRamdiskPatcher(const PatcherPaths * const pp,
                                      const QString &id,
                                      const FileInfo * const info,
                                      CpioFile * const cpio);
    ~HammerheadRamdiskPatcher();

    static const QString AOSP;
    static const QString CXL;

    virtual PatcherError::Error error() const;
    virtual QString errorString() const;

    virtual QString name() const;

    virtual bool patchRamdisk();

private:
    const QScopedPointer<HammerheadRamdiskPatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(HammerheadRamdiskPatcher)
};

#endif // HAMMERHEADRAMDISKPATCHER_H
