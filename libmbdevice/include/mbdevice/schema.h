/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of DualBootPatcher
 *
 * DualBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DualBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DualBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "mbcommon/common.h"

#include <algorithm>
#include <memory>
#include <vector>

#include <rapidjson/schema.h>

namespace mb
{
namespace device
{

MB_EXPORT const char * find_schema(const std::string &uri);

template<typename SchemaDocumentType = rapidjson::SchemaDocument>
class DeviceSchemaProvider
    : public rapidjson::IGenericRemoteSchemaDocumentProvider<SchemaDocumentType>
{
public:
    DeviceSchemaProvider()
    {
    }

    virtual ~DeviceSchemaProvider()
    {
    }

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(DeviceSchemaProvider)
    MB_DISABLE_MOVE_CONSTRUCT_AND_ASSIGN(DeviceSchemaProvider)

    virtual const SchemaDocumentType *
    GetRemoteDocument(const char *uri, rapidjson::SizeType length)
    {
        using SchemaDocItem = typename decltype(_schema_docs)::value_type;

        // Find cached SchemaDocument
        {
            auto it = std::find_if(_schema_docs.begin(), _schema_docs.end(),
                                   [uri, length](const SchemaDocItem &item) {
                return item.first.size() == length
                        && memcmp(item.first.data(), uri, length);
            });
            if (it != _schema_docs.end()) {
                return it->second.get();
            }
        }

        // Cache new SchemaDocument
        {
            std::string name{uri, length};
            const char *schema = find_schema(name);
            if (!schema) {
                return nullptr;
            }

            using DocumentType = rapidjson::GenericDocument<
                    typename SchemaDocumentType::EncodingType>;
            DocumentType d;
            if (d.Parse(schema).HasParseError()) {
                assert(false);
                return nullptr;
            }

            std::unique_ptr<SchemaDocumentType> ptr(
                    new SchemaDocumentType(d, this));

            _schema_docs.emplace_back(std::move(name), std::move(ptr));

            return _schema_docs.back().second.get();
        }
    }

    const SchemaDocumentType * GetSchema(const std::string &uri)
    {
        return GetRemoteDocument(
                uri.c_str(), static_cast<rapidjson::SizeType>(uri.size()));
    }

private:
    std::vector<std::pair<std::string, std::unique_ptr<SchemaDocumentType>>> _schema_docs;
};

}
}
