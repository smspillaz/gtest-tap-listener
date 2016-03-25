from conans import ConanFile
import os
from conans.tools import download
from conans.tools import unzip


class MochaGTestTapListenerConan(ConanFile):
    name = "gtest-tap-listener-mocha"
    version = "master"
    ZIP_FOLDER_NAME = "gtest-tap-listener-mocha-%s" % version
    generators = "cmake"
    settings = "os", "arch", "compiler", "build_type"
    options = {}
    default_options = ""
    url = "http://github.com/smspillaz/conan-gtest-tap-listener-mocha"
    license = "https://github.com/smspillaz/gtest-tap-listener-mocha/master/COPYING"

    def config(self):
        try:  # Try catch can be removed when conan 0.8 is released
            del self.settings.compiler.libcxx
        except:
            pass

    def source(self):
        zip_name = "gtest-%s.zip" % self.version
        url = "https://github.com/smspillaz/gtest-tap-listener-mocha/archive/master.zip"
        download(url, zip_name)
        unzip(zip_name)
        os.unlink(zip_name)

    def build(self):
        pass

    def package(self):
        # Copying headers
        self.copy(pattern="*.h",
                  dst="include/gtest-tap-listener-mocha",
                  src="%s/src" % self.ZIP_FOLDER_NAME,
                  keep_path=True)
