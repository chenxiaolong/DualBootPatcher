/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "primaryupgradepatcher.h"
#include "primaryupgradepatcher_p.h"

#include <libdbp/patcherpaths.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringBuilder>

#include <archive.h>
#include <archive_entry.h>


#define RETURN_IF_CANCELLED \
    if (d->cancelled) { \
        return false; \
    }

#define RETURN_IF_CANCELLED_AND_FREE_READ(x) \
    if (d->cancelled) { \
        archive_read_free(x); \
        return false; \
    }

#define RETURN_IF_CANCELLED_AND_FREE_WRITE(x) \
    if (d->cancelled) { \
        archive_write_free(x); \
        return false; \
    }

#define RETURN_IF_CANCELLED_AND_FREE_READ_WRITE(x, y) \
    if (d->cancelled) { \
        archive_read_free(x); \
        archive_write_free(y); \
        return false; \
    }

#define RETURN_ERROR_IF_CANCELLED \
    if (d->cancelled) { \
        d->errorCode = PatcherError::PatchingCancelled; \
        d->errorString = PatcherError::errorString(d->errorCode); \
        return false; \
    }


const QString PrimaryUpgradePatcher::Id =
        QStringLiteral("PrimaryUpgradePatcher");
const QString PrimaryUpgradePatcher::Name =
        tr("Primary Upgrade Patcher");

const QString UpdaterScript =
        QStringLiteral("META-INF/com/google/android/updater-script");
// MSVC doens't allow split strings inside QStringLiteral ...
const QString PrimaryUpgradePatcher::Format =
        QStringLiteral("run_program(\"/sbin/busybox\", \"find\", \"%1\", \"-maxdepth\", \"1\", \"!\", \"-name\", \"multi-slot-*\", \"!\", \"-name\", \"dual\", \"-delete\");");
const QString PrimaryUpgradePatcher::Mount =
        QStringLiteral("run_program(\"/sbin/busybox\", \"mount\", \"%1\");");
const QString PrimaryUpgradePatcher::Unmount =
        QStringLiteral("run_program(\"/sbin/busybox\", \"umount\", \"%1\");");
const QString PrimaryUpgradePatcher::TempDir =
        QStringLiteral("/tmp/");
const QString PrimaryUpgradePatcher::DualBootTool =
        QStringLiteral("dualboot.sh");
const QString PrimaryUpgradePatcher::SetFacl =
        QStringLiteral("setfacl");
const QString PrimaryUpgradePatcher::GetFacl =
        QStringLiteral("getfacl");
const QString PrimaryUpgradePatcher::SetFattr =
        QStringLiteral("setfattr");
const QString PrimaryUpgradePatcher::GetFattr =
        QStringLiteral("getfattr");
const QString PrimaryUpgradePatcher::PermTool =
        QStringLiteral("backuppermtool.sh");
const QString PrimaryUpgradePatcher::PermsBackup =
        QStringLiteral("run_program(\"/tmp/%1\", \"backup\", \"%2\");");
const QString PrimaryUpgradePatcher::PermsRestore =
        QStringLiteral("run_program(\"/tmp/%1\", \"restore\", \"%2\");");
const QString PrimaryUpgradePatcher::Extract =
        QStringLiteral("package_extract_file(\"%1\", \"%2\");");
//const QString PrimaryUpgradePatcher::MakeExecutable =
//        QStringLiteral("set_perm(0, 0, 0777, \"%1\");");
//const QString PrimaryUpgradePatcher::MakeExecutable =
//        QStringLiteral("set_metadata(\"%1\", \"uid\", 0, \"gid\", 0, \"mode\", 0777);");
const QString PrimaryUpgradePatcher::MakeExecutable =
        QStringLiteral("run_program(\"/sbin/busybox\", \"chmod\", \"0777\", \"%1\");");
const QString PrimaryUpgradePatcher::SetKernel =
        QStringLiteral("run_program(\"/tmp/dualboot.sh\", \"set-multi-kernel\");");

static const QString System = QStringLiteral("/system");
static const QString Cache = QStringLiteral("/cache");

static const QString AndroidBins = QStringLiteral("android/%1");

static const QChar Newline = QLatin1Char('\n');
static const QChar Sep = QLatin1Char('/');


PrimaryUpgradePatcher::PrimaryUpgradePatcher(const PatcherPaths * const pp,
                                             QObject *parent) :
    Patcher(parent), d_ptr(new PrimaryUpgradePatcherPrivate())
{
    Q_D(PrimaryUpgradePatcher);

    d->pp = pp;
}

PrimaryUpgradePatcher::~PrimaryUpgradePatcher()
{
    // Destructor so d_ptr is destroyed
}

