# Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import multiboot.autopatcher as autopatcher
import multiboot.fileio as fileio


class GoogleEditionPatcher:
    # WTF? Script removes system/etc/snd_soc_msm/snd_soc_msm_2x_Fusion3_auxpcm
    # causing the ROM to fail to boot
    @staticmethod
    def qcom_audio_fix(directory, bootimg=None, device_check=True,
                       partition_config=None, device=None):
        lines = fileio.all_lines('system/etc/init.qcom.audio.sh',
                                 directory=directory)

        i = 0
        while i < len(lines):
            if 'snd_soc_msm_2x_Fusion3_auxpcm' in lines[i]:
                del lines[i]

            else:
                i += 1

        fileio.write_lines('system/etc/init.qcom.audio.sh', lines,
                           directory=directory)

    @staticmethod
    def files_for_qcom_audio_fix():
        return ['system/etc/init.qcom.audio.sh']


def get_auto_patchers():
    autopatchers = list()

    googleedition = autopatcher.AutoPatcher()
    googleedition.name = 'Standard + Google Edition Fix'
    googleedition.patcher = [autopatcher.auto_patch,
                             GoogleEditionPatcher.qcom_audio_fix]
    googleedition.extractor = [autopatcher.files_to_auto_patch,
                               GoogleEditionPatcher.files_for_qcom_audio_fix]

    autopatchers.append(googleedition)

    return autopatchers
