/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * This is a slight adaptation of TWRP's implementation. Original source code is
 * available at the following link: https://gerrit.omnirom.org/#/c/6019/
 */

#pragma once

void legacy_get_property_workspace(int *fd, int *sz);
int legacy_property_set(const char *name, const char *value);
void legacy_properties_init();