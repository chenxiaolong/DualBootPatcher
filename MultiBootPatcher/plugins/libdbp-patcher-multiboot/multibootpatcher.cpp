#include "multibootpatcher.h"
#include "multibootpatcher_p.h"

#include <libdbp/bootimage.h>
#include <libdbp/cpiofile.h>
#include <libdbp/patcherpaths.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QStringBuilder>

#include <archive.h>
#include <archive_entry.h>


const QString MultiBootPatcher::Id =
        QStringLiteral("MultiBootPatcher");
const QString MultiBootPatcher::Name =
        tr("Multi Boot Patcher");

static const QChar Sep = QLatin1Char('/');

MultiBootPatcher::MultiBootPatcher(const PatcherPaths * const pp,
                                   const QString &id,
                                   QObject *parent) :
    Patcher(parent), d_ptr(new MultiBootPatcherPrivate())
{
    Q_D(MultiBootPatcher);

    d->pp = pp;
    d->id = id;
}

MultiBootPatcher::~MultiBootPatcher()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error MultiBootPatcher::error() const
{
    Q_D(const MultiBootPatcher);

    return d->errorCode;
}

QString MultiBootPatcher::errorString() const
{
    Q_D(const MultiBootPatcher);

    return d->errorString;
}

QString MultiBootPatcher::id() const
{
    Q_D(const MultiBootPatcher);

    return d->id;
}

QString MultiBootPatcher::name() const
{
    Q_D(const MultiBootPatcher);

    if (d->id == Id) {
        return Name;
    }

    return QString();
}

bool MultiBootPatcher::usesPatchInfo() const
{
    return true;
}

QStringList MultiBootPatcher::supportedPartConfigIds() const
{
    // TODO: Loopify this
    return QStringList()
            << QStringLiteral("dual")
            << QStringLiteral("multi-slot-1")
            << QStringLiteral("multi-slot-2")
            << QStringLiteral("multi-slot-3");
}

void MultiBootPatcher::setFileInfo(const FileInfo * const info)
{
    Q_D(MultiBootPatcher);

    d->info = info;
}

QString MultiBootPatcher::newFilePath()
{
    Q_D(MultiBootPatcher);

    if (d->info == nullptr) {
        qWarning() << "d->info cannot be null!";
        d->errorCode = PatcherError::ImplementationError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return QString();
    }

    QFileInfo fi(d->info->filename());
    return fi.dir().filePath(fi.completeBaseName() % QLatin1Char('_')
            % d->info->partConfig()->id() % QLatin1Char('.') % fi.suffix());
}

bool MultiBootPatcher::patchFile()
{
    Q_D(MultiBootPatcher);

    if (d->info == nullptr) {
        qWarning() << "d->info cannot be null!";
        d->errorCode = PatcherError::ImplementationError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return false;
    }

    if (d->id == Id) {
        return patchZip();
    } else {
        qWarning() << "Invalid patcher ID!";
        d->errorCode = PatcherError::ImplementationError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return false;
    }
}

