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

#include "cpiofile.h"
#include "cpiofile_p.h"

// libarchive
#include <archive.h>
#include <archive_entry.h>

#include <QtCore/QDebug>
#include <QtCore/QFile>


CpioFilePrivate::~CpioFilePrivate()
{
    for (QPair<archive_entry *, QByteArray> &p : files) {
        archive_entry_free(p.first);
    }
}

// --------------------------------

CpioFile::CpioFile() : d_ptr(new CpioFilePrivate())
{
}

CpioFile::~CpioFile()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error CpioFile::error() const
{
    Q_D(const CpioFile);

    return d->errorCode;
}

QString CpioFile::errorString() const
{
    Q_D(const CpioFile);

    return d->errorString;
}

bool CpioFile::load(const QByteArray &data)
{
    Q_D(CpioFile);

    archive *a;
    archive_entry *entry;

    a = archive_read_new();

    // Allow gzip-compressed cpio files to work as well
    // (libarchive is awesome)
    archive_read_support_filter_gzip(a);
    archive_read_support_format_cpio(a);

    int ret = archive_read_open_memory(a,
            const_cast<char *>(data.constData()), data.size());
    if (ret != ARCHIVE_OK) {
        qWarning() << "libarchive:" << archive_error_string(a);
        archive_read_free(a);

        d->errorCode = PatcherError::LibArchiveReadOpenError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return false;
    }

    while ((ret = archive_read_next_header(a, &entry)) == ARCHIVE_OK) {
        // Read the data for the entry
        QByteArray entryData;
        entryData.reserve(archive_entry_size(entry));

        int r;
        __LA_INT64_T offset;
        const void *buff;
        size_t bytes_read;

        while ((r = archive_read_data_block(a, &buff,
                &bytes_read, &offset)) == ARCHIVE_OK) {
            entryData.append(reinterpret_cast<const char *>(buff), bytes_read);
        }

        if (r < ARCHIVE_WARN) {
            qWarning() << "libarchive:" << archive_error_string(a);
            archive_read_free(a);

            d->errorCode = PatcherError::LibArchiveReadDataError;
            d->errorString = PatcherError::errorString(d->errorCode)
                    .arg(QString::fromLocal8Bit(archive_entry_pathname(entry)));
            return false;
        }

        // Save the header and data
        archive_entry *cloned = archive_entry_clone(entry);
        d->files.append(qMakePair(cloned, entryData));
    }

    if (ret < ARCHIVE_WARN) {
        qWarning() << "libarchive:" << archive_error_string(a);
        archive_read_free(a);

        d->errorCode = PatcherError::LibArchiveReadHeaderError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return false;
    }

    ret = archive_read_free(a);
    if (ret != ARCHIVE_OK) {
        qWarning() << "libarchive:" << archive_error_string(a);

        d->errorCode = PatcherError::LibArchiveFreeError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return false;
    }

    return true;
}

static int archiveOpenCallback(archive *a, void *clientData)
{
    Q_UNUSED(a);
    Q_UNUSED(clientData);
    return ARCHIVE_OK;
}

static __LA_SSIZE_T archiveWriteCallback(archive *, void *clientData,
                                         const void *buffer, size_t length)
{
    QByteArray *data = reinterpret_cast<QByteArray *>(clientData);
    data->append(reinterpret_cast<const char *>(buffer), length);
    return length;
}

static int archiveCloseCallback(archive *a, void *clientData)
{
    Q_UNUSED(a);
    Q_UNUSED(clientData);
    return ARCHIVE_OK;
}

static bool sortByName(const QPair<archive_entry *, QByteArray> &p1,
                       const QPair<archive_entry *, QByteArray> &p2)
{
    const char *cname1 = archive_entry_pathname(p1.first);
    const char *cname2 = archive_entry_pathname(p2.first);
    return qstrcmp(cname1, cname2) < 0;
}

QByteArray CpioFile::createData(bool gzip)
{
    Q_D(CpioFile);

    archive *a;
    QByteArray data;

    a = archive_write_new();

    archive_write_set_format_cpio_newc(a);

    if (gzip) {
        archive_write_add_filter_gzip(a);
    } else {
        archive_write_add_filter_none(a);
    }

    archive_write_set_bytes_per_block(a, 512);

    int ret = archive_write_open(a, reinterpret_cast<void *>(&data),
                                 &archiveOpenCallback,
                                 &archiveWriteCallback,
                                 &archiveCloseCallback);
    if (ret != ARCHIVE_OK) {
        qWarning() << "libarchive:" << archive_error_string(a);
        archive_write_fail(a);
        archive_write_free(a);

        d->errorCode = PatcherError::LibArchiveWriteOpenError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return QByteArray();
    }

    for (const QPair<archive_entry *, QByteArray> &p : d->files) {
        if (archive_write_header(a, p.first) != ARCHIVE_OK) {
            qWarning() << "libarchive:"
                       << archive_error_string(a) << ":"
                       << archive_entry_pathname(p.first);

            archive_write_fail(a);
            archive_write_free(a);

            d->errorCode = PatcherError::LibArchiveWriteHeaderError;
            d->errorString = PatcherError::errorString(d->errorCode)
                    .arg(QString::fromLocal8Bit(archive_entry_pathname(p.first)));
            return QByteArray();
        }

        if (archive_write_data(a, p.second.constData(), p.second.size())
                != p.second.size()) {
            archive_write_fail(a);
            archive_write_free(a);

            d->errorCode = PatcherError::LibArchiveWriteDataError;
            d->errorString = PatcherError::errorString(d->errorCode)
                    .arg(QString::fromLocal8Bit(archive_entry_pathname(p.first)));
            return QByteArray();
        }
    }

    ret = archive_write_close(a);

    if (ret != ARCHIVE_OK) {
        qWarning() << archive_error_string(a);
        archive_write_fail(a);
        archive_write_free(a);

        d->errorCode = PatcherError::LibArchiveCloseError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return QByteArray();
    }

    archive_write_free(a);

    return data;
}

