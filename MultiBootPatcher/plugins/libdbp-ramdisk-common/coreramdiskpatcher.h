#ifndef CORERAMDISKPATCHER_H
#define CORERAMDISKPATCHER_H

#include "libdbp-ramdisk-common_global.h"

#include <libdbp/cpiofile.h>
#include <libdbp/plugininterface.h>


class CoreRamdiskPatcherPrivate;

class LIBDBPRAMDISKCOMMONSHARED_EXPORT CoreRamdiskPatcher : public RamdiskPatcher
{
public:
    explicit CoreRamdiskPatcher(const PatcherPaths * const pp,
                                const FileInfo * const info,
                                CpioFile * const cpio);
    ~CoreRamdiskPatcher();

    static const QString FstabRegex;
    static const QString ExecMount;
    static const QString PropPartconfig;
    static const QString PropVersion;
    static const QString SyncdaemonService;

    virtual PatcherError::Error error() const;
    virtual QString errorString() const;

    virtual QString name() const;

    virtual bool patchRamdisk();

    bool modifyDefaultProp();
    bool addSyncdaemon();
    bool removeAndRelinkBusybox();

private:
    const QScopedPointer<CoreRamdiskPatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(CoreRamdiskPatcher)
};

#endif // CORERAMDISKPATCHER_H