bool MultiBootPatcher::patchBootImage(QByteArray *data)
{
    Q_D(MultiBootPatcher);

    // Determine patchinfo key for the current file
    QString key = d->info->patchInfo()->keyFromFilename(d->info->filename());

    BootImage bi;
    if (!bi.load(*data)) {
        d->errorCode = bi.error();
        d->errorString = bi.errorString();
        return false;
    }

    // Change the SELinux mode according to the device config
    QString cmdline = bi.kernelCmdline();
    if (!d->info->device()->selinux().isNull()) {
        bi.setKernelCmdline(cmdline
                % QStringLiteral(" androidboot.selinux=")
                % d->info->device()->selinux());
    }

    // Load the ramdisk cpio
    CpioFile cpio;
    cpio.load(bi.ramdiskImage());

    IRamdiskPatcherFactory *f = d->pp->ramdiskPatcherFactory(
            d->info->patchInfo()->ramdisk(key));
    if (f == nullptr) {
        d->errorCode = PatcherError::RamdiskPatcherCreateError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(d->info->patchInfo()->ramdisk(key));
        return false;
    }

    QSharedPointer<RamdiskPatcher> rp =
            f->createRamdiskPatcher(d->pp,
                                    d->info->patchInfo()->ramdisk(key),
                                    d->info,
                                    &cpio);
    if (!rp->patchRamdisk()) {
        d->errorCode = rp->error();
        d->errorString = rp->errorString();
        return false;
    }

    QString mountScript = QStringLiteral("init.multiboot.mounting.sh");
    QFile mountScriptFile(d->pp->scriptsDirectory()
            % QLatin1Char('/') % mountScript);
    if (!mountScriptFile.open(QFile::ReadOnly)) {
        d->errorCode = PatcherError::FileOpenError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(mountScriptFile.fileName());
        return false;
    }

    QByteArray mountScriptContents = mountScriptFile.readAll();
    mountScriptFile.close();

    d->info->partConfig()->replaceShellLine(&mountScriptContents, true);

    if (cpio.exists(mountScript)) {
        cpio.remove(mountScript);
    }

    cpio.addFile(mountScriptContents, mountScript, 0750);

    // Add busybox
    QString busybox = QStringLiteral("sbin/busybox-static");

    if (cpio.exists(busybox)) {
        cpio.remove(busybox);
    }

    cpio.addFile(d->pp->binariesDirectory()
            % QStringLiteral("/busybox-static"), busybox, 0750);

    // Add syncdaemon
    QString syncdaemon = QStringLiteral("sbin/syncdaemon");

    if (cpio.exists(syncdaemon)) {
        cpio.remove(syncdaemon);
    }

    cpio.addFile(d->pp->binariesDirectory() % Sep
            % QStringLiteral("android") % Sep
            % d->info->device()->architecture() % Sep
            % QStringLiteral("syncdaemon"), syncdaemon, 0750);

    // Add patched init binary if it was specified by the patchinfo
    if (!d->info->patchInfo()->patchedInit(key).isEmpty()) {
        if (cpio.exists(QStringLiteral("init"))) {
            cpio.remove(QStringLiteral("init"));
        }

        if (!cpio.addFile(d->pp->initsDirectory() % Sep
                % d->info->patchInfo()->patchedInit(key),
                QStringLiteral("init"), 0755)) {
            d->errorCode = cpio.error();
            d->errorString = cpio.errorString();
            return false;
        }
    }

    QByteArray newRamdisk = cpio.createData(true);
    bi.setRamdiskImage(newRamdisk);

    *data = bi.create();

    return true;
}

bool MultiBootPatcher::patchZip()
{
    Q_D(MultiBootPatcher);

    d->progress = 0;

    // Open the input and output zips with libarchive
    archive *aOutput;

    aOutput = archive_write_new();

    archive_write_set_format_zip(aOutput);
    archive_write_add_filter_none(aOutput);

    // Unlike the old patcher, we'll write directly to the new file
    QString newPath = newFilePath();
    int ret = archive_write_open_filename(
            aOutput, newPath.toUtf8().constData());
    if (ret != ARCHIVE_OK) {
        qWarning() << "libarchive:" << archive_error_string(aOutput);
        archive_write_free(aOutput);

        d->errorCode = PatcherError::LibArchiveWriteOpenError;
        d->errorString = PatcherError::errorString(d->errorCode).arg(newPath);
        return false;
    }

    emit detailsUpdated(tr("Counting number of files in zip file ..."));

    int count = scanNumberOfFiles();
    if (count < 0) {
        archive_write_free(aOutput);
        return false;
    }

    // +1 for dualboot.sh
    emit maxProgressUpdated(count + 1);
    emit progressUpdated(d->progress);

    // Rather than using the extract->patch->repack process, we'll start
    // creating the new zip immediately and then patch the files as we
    // run into them.

    // On the initial pass, find all the boot images, patch them, and
    // write them to the new zip
    QStringList bootImages;

    if (!scanAndPatchBootImages(aOutput, &bootImages)) {
        archive_write_free(aOutput);
        return false;
    }

    // On the second pass, run the autopatchers on the rest of the files

    if (!scanAndPatchRemaining(aOutput, bootImages)) {
        archive_write_free(aOutput);
        return false;
    }

    emit progressUpdated(++d->progress);
    emit detailsUpdated(QStringLiteral("dualboot.sh"));

    // Add dualboot.sh
    QFile dualbootsh(d->pp->scriptsDirectory()
            % QStringLiteral("/dualboot.sh"));
    if (!dualbootsh.open(QFile::ReadOnly)) {
        d->errorCode = PatcherError::FileOpenError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(dualbootsh.fileName());
        return false;
    }

    QByteArray contents = dualbootsh.readAll();
    dualbootsh.close();

    d->info->partConfig()->replaceShellLine(&contents);

    if (!addFile(aOutput, contents, QStringLiteral("dualboot.sh"))) {
        return false;
    }

    archive_write_free(aOutput);

    return true;
}