bool CpioFile::exists(const QString &name) const
{
    Q_D(const CpioFile);

    for (const QPair<archive_entry *, QByteArray> &p : d->files) {
        if (name == QString::fromLocal8Bit(archive_entry_pathname(p.first))) {
            return true;
        }
    }

    return false;
}

bool CpioFile::remove(const QString &name)
{
    Q_D(CpioFile);

    QMutableListIterator<QPair<archive_entry *, QByteArray>> i(d->files);
    while (i.hasNext()) {
        archive_entry *entry = i.next().first;
        if (name == QString::fromLocal8Bit(archive_entry_pathname(entry))) {
            archive_entry_free(entry);
            i.remove();
            return true;
        }
    }

    return false;
}

QStringList CpioFile::filenames() const
{
    Q_D(const CpioFile);

    QStringList list;

    for (const QPair<archive_entry *, QByteArray> &p : d->files) {
        list << QString::fromLocal8Bit(archive_entry_pathname(p.first));
    }

    return list;
}

QByteArray CpioFile::contents(const QString &name) const
{
    Q_D(const CpioFile);

    for (const QPair<archive_entry *, QByteArray> &p : d->files) {
        if (name == QString::fromLocal8Bit(archive_entry_pathname(p.first))) {
            return p.second;
        }
    }

    return QByteArray();
}

void CpioFile::setContents(const QString &name, const QByteArray &data)
{
    Q_D(CpioFile);

    for (QPair<archive_entry *, QByteArray> &p : d->files) {
        if (name == QString::fromLocal8Bit(archive_entry_pathname(p.first))) {
            archive_entry_set_size(p.first, data.size());
            p.second = data;
            break;
        }
    }
}

bool CpioFile::addSymlink(const QString &source, const QString &target)
{
    Q_D(CpioFile);

    if (exists(target)) {
        d->errorCode = PatcherError::CpioFileAlreadyExistsError;
        d->errorString = PatcherError::errorString(d->errorCode).arg(target);
        return false;
    }

    archive_entry *entry = archive_entry_new();

    archive_entry_set_uid(entry, 0);
    archive_entry_set_gid(entry, 0);
    archive_entry_set_nlink(entry, 1);
    archive_entry_set_mtime(entry, 0, 0);
    archive_entry_set_devmajor(entry, 0);
    archive_entry_set_devminor(entry, 0);
    archive_entry_set_rdevmajor(entry, 0);
    archive_entry_set_rdevminor(entry, 0);

    archive_entry_set_pathname(entry, target.toLocal8Bit().constData());
    archive_entry_set_size(entry, 0);
    archive_entry_set_symlink(entry, source.toLocal8Bit().constData());

    archive_entry_set_filetype(entry, AE_IFLNK);
    archive_entry_set_perm(entry, 0777);

    d->files.append(qMakePair(entry, QByteArray()));

    std::sort(d->files.begin(), d->files.end(), sortByName);

    return true;
}

bool CpioFile::addFile(const QString &path, const QString &name, uint perms)
{
    Q_D(CpioFile);

    if (exists(name)) {
        d->errorCode = PatcherError::CpioFileAlreadyExistsError;
        d->errorString = PatcherError::errorString(d->errorCode).arg(name);
        return false;
    }

    QFile file(path);
    if (!file.open(QFile::ReadOnly)) {
        d->errorCode = PatcherError::FileOpenError;
        d->errorString = PatcherError::errorString(d->errorCode).arg(path);
        return false;
    }

    QByteArray contents = file.readAll();
    file.close();

    return addFile(contents, name, perms);
}

bool CpioFile::addFile(const QByteArray &contents, const QString &name, uint perms)
{
    Q_D(CpioFile);

    if (exists(name)) {
        d->errorCode = PatcherError::CpioFileAlreadyExistsError;
        d->errorString = PatcherError::errorString(d->errorCode).arg(name);
        return false;
    }

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
    archive_entry_set_perm(entry, perms);

    d->files.append(qMakePair(entry, contents));

    std::sort(d->files.begin(), d->files.end(), sortByName);

    return true;
}
