from conans import ConanFile, AutoToolsBuildEnvironment


class TermioConan(ConanFile):
    name = "termio"
    description = "Double-buffering terminal UI library (light-weight ncurses alternative)"
    version = "1.0.0"
    license = "MIT"
    url = "https://github.com/tomsmeding/termio"
    settings = ("os", "compiler", "build_type", "arch")
    exports_sources = "*"

    def build(self):
        env_build = AutoToolsBuildEnvironment(self)
        env_build.make()

    def package(self):
        self.copy("*.h", dst="include")
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["termio"]
