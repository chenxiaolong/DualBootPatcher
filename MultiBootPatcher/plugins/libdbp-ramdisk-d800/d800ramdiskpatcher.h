#ifndef D800RAMDISKPATCHER_H
#define D800RAMDISKPATCHER_H

#include <libdbp/cpiofile.h>
#include <libdbp/plugininterface.h>

#include <QtCore/QObject>


class D800RamdiskPatcherPrivate;

class D800RamdiskPatcher : public RamdiskPatcher
{
public:
    explicit D800RamdiskPatcher(const PatcherPaths * const pp,
                                const QString &id,
                                const FileInfo * const info,
                                CpioFile * const cpio);
    ~D800RamdiskPatcher();

    static const QString AOSP;

    PatcherError::Error error() const;
    QString errorString() const;

    QString name() const;

    virtual bool patchRamdisk();

private:
    const QScopedPointer<D800RamdiskPatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(D800RamdiskPatcher)
};

#endif // D800RAMDISKPATCHER_H
