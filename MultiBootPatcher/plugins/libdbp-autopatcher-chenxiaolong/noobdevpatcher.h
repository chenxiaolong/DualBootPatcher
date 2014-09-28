#ifndef NOOBDEVPATCHER_H
#define NOOBDEVPATCHER_H

#include <libdbp/plugininterface.h>

#include <QtCore/QtPlugin>

class NoobdevPatcherPrivate;

class NoobdevPatcher : public AutoPatcher
{
public:
    explicit NoobdevPatcher(const PatcherPaths * const pp,
                            const QString &id,
                            const FileInfo * const info);
    ~NoobdevPatcher();

    static const QString NoobdevMultiBoot;
    static const QString NoobdevSystemProp;

    virtual PatcherError::Error error() const;
    virtual QString errorString() const;

    virtual QString name() const;

    virtual QStringList newFiles() const;
    virtual QStringList existingFiles() const;

    virtual bool patchFile(const QString &file,
                           QByteArray * const contents,
                           const QStringList &bootImages);

private:
    const QScopedPointer<NoobdevPatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(NoobdevPatcher)
};

#endif // NOOBDEVPATCHER_H
