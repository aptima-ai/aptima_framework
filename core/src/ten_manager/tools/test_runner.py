import os
import shutil
import sys
from pathlib import Path
import subprocess
import platform


def main(executable_path, *args):
    print("tman_test executable path:", executable_path)
    print("tman_test arguments:", args)

    src_path = Path(executable_path).parent

    if platform.system() == "Windows":
        target_filename = "tman_test.exe"
    else:
        target_filename = "tman_test"

    target_path = os.path.join(src_path, target_filename)

    shutil.copy(executable_path, target_path)

    # Prepare the command with arguments passed to the script.
    command = [str(executable_path)] + list(args)
    print("tman_test final test command:", command)

    result = subprocess.run(command, capture_output=True, text=True)

    print("STDOUT:", result.stdout)
    print("STDERR:", result.stderr)

    exit(result.returncode)


if __name__ == "__main__":
    main(*sys.argv[1:])
