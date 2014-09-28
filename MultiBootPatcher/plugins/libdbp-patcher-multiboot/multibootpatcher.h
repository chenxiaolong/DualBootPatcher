#ifndef MULTIBOOTPATCHER_H
#define MULTIBOOTPATCHER_H

#include <libdbp/plugininterface.h>

// libarchive
#include <archive.h>

class MultiBootPatcherPrivate;

class MultiBootPatcher : public Patcher
{
    Q_OBJECT

public:
    explicit MultiBootPatcher(const PatcherPaths * const pp,
                              const QString &id,
                              QObject *parent = 0);
    ~MultiBootPatcher();

    static const QString Id;
    static const QString Name;

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
    bool patchBootImage(QByteArray *data);
    bool patchZip();
    bool addFile(archive * const a,
                 const QString &path,
                 const QString &name);
    bool addFile(archive * const a,
                 const QByteArray &contents,
                 const QString &name);
    bool readToByteArray(archive *aInput,
                         QByteArray *output) const;
    bool copyData(archive *aInput, archive *aOutput) const;

    bool scanAndPatchBootImages(archive * const aOutput,
                                QStringList *bootImages);
    bool scanAndPatchRemaining(archive * const aOutput,
                               const QStringList &bootImages);

    int scanNumberOfFiles();

private:
    const QScopedPointer<MultiBootPatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(MultiBootPatcher)
};

#endif // MULTIBOOTPATCHER_H
