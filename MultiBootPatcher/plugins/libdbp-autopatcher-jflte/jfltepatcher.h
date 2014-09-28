#ifndef JFLTEPATCHER_H
#define JFLTEPATCHER_H

#include <libdbp/plugininterface.h>

#include <QtCore/QtPlugin>

class JfltePatcherPrivate;

class JfltePatcher : public AutoPatcher
{
public:
    explicit JfltePatcher(const PatcherPaths * const pp,
                          const QString &id,
                          const FileInfo * const info);
    ~JfltePatcher();

    static const QString DalvikCachePatcher;
    static const QString GoogleEditionPatcher;
    static const QString SlimAromaBundledMount;
    static const QString ImperiumPatcher;
    static const QString NegaliteNoWipeData;
    static const QString TriForceFixAroma;
    static const QString TriForceFixUpdate;

    virtual PatcherError::Error error() const;
    virtual QString errorString() const;

    virtual QString name() const;

    virtual QStringList newFiles() const;
    virtual QStringList existingFiles() const;

    virtual bool patchFile(const QString &file,
                           QByteArray * const contents,
                           const QStringList &bootImages);

private:
    const QScopedPointer<JfltePatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(JfltePatcher)
};

#endif // JFLTEPATCHER_H
