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
    googleedition.patcher = [ autopatcher.auto_patch,
                              GoogleEditionPatcher.qcom_audio_fix ]
    googleedition.extractor = [ autopatcher.files_to_auto_patch,
                                GoogleEditionPatcher.files_for_qcom_audio_fix ]

    autopatchers.append(googleedition)

    return autopatchers

