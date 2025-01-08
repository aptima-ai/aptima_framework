#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os

from conans import CMake, ConanFile


class TestPackageConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake_find_package_multi", "cmake"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        assert os.path.isfile(os.path.join(
            self.deps_cpp_info["catch2"].rootpath, "licenses", "LICENSE.txt"))
        bin_path = os.path.join("bin", "test_package")
        self.run("%s -s" % bin_path, run_environment=True)