QList<PartitionConfig *> PrimaryUpgradePatcher::partConfigs()
{
    // Add primary upgrade partition configuration
    PartitionConfig *config = new PartitionConfig();
    config->setId(QStringLiteral("primaryupgrade"));
    config->setKernel(QStringLiteral("primary"));
    config->setName(tr("Primary ROM Upgrade"));
    config->setDescription(
        tr("Upgrade primary ROM without wiping other ROMs"));

    config->setTargetSystem(QStringLiteral("/system"));
    config->setTargetCache(QStringLiteral("/cache"));
    config->setTargetData(QStringLiteral("/data"));

    config->setTargetSystemPartition(PartitionConfig::System);
    config->setTargetCachePartition(PartitionConfig::Cache);
    config->setTargetDataPartition(PartitionConfig::Data);

    return QList<PartitionConfig *>() << config;
}

PatcherError::Error PrimaryUpgradePatcher::error() const
{
    Q_D(const PrimaryUpgradePatcher);

    return d->errorCode;
}

QString PrimaryUpgradePatcher::errorString() const
{
    Q_D(const PrimaryUpgradePatcher);

    return d->errorString;
}

QString PrimaryUpgradePatcher::id() const
{
    return Id;
}

QString PrimaryUpgradePatcher::name() const
{
    return Name;
}

bool PrimaryUpgradePatcher::usesPatchInfo() const
{
    return false;
}

QStringList PrimaryUpgradePatcher::supportedPartConfigIds() const
{
    return QStringList() << QStringLiteral("primaryupgrade");
}

void PrimaryUpgradePatcher::setFileInfo(const FileInfo * const info)
{
    Q_D(PrimaryUpgradePatcher);

    d->info = info;
}

QString PrimaryUpgradePatcher::newFilePath()
{
    Q_D(PrimaryUpgradePatcher);

    if (d->info == nullptr) {
        qWarning() << "d->info is null!";
        d->errorCode = PatcherError::ImplementationError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return QString();
    }

    QFileInfo fi(d->info->filename());
    return fi.dir().filePath(fi.completeBaseName()
            % QLatin1String("_primaryupgrade.") % fi.suffix());
}

void PrimaryUpgradePatcher::cancelPatching()
{
    Q_D(PrimaryUpgradePatcher);

    d->cancelled = true;
}

bool PrimaryUpgradePatcher::patchFile()
{
    Q_D(PrimaryUpgradePatcher);

    d->cancelled = false;

    if (d->info == nullptr) {
        qWarning() << "d->info is null!";
        d->errorCode = PatcherError::ImplementationError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return false;
    }

    if (!d->info->filename().endsWith(
            QStringLiteral(".zip"), Qt::CaseInsensitive)) {
        d->errorCode = PatcherError::OnlyZipSupported;
        d->errorString = PatcherError::errorString(d->errorCode);
        return false;
    }

    bool ret = patchZip();

    RETURN_ERROR_IF_CANCELLED

    return ret;
}

