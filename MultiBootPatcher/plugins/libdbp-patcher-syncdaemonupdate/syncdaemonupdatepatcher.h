#ifndef SYNCDAEMONUPDATEPATCHER_H
#define SYNCDAEMONUPDATEPATCHER_H

#include <libdbp/plugininterface.h>

class SyncdaemonUpdatePatcherPrivate;

class SyncdaemonUpdatePatcher : public Patcher
{
    Q_OBJECT

public:
    explicit SyncdaemonUpdatePatcher(const PatcherPaths * const pp,
                                     const QString &id);
    ~SyncdaemonUpdatePatcher();

    static const QString Id;
    static const QString Name;

    // Error reporting
    virtual PatcherError::Error error() const;
    virtual QString errorString() const;

    // Patcher info
    virtual QString id() const;
    virtual QString name() const;
    virtual bool usesPatchInfo() const;
    virtual QStringList supportedPartConfigIds() const;

    // Patching
    virtual void setFileInfo(const FileInfo * const info);

    virtual QString newFilePath();

    virtual bool patchFile();

private:
    bool patchImage();
    QString findPartConfigId(const CpioFile * const cpio) const;

private:
    const QScopedPointer<SyncdaemonUpdatePatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(SyncdaemonUpdatePatcher)
};
#endif // SYNCDAEMONUPDATEPATCHER_H