bool MultiBootPatcher::addFile(archive * const a,
                               const QString &path,
                               const QString &name)
{
    Q_D(MultiBootPatcher);

    QFile file(path);
    if (!file.open(QFile::ReadOnly)) {
        d->errorCode = PatcherError::FileOpenError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(path);
        return false;
    }

    QByteArray contents = file.readAll();

    bool ret = addFile(a, contents, name);

    file.close();
    return ret;
}

bool MultiBootPatcher::addFile(archive * const a,
                               const QByteArray &contents,
                               const QString &name)
{
    Q_D(MultiBootPatcher);

    archive_entry *entry = archive_entry_new();

    archive_entry_set_uid(entry, 0);
    archive_entry_set_gid(entry, 0);
    archive_entry_set_nlink(entry, 1);
    archive_entry_set_mtime(entry, 0, 0);
    archive_entry_set_devmajor(entry, 0);
    archive_entry_set_devminor(entry, 0);
    archive_entry_set_rdevmajor(entry, 0);
    archive_entry_set_rdevminor(entry, 0);

    archive_entry_set_pathname(entry, name.toLocal8Bit().constData());
    archive_entry_set_size(entry, contents.size());

    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);

    // Write header to new file
    if (archive_write_header(a, entry) != ARCHIVE_OK) {
        qWarning() << "libarchive:" << archive_error_string(a);

        d->errorCode = PatcherError::LibArchiveWriteHeaderError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(name);
        return false;
    }

    // Write contents
    archive_write_data(a, contents.constData(), contents.size());

    archive_entry_free(entry);

    return true;
}

bool MultiBootPatcher::readToByteArray(archive *aInput,
                                       QByteArray *output) const
{
    QByteArray data;

    int r;
    __LA_INT64_T offset;
    const void *buff;
    size_t bytes_read;

    while ((r = archive_read_data_block(aInput, &buff,
            &bytes_read, &offset)) == ARCHIVE_OK) {
        data.append(reinterpret_cast<const char *>(buff), bytes_read);
    }

    if (r < ARCHIVE_WARN) {
        return false;
    }

    *output = data;
    return true;
}

bool MultiBootPatcher::copyData(archive *aInput, archive *aOutput) const
{
    int r;
    __LA_INT64_T offset;
    const void *buff;
    size_t bytes_read;

    while ((r = archive_read_data_block(aInput, &buff,
            &bytes_read, &offset)) == ARCHIVE_OK) {
        archive_write_data(aOutput, buff, bytes_read);
    }

    return r >= ARCHIVE_WARN;
}