bool PrimaryUpgradePatcher::patchZip()
{
    Q_D(PrimaryUpgradePatcher);

    d->progress = 0;

    emit detailsUpdated(tr("Counting number of files in zip file ..."));

    int count = scanNumberOfFiles();
    if (count < 0) {
        return false;
    }

    RETURN_IF_CANCELLED

    // 6 extra files
    emit maxProgressUpdated(count + 6);
    emit progressUpdated(d->progress);

    // Open the input and output zips with libarchive
    archive *aInput;
    archive *aOutput;

    aInput = archive_read_new();
    aOutput = archive_write_new();

    archive_read_support_filter_none(aInput);
    archive_read_support_format_zip(aInput);

    archive_write_set_format_zip(aOutput);
    archive_write_add_filter_none(aOutput);

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

    QString newPath = newFilePath();
    ret = archive_write_open_filename(aOutput, newPath.toUtf8().constData());
    if (ret != ARCHIVE_OK) {
        qWarning() << "libarchive:" << archive_error_string(aOutput);
        archive_read_free(aInput);
        archive_write_free(aOutput);

        d->errorCode = PatcherError::LibArchiveWriteOpenError;
        d->errorString = PatcherError::errorString(d->errorCode).arg(newPath);
        return false;
    }

    archive_entry *entry;

    while (archive_read_next_header(aInput, &entry) == ARCHIVE_OK) {
        RETURN_IF_CANCELLED_AND_FREE_READ_WRITE(aInput, aOutput)

        const QString curFile =
                QString::fromLocal8Bit(archive_entry_pathname(entry));

        if (ignoreFile(curFile)) {
            archive_read_data_skip(aInput);
            continue;
        }

        emit progressUpdated(++d->progress);
        emit detailsUpdated(curFile);

        if (curFile == UpdaterScript) {
            QByteArray contents;
            if (!readToByteArray(aInput, &contents)) {
                qWarning() << "libarchive:" << archive_error_string(aInput);
                archive_read_free(aInput);
                archive_write_free(aOutput);

                d->errorCode = PatcherError::LibArchiveReadDataError;
                d->errorString = PatcherError::errorString(d->errorCode)
                        .arg(curFile);
                return false;
            }

            patchUpdaterScript(&contents);
            archive_entry_set_size(entry, contents.size());

            if (archive_write_header(aOutput, entry) != ARCHIVE_OK) {
                qWarning() << "libarchive:" << archive_error_string(aOutput);

                d->errorCode = PatcherError::LibArchiveWriteHeaderError;
                d->errorString = PatcherError::errorString(d->errorCode)
                        .arg(curFile);
                return false;
            }

            archive_write_data(aOutput, contents.constData(), contents.size());
        } else {
            if (archive_write_header(aOutput, entry) != ARCHIVE_OK) {
                qWarning() << "libarchive:" << archive_error_string(aOutput);

                d->errorCode = PatcherError::LibArchiveWriteHeaderError;
                d->errorString = PatcherError::errorString(d->errorCode)
                        .arg(curFile);
                return false;
            }

            if (!copyData(aInput, aOutput)) {
                qWarning() << "libarchive:" << archive_error_string(aInput);

                d->errorCode = PatcherError::LibArchiveReadDataError;
                d->errorString = PatcherError::errorString(d->errorCode)
                        .arg(curFile);
                return false;
            }
        }
    }

    // Add dualboot.sh
    QFile dualbootsh(d->pp->scriptsDirectory() % Sep % DualBootTool);
    if (!dualbootsh.open(QFile::ReadOnly)) {
        d->errorCode = PatcherError::FileOpenError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(dualbootsh.fileName());
        return false;
    }

    QByteArray contents = dualbootsh.readAll();
    dualbootsh.close();

    d->info->partConfig()->replaceShellLine(&contents);

    if (!addFile(aOutput, contents, DualBootTool)) {
        return false;
    }

    emit progressUpdated(++d->progress);
    emit detailsUpdated(DualBootTool);

    if (!addFile(aOutput, d->pp->scriptsDirectory()
            % Sep % PermTool, PermTool)) {
        return false;
    }

    emit progressUpdated(++d->progress);
    emit detailsUpdated(PermTool);

    if (!addFile(aOutput, d->pp->binariesDirectory()
            % Sep % AndroidBins.arg(d->info->device()->architecture())
            % Sep % SetFacl, SetFacl)) {
        return false;
    }

    emit progressUpdated(++d->progress);
    emit detailsUpdated(SetFacl);

    if (!addFile(aOutput, d->pp->binariesDirectory()
            % Sep % AndroidBins.arg(d->info->device()->architecture())
            % Sep % GetFacl, GetFacl)) {
        return false;
    }

    emit progressUpdated(++d->progress);
    emit detailsUpdated(GetFacl);

    if (!addFile(aOutput, d->pp->binariesDirectory()
            % Sep % AndroidBins.arg(d->info->device()->architecture())
            % Sep % SetFattr, SetFattr)) {
        return false;
    }

    emit progressUpdated(++d->progress);
    emit detailsUpdated(SetFattr);

    if (!addFile(aOutput, d->pp->binariesDirectory()
            % Sep % AndroidBins.arg(d->info->device()->architecture())
            % Sep % GetFattr, GetFattr)) {
        return false;
    }

    emit progressUpdated(++d->progress);
    emit detailsUpdated(GetFattr);

    archive_read_free(aInput);
    archive_write_free(aOutput);

    RETURN_IF_CANCELLED

    return true;
}

