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

#include "cwrapper/cstringmap.h"

#include <cstdlib>
#include <cstring>

#include <unordered_map>


/*!
 * \file cstringmap.h
 * \brief C Wrapper for a C++ map with string keys and values
 */

extern "C" {

    typedef std::unordered_map<std::string, std::string> MapType;

    /*!
     * \brief Create a new map
     *
     * \note The returned object must be freed with mbp_stringmap_destroy().
     *
     * \return New CStringMap
     */
    CStringMap * mbp_stringmap_create()
    {
        return reinterpret_cast<CStringMap *>(new MapType());
    }

    /*!
     * \brief Destroys a CStringMap object.
     *
     * \param map CStringMap to destroy
     */
    void mbp_stringmap_destroy(CStringMap *map)
    {
        delete reinterpret_cast<MapType *>(map);
    }

    /*!
     * \brief Get list of keys in CStringMap
     *
     * \note The returned array should be freed with `mbp_free_array()` when it
     *       is no longer needed.
     *
     * \param map CStringMap object
     *
     * \return NULL-terminated list of keys
     */
    char ** mbp_stringmap_keys(const CStringMap *map)
    {
        const MapType *m = reinterpret_cast<const MapType *>(map);

        unsigned int i = 0;
        char **keys = (char **) std::malloc(sizeof(char *) * (m->size() + 1));
        for (auto const &p : *m) {
            keys[i] = strdup(p.first.c_str());
            ++i;
        }
        keys[m->size()] = nullptr;

        return keys;
    }

    /*!
     * \brief Get value for key in CStringMap
     *
     * \param map CStringMap object
     *
     * \return Value or NULL if key is not in the map
     */
    char * mbp_stringmap_get(const CStringMap *map, const char *key)
    {
        const MapType *m = reinterpret_cast<const MapType *>(map);

        if (m->find(key) != m->end()) {
            return strdup(m->at(key).c_str());
        }

        return nullptr;
    }

    /*!
     * \brief Add or set value in CStringMap
     *
     * \param map CStringMap object
     * \param key Key
     * \param value Value
     */
    void mbp_stringmap_set(CStringMap *map, const char *key, const char *value)
    {
        MapType *m = reinterpret_cast<MapType *>(map);
        (*m)[key] = value;
    }

    /*!
     * \brief Remove key from CStringMap
     *
     * \param map CStringMap object
     * \param key Key
     */
    void mbp_stringmap_remove(CStringMap *map, const char *key)
    {
        MapType *m = reinterpret_cast<MapType *>(map);
        auto it = m->find(key);
        if (it != m->end()) {
            m->erase(it);
        }
    }

    /*!
     * \brief Clear CStringMap
     *
     * \param map CStringMap object
     */
    void mbp_stringmap_clear(CStringMap *map)
    {
        MapType *m = reinterpret_cast<MapType *>(map);
        m->clear();
    }

}
