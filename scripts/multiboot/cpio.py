import multiboot.cmd as cmd
import multiboot.operatingsystem as OS

output    = None
error     = None
error_msg = None


def extract(destination, path=None, data=None):
    global output, error, error_msg

    if not path and not data:
        return False

    if path:
        f = open(path, 'rb')
        data = f.read()
        f.close()

    exit_code, output, error = cmd.run_command(
        [OS.cpio, '-i', '-d', '-m', '-v'],
        stdin_data=data,
        cwd=destination,
        universal_newlines=False
    )

    if exit_code != 0:
        return False
    else:
        return True


def create(base_directory, file_list):
    global output, error, error_msg

    if OS.is_windows():
        # We need the /cygdrive/c/Users/.../ path instead of C:\Users\...\ because
        # a backslash is a valid character in a Unix path. When the list of files
        # is passed to cpio, sbin/adbd, for example, would be included as a file
        # named 'sbin\adbd'

        stdin = "cd '%s' && pwd\n" % OS.binariesdir

        exit_code, output, error = cmd.run_command(
            [OS.bash],
            stdin_data=stdin.encode('UTF-8'),
            cwd=base_directory,
            universal_newlines=False
        )

        if exit_code != 0:
            error_msg = 'Failed to get Cygwin drive path'
            return None

        cpio_cygpath = output.decode('UTF-8').strip('\n') + '/cpio.exe'

        stdin = cpio_cygpath              \
            + ' -o -H newc << EOF\n'      \
            + '\n'.join(file_list) + '\n' \
            + 'EOF\n'

        # We cannot use "bash -c '...'" because the /cygdrive/ mountpoints are
        # created only in an interactive shell. We'll launch bash first and then
        # run cpio.
        exit_code, output, error = cmd.run_command(
            [OS.bash],
            stdin_data=stdin.encode('UTF-8'),
            cwd=base_directory,
            universal_newlines=False
        )

    else:
        # So much easier than in Windows ...
        exit_code, output, error = cmd.run_command(
            [OS.cpio, '-o', '-H', 'newc'],
            stdin_data='\n'.join(file_list).encode('UTF-8'),
            cwd=base_directory,
            universal_newlines=False
        )

    if exit_code != 0:
        error_msg = 'Failed to create cpio archive'
        return None

    return output
