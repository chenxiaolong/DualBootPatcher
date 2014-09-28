#ifndef CPIOFILE_H
#define CPIOFILE_H

#include "libdbp_global.h"
#include "patchererror.h"

#include <QtCore/QByteArray>
#include <QtCore/QScopedPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

class CpioFilePrivate;

class LIBDBPSHARED_EXPORT CpioFile
{
public:
    CpioFile();
    ~CpioFile();

    PatcherError::Error error() const;
    QString errorString() const;

    bool load(const QByteArray &data);
    QByteArray createData(bool gzip);

    bool exists(const QString &name) const;
    bool remove(const QString &name);

    QStringList filenames() const;

    // File contents

    QByteArray contents(const QString &name) const;
    void setContents(const QString &name, const QByteArray &data);

    // Adding new files

    bool addSymlink(const QString &source, const QString &target);
    bool addFile(const QString &path, const QString &name, uint perms);
    bool addFile(const QByteArray &contents, const QString &name, uint perms);


private:
    const QScopedPointer<CpioFilePrivate> d_ptr;
    Q_DECLARE_PRIVATE(CpioFile)
};

#endif // CPIOFILE_H