bool MultiBootPatcher::scanAndPatchBootImages(archive * const aOutput,
                                              QStringList *bootImages)
{
    Q_D(MultiBootPatcher);

    QString key = d->info->patchInfo()->keyFromFilename(d->info->filename());

    // If we don't expect boot images, don't scan for them
    if (!d->info->patchInfo()->hasBootImage(key)) {
        return true;
    }

    archive *aInput = archive_read_new();
    archive_read_support_filter_none(aInput);
    archive_read_support_format_zip(aInput);

    int ret = archive_read_open_filename(
            aInput, d->info->filename().toUtf8().constData(), 10240);
    if (ret != ARCHIVE_OK) {
        qWarning() << "libarchive:" << archive_error_string(aInput);
        archive_read_free(aInput);

        d->errorCode = PatcherError::LibArchiveReadOpenError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(d->info->filename());
        return false;
    }

    archive_entry *entry;

    while (archive_read_next_header(aInput, &entry) == ARCHIVE_OK) {
        const QString curFile =
                QString::fromLocal8Bit(archive_entry_pathname(entry));

        // Try to patch the patchinfo-defined boot images as well as
        // files that end in a common boot image extension
        if (d->info->patchInfo()->bootImages(key).contains(curFile)
                || curFile.endsWith(QStringLiteral(".img"))
                || curFile.endsWith(QStringLiteral(".lok"))) {
            emit progressUpdated(++d->progress);
            emit detailsUpdated(curFile);

            QByteArray data;

            if (!readToByteArray(aInput, &data)) {
                qWarning() << "libarchive:" << archive_error_string(aInput);
                archive_read_free(aInput);

                d->errorCode = PatcherError::LibArchiveReadDataError;
                d->errorString = PatcherError::errorString(d->errorCode)
                        .arg(curFile);
                return false;
            }

            // Make sure the file is a boot iamge and if it is, patch it
            int pos = data.indexOf(QStringLiteral("ANDROID!").toLocal8Bit());
            if (pos >= 0 && pos <= 512) {
                *bootImages << curFile;

                if (!patchBootImage(&data)) {
                    archive_read_free(aInput);
                    return false;
                }

                archive_entry_set_size(entry, data.size());
            }

            // Write header to new file
            if (archive_write_header(aOutput, entry) != ARCHIVE_OK) {
                qWarning() << "libarchive:" << archive_error_string(aOutput);
                archive_read_free(aInput);

                d->errorCode = PatcherError::LibArchiveWriteHeaderError;
                d->errorString = PatcherError::errorString(d->errorCode)
                        .arg(curFile);
                return false;
            }

            archive_write_data(aOutput, data.constData(), data.size());
        } else {
            // Don't process any non-boot-image files
            archive_read_data_skip(aInput);
        }
    }

    archive_read_free(aInput);
    return true;
}

