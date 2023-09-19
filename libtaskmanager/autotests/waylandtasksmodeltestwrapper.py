#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

import os
import subprocess
import sys
import time

if __name__ == '__main__':
    assert len(sys.argv) >= 2, f"Missing waylandtasksmodeltest argument {len(sys.argv)}"
    test_executable_path: str = sys.argv.pop()

    environ = os.environ.copy()
    environ["KWIN_WAYLAND_NO_PERMISSION_CHECKS"] = "1"
    kwin_process = subprocess.Popen(["kwin_wayland", "--no-lockscreen", "--no-global-shortcuts", "--no-kactivities"], env = environ)

    time.sleep(10)
    assert kwin_process.poll() is None

    test_environ = os.environ.copy()
    test_environ["QT_QPA_PLATFORM"] = "wayland"
    test_process = subprocess.Popen([test_executable_path], env = test_environ)
    assert test_process.poll() is None

    result: int = 1
    try:
        result = test_process.wait(timeout=60)
    except subprocess.TimeoutExpired as e:
        print("Timeout", e)
    finally:
        kwin_process.terminate()

    sys.exit(result)