bool PrimaryUpgradePatcher::patchUpdaterScript(QByteArray *contents)
{
    Q_D(PrimaryUpgradePatcher);

    QStringList lines = QString::fromUtf8(*contents).split(Newline);

    lines.insert(0, Extract.arg(PermTool, TempDir % PermTool));
    lines.insert(1, Extract.arg(DualBootTool, TempDir % DualBootTool));
    lines.insert(2, Extract.arg(SetFacl, TempDir % SetFacl));
    lines.insert(3, Extract.arg(GetFacl, TempDir % GetFacl));
    lines.insert(4, Extract.arg(SetFattr, TempDir % SetFattr));
    lines.insert(5, Extract.arg(GetFattr, TempDir % GetFattr));
    lines.insert(6, MakeExecutable.arg(TempDir % PermTool));
    lines.insert(7, MakeExecutable.arg(TempDir % DualBootTool));
    lines.insert(8, MakeExecutable.arg(TempDir % SetFacl));
    lines.insert(9, MakeExecutable.arg(TempDir % GetFacl));
    lines.insert(10, MakeExecutable.arg(TempDir % SetFattr));
    lines.insert(11, MakeExecutable.arg(TempDir % GetFattr));
    lines.insert(12, Mount.arg(System));
    lines.insert(13, PermsBackup.arg(PermTool).arg(System));
    lines.insert(14, Unmount.arg(System));
    lines.insert(15, Mount.arg(Cache));
    lines.insert(16, PermsBackup.arg(PermTool).arg(Cache));
    lines.insert(17, Unmount.arg(Cache));

    QString pSystem = d->info->device()->partition(QStringLiteral("system"));
    QString pCache = d->info->device()->partition(QStringLiteral("cache"));
    bool replacedFormatSystem = false;
    bool replacedFormatCache = false;

    for (int i = 0; i < lines.size(); i++) {
        if (lines.at(i).contains(QRegularExpression(
                QStringLiteral("^\\s*format\\s*\\(.*$")))) {
            if (lines.at(i).contains(QStringLiteral("system"))
                    || (!pSystem.isEmpty() && lines.at(i).contains(pSystem))) {
                replacedFormatSystem = true;
                lines.removeAt(i);
                i += insertFormatSystem(i, &lines, true);
            } else if (lines.at(i).contains(QStringLiteral("cache"))
                    || (!pCache.isEmpty() && lines.at(i).contains(pCache))) {
                replacedFormatCache = true;
                lines.removeAt(i);
                i += insertFormatCache(i, &lines, true);
            } else {
                i++;
            }
        } else if (lines.at(i).contains(QRegularExpression(
                QStringLiteral("delete_recursive\\s*\\([^\\)]*\"/system\"")))) {
            replacedFormatSystem = true;
            lines.removeAt(i);
            i += insertFormatSystem(i, &lines, false);
        } else if (lines.at(i).contains(QRegularExpression(
                QStringLiteral("delete_recursive\\s*\\([^\\)]*\"/cache\"")))) {
            replacedFormatCache = true;
            lines.removeAt(i);
            i += insertFormatCache(i, &lines, false);
        } else {
            i++;
        }
    }

    if (replacedFormatSystem && replacedFormatCache) {
        d->errorCode = PatcherError::CustomError;
        d->errorString = tr("The patcher could not find any /system or /cache"
                 "formatting lines in the updater-script file.\n\n"
                 "If the file is a ROM, then something failed. If the"
                 "file is not a ROM (eg. kernel or mod), it doesn't"
                 "need to be patched.");
        return false;
    }

    lines << Mount.arg(System);
    lines << PermsRestore.arg(PermTool).arg(System);
    lines << Unmount.arg(System);
    lines << Mount.arg(Cache);
    lines << PermsRestore.arg(PermTool).arg(Cache);
    lines << Unmount.arg(Cache);
    lines << SetKernel;

    *contents = lines.join(Newline).toUtf8();

    return true;
}

int PrimaryUpgradePatcher::insertFormatSystem(int index,
                                              QStringList *lines,
                                              bool mount) const
{
    int i = 0;

    if (mount) {
        lines->insert(index + i, Mount.arg(System));
        i++;
    }

    lines->insert(index + i, Format.arg(System));
    i++;

    if (mount) {
        lines->insert(index + i, Unmount.arg(System));
        i++;
    }

    return i;
}

int PrimaryUpgradePatcher::insertFormatCache(int index,
                                             QStringList *lines,
                                             bool mount) const
{
    int i = 0;

    if (mount) {
        lines->insert(index + i, Mount.arg(Cache));
        i++;
    }

    lines->insert(index + i, Format.arg(Cache));
    i++;

    if (mount) {
        lines->insert(index + i, Unmount.arg(Cache));
        i++;
    }

    return i;
}

bool PrimaryUpgradePatcher::addFile(archive * const a,
                                    const QString &path,
                                    const QString &name)
{
    Q_D(PrimaryUpgradePatcher);

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

bool PrimaryUpgradePatcher::addFile(archive * const a,
                                    const QByteArray &contents,
                                    const QString &name)
{
    Q_D(PrimaryUpgradePatcher);

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

bool PrimaryUpgradePatcher::readToByteArray(archive *aInput,
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

bool PrimaryUpgradePatcher::copyData(archive *aInput, archive *aOutput) const
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

int PrimaryUpgradePatcher::scanNumberOfFiles()
{
    Q_D(PrimaryUpgradePatcher);

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
        RETURN_IF_CANCELLED_AND_FREE_READ(aInput)

        const QString curFile =
                QString::fromLocal8Bit(archive_entry_pathname(entry));
        if (!ignoreFile(curFile)) {
            count++;
        }
        archive_read_data_skip(aInput);
    }

    archive_read_free(aInput);

    RETURN_IF_CANCELLED

    return count;
}

bool PrimaryUpgradePatcher::ignoreFile(const QString& file) const
{
    return file == DualBootTool
            || file == PermTool
            || file == GetFacl
            || file == SetFacl
            || file == GetFattr
            || file == SetFattr;
}