bool MultiBootPatcher::scanAndPatchRemaining(archive * const aOutput,
                                             const QStringList &bootImages)
{
    Q_D(MultiBootPatcher);

    QString key = d->info->patchInfo()->keyFromFilename(d->info->filename());

    archive *aInput = archive_read_new();
    archive_read_support_filter_none(aInput);
    archive_read_support_format_zip(aInput);

    int ret = archive_read_open_filename(
            aInput, d->info->filename().toUtf8().constData(), 10240);
    if (ret != ARCHIVE_OK) {
        qWarning() << "libarchive:" << archive_error_string(aInput);
        archive_read_free(aInput);

        d->errorCode = PatcherError::LibArchiveReadOpenError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(d->info->filename());
        return false;
    }

    QHash<QString, QList<QSharedPointer<AutoPatcher>>> patcherFromFile;

    for (const PatchInfo::AutoPatcherItem &item :
            d->info->patchInfo()->autoPatchers(key)) {
        IAutoPatcherFactory *factory = d->pp->autoPatcherFactory(item.first);
        if (factory == nullptr) {
            archive_read_free(aInput);

            d->errorCode = PatcherError::AutoPatcherCreateError;
            d->errorString = PatcherError::errorString(d->errorCode)
                    .arg(item.first);
            return false;
        }

        // Insert autopatchers in the hash table to make it easier to
        // call them later on
        QSharedPointer<AutoPatcher> ap =
                factory->createAutoPatcher(d->pp, item.first, d->info,
                                           item.second);

        QStringList existingFiles = ap->existingFiles();
        if (existingFiles.isEmpty()) {
            d->errorCode = PatcherError::CustomError;
            d->errorString = tr("Failed to run autopatcher: The %1 plugin says no existing files in the zip file should be patched.").arg(item.first);
            return false;
        }

        for (const QString &file : existingFiles) {
            patcherFromFile[file] << ap;
        }
    }

    archive_entry *entry;

    while (archive_read_next_header(aInput, &entry) == ARCHIVE_OK) {
        const QString curFile =
                QString::fromLocal8Bit(archive_entry_pathname(entry));

        if (d->info->patchInfo()->bootImages(key).contains(curFile)
                || curFile.endsWith(QStringLiteral(".img"))
                || curFile.endsWith(QStringLiteral(".lok"))) {
            // These have already been handled by scanAndPatchBootImages()
            archive_read_data_skip(aInput);
            continue;
        }

        emit progressUpdated(++d->progress);
        emit detailsUpdated(curFile);

        // Go through all the autopatchers
        if (patcherFromFile.contains(curFile)) {
            QByteArray contents;
            if (!readToByteArray(aInput, &contents)) {
                qWarning() << "libarchive:" << archive_error_string(aInput);
                archive_read_free(aInput);

                d->errorCode = PatcherError::LibArchiveReadDataError;
                d->errorString = PatcherError::errorString(d->errorCode)
                        .arg(curFile);
                return false;
            }

            for (const QSharedPointer<AutoPatcher> &ap :
                    patcherFromFile[curFile]) {
                if (!ap->patchFile(curFile, &contents, bootImages)) {
                    archive_read_free(aInput);

                    d->errorCode = ap->error();
                    d->errorString = ap->errorString();
                    return false;
                }
            }

            archive_entry_set_size(entry, contents.size());

            if (archive_write_header(aOutput, entry) != ARCHIVE_OK) {
                qWarning() << "libarchive:" << archive_error_string(aOutput);
                archive_read_free(aInput);

                d->errorCode = PatcherError::LibArchiveWriteHeaderError;
                d->errorString = PatcherError::errorString(d->errorCode)
                        .arg(curFile);
                return false;
            }

            archive_write_data(aOutput, contents.constData(), contents.size());
        } else {
            if (archive_write_header(aOutput, entry) != ARCHIVE_OK) {
                qWarning() << "libarchive:" << archive_error_string(aOutput);
                archive_read_free(aInput);

                d->errorCode = PatcherError::LibArchiveWriteHeaderError;
                d->errorString = PatcherError::errorString(d->errorCode)
                        .arg(curFile);
                return false;
            }

            if (!copyData(aInput, aOutput)) {
                qWarning() << "libarchive:" << archive_error_string(aInput);
                archive_read_free(aInput);

                d->errorCode = PatcherError::LibArchiveWriteDataError;
                d->errorString = PatcherError::errorString(d->errorCode)
                        .arg(curFile);
                return false;
            }
        }
    }

    archive_read_free(aInput);
    return true;
}

int MultiBootPatcher::scanNumberOfFiles()
{
    Q_D(MultiBootPatcher);

    archive *aInput = archive_read_new();
    archive_read_support_filter_none(aInput);
    archive_read_support_format_zip(aInput);

    int ret = archive_read_open_filename(
            aInput, d->info->filename().toUtf8().constData(), 10240);
    if (ret != ARCHIVE_OK) {
        qWarning() << "libarchive:" << archive_error_string(aInput);
        archive_read_free(aInput);

        d->errorCode = PatcherError::LibArchiveReadOpenError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(d->info->filename());
        return -1;
    }

    archive_entry *entry;

    int count = 0;

    while (archive_read_next_header(aInput, &entry) == ARCHIVE_OK) {
        count++;
        archive_read_data_skip(aInput);
    }

    archive_read_free(aInput);
    return count;
}
