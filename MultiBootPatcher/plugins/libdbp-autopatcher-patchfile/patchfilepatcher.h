#ifndef PATCHFILEPATCHER_H
#define PATCHFILEPATCHER_H

#include <libdbp/plugininterface.h>

#include <QtCore/QtPlugin>

class PatchFilePatcherPrivate;

class PatchFilePatcher : public QObject,
                         public AutoPatcher
{
    Q_OBJECT

public:
    explicit PatchFilePatcher(const PatcherPaths * const pp,
                              const QString &id,
                              const FileInfo * const info,
                              const PatchInfo::AutoPatcherArgs &args,
                              QObject *parent = 0);
    ~PatchFilePatcher();

    static const QString PatchFile;

    virtual PatcherError::Error error() const;
    virtual QString errorString() const;

    virtual QString name() const;

    virtual QStringList newFiles() const;
    virtual QStringList existingFiles() const;

    virtual bool patchFile(const QString &file,
                           QByteArray * const contents,
                           const QStringList &bootImages);

private:
    void skipNewlinesAndAdd(const QString &file,
                            const QString &contents,
                            int begin, int end);

    const QScopedPointer<PatchFilePatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(PatchFilePatcher)
};

#endif // PATCHFILEPATCHER_H
