from conans import ConanFile, AutoToolsBuildEnvironment


class HelloConan(ConanFile):
    name = "termio"
    version = "0.1"
    settings = ("os", "compiler", "build_type", "arch")
    generators = "cmake"
    exports_sources = "*"

    def build(self):
        env_build = AutoToolsBuildEnvironment(self)
        env_build.make()

    def package(self):
        self.copy("*.h", dst="include")
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        # self.cpp_info.libs = ["hello"]
        pass
