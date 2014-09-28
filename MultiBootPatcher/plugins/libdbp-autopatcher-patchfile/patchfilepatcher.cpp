#include "patchfilepatcher.h"
#include "patchfilepatcher_p.h"

#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QRegularExpressionMatch>
#include <QtCore/QRegularExpressionMatchIterator>
#include <QtCore/QStringBuilder>
#include <QtCore/QTemporaryDir>


#define PATCH_FAIL_MSG "Failed to run \"patch\" command"

const QString PatchFilePatcher::PatchFile = QStringLiteral("PatchFile");

static const QString ArgFile = QStringLiteral("file");
static const QChar Newline = QLatin1Char('\n');
static const QChar Sep = QLatin1Char('/');

PatchFilePatcher::PatchFilePatcher(const PatcherPaths * const pp,
                                   const QString &id,
                                   const FileInfo * const info,
                                   const PatchInfo::AutoPatcherArgs &args,
                                   QObject *parent) :
    QObject(parent), d_ptr(new PatchFilePatcherPrivate())
{
    Q_D(PatchFilePatcher);

    d->pp = pp;
    d->id = id;
    d->info = info;
    d->args = args;

    // The arguments should have the patch to the file
    if (!args.contains(ArgFile)) {
        qWarning() << "Arguments does not contain the path to a patch file:" << args;
        return;
    }

    QFile file(pp->patchesDirectory() % Sep % args[ArgFile]);
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "Failed to open" << file.fileName();
        return;
    }

    QByteArray patchContents = file.readAll();

    // Assume the patch file is UTF-8 encoded
    QString contents = QString::fromUtf8(patchContents);

    QRegularExpression reOrigFile(QStringLiteral(
            "---\\s+(?<file>.+)(?:\n|\t)"));

    QString curFile;
    int begin = -1;

    QRegularExpressionMatchIterator iter = reOrigFile.globalMatch(contents);
    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString file = match.captured(QStringLiteral("file"));
        if (file.startsWith(QLatin1Char('"')) && file.endsWith(QLatin1Char('"'))) {
            file = file.mid(1, file.size() - 2);
        }

        if (begin != -1) {
            skipNewlinesAndAdd(curFile, contents, begin,
                               match.capturedStart() - 1);
        }

        // Skip files containing escaped characters
        if (file.contains(QLatin1Char('\\'))) {
            begin = -1;
        } else {
            curFile = file;
            begin = match.capturedStart();
        }
    }

    skipNewlinesAndAdd(curFile, contents, begin, contents.size() - 1);

    file.close();
}

PatchFilePatcher::~PatchFilePatcher()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error PatchFilePatcher::error() const
{
    Q_D(const PatchFilePatcher);

    return d->errorCode;
}

QString PatchFilePatcher::errorString() const
{
    Q_D(const PatchFilePatcher);

    return d->errorString;
}

QString PatchFilePatcher::name() const
{
    Q_D(const PatchFilePatcher);

    return d->id;
}

QStringList PatchFilePatcher::newFiles() const
{
    return QStringList();
}

QStringList PatchFilePatcher::existingFiles() const
{
    Q_D(const PatchFilePatcher);

    if (d->id == PatchFile) {
        return d->patches.keys();
    }

    return QStringList();
}

bool PatchFilePatcher::patchFile(const QString &file,
                                 QByteArray * const contents,
                                 const QStringList &bootImages)
{
    Q_D(PatchFilePatcher);
    Q_UNUSED(bootImages);

    if (d->id != PatchFile) {
        qWarning() << "Invalid ID:" << d->id;
        d->errorCode = PatcherError::ImplementationError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return false;
    }

    if (d->patches.isEmpty()) {
        d->errorCode = PatcherError::CustomError;
        d->errorString = tr("Failed to parse patch file: %1").arg(d->args[ArgFile]);
        return false;
    }

    // Write the file to be patched to a temporary file
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        d->errorCode = PatcherError::CustomError;
        d->errorString = tr("Failed to create temporary directory");
        return false;
    }

    if (d->patches.isEmpty()) {
        d->errorCode = PatcherError::CustomError;
        d->errorString = tr("The patch file is empty");
        return false;
    }

    QString path(tempDir.path() % QStringLiteral("/file"));
    QFile f(path);
    f.open(QFile::WriteOnly);
    f.write(*contents);
    f.close();

#if defined(Q_OS_ANDROID)
    QString patch = d->pp->binariesDirectory() % Sep
            % QStringLiteral("android") % Sep
            % d->info->architecture() % Sep
            % QStringLiteral("patch");
#elif defined(Q_OS_WIN32)
    QString patch = d->pp->binariesDirectory() % Sep
            % QStringLiteral("windows") % Sep
            % QStringLiteral("hctap.exe");
#else
    QString patch = QStringLiteral("patch");
#endif

    QProcess process;
    process.setWorkingDirectory(tempDir.path());
    process.start(patch,
                  QStringList() << QStringLiteral("--no-backup-if-mismatch")
                                << QStringLiteral("-p1")
                                << QStringLiteral("-d")
                                << tempDir.path());
    if (!process.waitForStarted()) {
        d->errorCode = PatcherError::CustomError;
        d->errorString = tr(PATCH_FAIL_MSG);
        return false;
    }

    process.write(QStringLiteral("--- a/file\n").toUtf8());
    process.write(QStringLiteral("+++ b/file\n").toUtf8());
    process.write(d->patches[file]);
    process.closeWriteChannel();

    if (!process.waitForFinished()) {
        d->errorCode = PatcherError::CustomError;
        d->errorString = tr(PATCH_FAIL_MSG);
        return false;
    }

    // Print console output
    qDebug() << process.readAll();

    bool ret = process.exitStatus() == QProcess::NormalExit
            && process.exitCode() == 0;

    if (!ret) {
        d->errorCode = PatcherError::CustomError;
        d->errorString = tr(PATCH_FAIL_MSG);
    } else {
        f.open(QFile::ReadOnly);
        *contents = f.readAll();
        f.close();
    }

    return ret;
}

void PatchFilePatcher::skipNewlinesAndAdd(const QString &file,
                                          const QString &contents,
                                          int begin, int end)
{
    Q_D(PatchFilePatcher);

    // Strip 1st component if possible
    QString actualFile;
    int slashPos = file.indexOf(Sep);
    if (slashPos >= 0 && file.length() > slashPos) {
        actualFile = file.mid(slashPos + 1);
    } else {
        actualFile = file;
    }

    // Skip two newlines (for '^---' and '^+++')
    int newline1 = contents.indexOf(Newline, begin);
    if (newline1 > 0 && (newline1 + 1) < contents.size()) {
        int newline2 = contents.indexOf(Newline, newline1 + 1);
        if (newline2 > 0 && (newline2 + 1) < contents.size()) {
            d->patches[actualFile] =
                    contents.mid(newline2 + 1, end - newline2).toUtf8();
        }
    }
}
