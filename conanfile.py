from conan import ConanFile
from conan.tools.meson import MesonToolchain
from conan.tools.meson import Meson
from conan.tools.gnu import PkgConfigDeps

class TriglavEngine(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    requires = (
        "cgal/6.0.1",
        "entt/3.13.0",
        "freetype/2.13.2",
        "fmt/10.2.1",
        "gtest/1.14.0",
        "glm/0.9.9.8",
        "rapidyaml/0.5.0",
        "spdlog/1.13.0",
        "rapidjson/1.1.0",
        "ktx/4.3.2"
    )
    options = {
        "address_sanitizer": [False, True],
        "disable_debug_utils": [False, True],
        "wayland": [False, True]
    }
    default_options = {
        "address_sanitizer": False,
        "disable_debug_utils": False,
        "wayland": False,
    }
    # We should use system vulkan

    def configure(self):
        self.options["boost"].without_test = True

    def generate(self):
        pc = PkgConfigDeps(self)
        pc.generate()

        backend = 'ninja'
        if self.settings.os == "Windows" and self.settings.compiler == "msvc":
            backend = 'vs2022'

        tc = MesonToolchain(self, backend=backend)
        tc.project_options["address_sanitizer"] = "enabled" if self.options["address_sanitizer"] else "disabled"
        tc.project_options["disable_debug_utils"] = "enabled" if self.options["disable_debug_utils"] else "disabled"
        tc.project_options["wayland"] = "enabled" if self.options["wayland"] else "disabled"
        tc.generate()

    def build(self):
        meson = Meson(self)
        meson.configure()
        meson.build()
