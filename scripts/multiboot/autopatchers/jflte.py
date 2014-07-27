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

from multiboot.autopatchers.base import BasePatcher
import multiboot.fileio as fileio


class GoogleEditionPatcher(BasePatcher):
    # WTF? Script removes system/etc/snd_soc_msm/snd_soc_msm_2x_Fusion3_auxpcm
    # causing the ROM to fail to boot

    def __init__(self, **kwargs):
        super(GoogleEditionPatcher, self).__init__(**kwargs)

        self.files_list = ['system/etc/init.qcom.audio.sh']

    def patch(self, directory, file_info, bootimages=None):
        lines = fileio.all_lines(os.path.join(
            directory, 'system/etc/init.qcom.audio.sh'))

        i = 0
        while i < len(lines):
            if 'snd_soc_msm_2x_Fusion3_auxpcm' in lines[i]:
                del lines[i]

            else:
                i += 1

        fileio.write_lines(os.path.join(
            directory, 'system/etc/init.qcom.audio.sh'), lines)
