#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import sys
import stat
import os
import shutil
import tempfile
from . import log
import subprocess
import inspect


def mkdir_p(path: str) -> None:
    if sys.version > "3":
        os.makedirs(path, exist_ok=True)
    else:
        raise Exception("tgn supports Python 3 only")


def remove_tree(path: str) -> None:
    if os.path.exists(path):
        if sys.platform == "win32":
            # There are many corner cases in Windows which will prevent the
            # target path from deleting.
            #
            # Therefore, we use robocopy to empty a folder before deleting it in
            # Windows. robocopy could handle more cases in Windows when
            # deleting a folder.
            #
            # 1. Create an empty tmp folder.
            tmp_folder = tempfile.mkdtemp()

            # 2. Use 'robocopy' to clear the contents of the target folder. The
            # important argument is '/mir'.
            rt = subprocess.call(
                [
                    "robocopy",
                    tmp_folder,
                    path,
                    "/r:3",
                    "/w:2",
                    "/mir",
                    "/purge",
                    "/mt",
                    "/LOG:robocopy_remove_tree_log.txt",
                ]
            )
            if rt >= 8:
                log.error(f"Failed to remove_tree({path}): {str(rt)}")
                exit(rt)

            # 3. The target folder is empty now, its safe to remove it.
            shutil.rmtree(path)

            # 4. Remove the tmp folder, too.
            shutil.rmtree(tmp_folder)
        else:
            shutil.rmtree(path)


def copy_tree(src_path: str, dst_path: str, rm_dst=False) -> None:
    if not os.path.exists(src_path):
        raise Exception(src_path + " not exist")

    if not os.path.isdir(src_path):
        raise Exception(src_path + " is not a directory.")

    try:
        if sys.platform == "win32":
            # Use 'robocopy' in Windows to perform copying.
            cmd = [
                "robocopy",
                src_path,
                dst_path,
                # Copies subdirectories. This option automatically includes
                # empty directories.
                "/e",
                # Creates multi-threaded copies with n threads. The default
                # value for n is 8.
                "/mt",
                # Specifies the number of retries on failed copies.
                "/r:30",
                # Specifies the wait time between retries, in seconds.
                "/w:3",
                # Specifies that there's no job header.
                "/njh",
                # Specifies that there's no job summary.
                "/njs",
                # Specifies that directory names aren't to be logged.
                "/ndl",
                # Specifies that file classes aren't to be logged.
                "/nc",
                # Specifies that file sizes aren't to be logged.
                "/ns",
                # Specifies that the progress of the copying operation (the
                # number of files or directories copied so far) won't be
                # displayed.
                "/np",
                # Specifies that file names aren't to be logged.
                "/nfl",
                "/LOG:robocopy_tree_log.txt",
                # Specifies what to copy in directories. DA (data and
                # attributes).
                "/dcopy:DA",
            ]

            if rm_dst is True:
                cmd += [
                    # Mirrors a directory tree (equivalent to /e plus /purge).
                    "/mir"
                ]

            rt = subprocess.call(cmd)
            if rt >= 8:
                log.error(
                    f"Failed to copy_tree: {src_path} => {dst_path}: {str(rt)}"
                )
                exit(rt)
        else:
            try:
                if rm_dst:
                    remove_tree(dst_path)

                    # shutil.copytree requires the destination folder is empty,
                    # otherwise, this function will throw exceptions. So use it
                    # in this case.
                    shutil.copytree(
                        src_path,
                        dst_path,
                        symlinks=True,
                        copy_function=shutil.copy,
                    )

                # Loop all immediate items in src_path, and copy them one by
                # one.
                for name in os.listdir(src_path):
                    src_item_path = os.path.join(src_path, name)
                    dest_item_path = os.path.join(dst_path, name)

                    if os.path.isdir(src_item_path):
                        # Create the destination folder if not exist.
                        if not os.path.exists(dest_item_path):
                            os.makedirs(dest_item_path, exist_ok=True)
                        copy_tree(src_item_path, dest_item_path)
                    else:
                        copy_file(src_item_path, dest_item_path, rm_dst=rm_dst)
            except Exception as exc:
                log.error(
                    inspect.cleandoc(
                        f"""Failed to copy tree:
                        {src_path} =>
                        {dst_path}
                        Exception: {exc}
                        rm_dst: {rm_dst}"""
                    )
                )
                raise

        try:
            # Update the file access and modify time to the current time.
            os.utime(dst_path, follow_symlinks=False)
        except Exception:
            try:
                # If follow_symlinks parameter is not supported, fall back to
                # default behavior
                os.utime(dst_path)
            except Exception as exc:
                log.error(
                    inspect.cleandoc(
                        f"""Failed to copy_tree:
                    {src_path} =>
                    {dst_path}
                    Exception: {exc}
                    rm_dst: {rm_dst}"""
                    )
                )
                exit(1)

    except Exception as exc:
        log.error(
            inspect.cleandoc(
                f"""Failed to copy_tree:
            {src_path} =>
            {dst_path}
            Exception: {exc}
            rm_dst: {rm_dst}"""
            )
        )
        exit(1)


def copy_file(src_file: str, dst_file: str, rm_dst=False) -> None:
    if not os.path.exists(src_file):
        raise Exception(f"{src_file} not exist")

    if os.path.isdir(src_file):
        raise Exception(f"{src_file} is a directory.")

    src_file = os.path.abspath(src_file)
    dst_file = os.path.abspath(dst_file)

    try:
        mkdir_p(os.path.dirname(dst_file))
    except Exception as exc:
        log.error(
            inspect.cleandoc(
                f"""Failed to create destination folder before copy_file:
                {src_file} =>
                {dst_file}
                Exception: {exc}
                rm_dst: {rm_dst}"""
            )
        )
        exit(1)

    try:
        if rm_dst and os.path.exists(dst_file):
            os.remove(dst_file)
    except Exception as exc:
        log.error(
            inspect.cleandoc(
                f"""Failed to delete dst_file before copy_file:
                {src_file} =>
                {dst_file}
                Exception: {exc}
                rm_dst: {rm_dst}"""
            )
        )
        exit(1)

    try:
        if os.path.islink(src_file):
            subprocess.call(["cp", "-a", src_file, dst_file])
        else:
            # Ensure the destination file is writable, so that the copying
            # operation would not be fail due to this.
            if os.path.exists(dst_file):
                st = os.stat(dst_file)
                os.chmod(dst_file, st.st_mode | stat.S_IWUSR)
            shutil.copy(src_file, dst_file)
    except Exception as exc:
        log.error(
            inspect.cleandoc(
                f"""Failed to copy_file:
                                   {src_file} =>
                                   {dst_file}
                                   Exception: {exc}
                                   rm_dst: {rm_dst}"""
            )
        )
        exit(1)

    try:
        # Update the file access and modify time to the current time.
        os.utime(dst_file, follow_symlinks=False)
    except Exception:
        try:
            # If follow_symlinks parameter is not supported, fall back to
            # default behavior
            os.utime(dst_file)
        except Exception as exc:
            log.error(
                inspect.cleandoc(
                    f"""Failed to copy_file:
                                       {src_file} =>
                                       {dst_file}
                                       Exception: {exc}
                                       rm_dst: {rm_dst}"""
                )
            )
            exit(1)


def copy(src: str, dst: str, rm_dst=False) -> None:
    if os.path.exists(src):
        # Perform copying.
        if os.path.isdir(src):
            copy_tree(src, dst, rm_dst)
        else:
            copy_file(src, dst, rm_dst)
    else:
        raise Exception(f"{src} does not exist.")
