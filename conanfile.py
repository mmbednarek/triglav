from conan import ConanFile
from conan.tools.meson import MesonToolchain
from conan.tools.meson import Meson
from conan.tools.gnu import PkgConfigDeps


class TriglavEngine(ConanFile):
    name = "triglav-engine"
    version = "0.0.1"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "address_sanitizer": [False, True],
        "disable_debug_utils": [False, True],
    }
    default_options = {
        "address_sanitizer": False,
        "disable_debug_utils": False,
    }

    # We should use system vulkan

    def requirements(self):
        self.requires("cgal/6.0.1")
        self.requires("entt/3.15.0")
        self.requires("freetype/2.13.2")
        self.requires("gtest/1.14.0")
        self.requires("glm/0.9.9.8")
        self.requires("rapidyaml/0.5.0")
        self.requires("rapidjson/1.1.0")
        self.requires("ktx/4.3.2")

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
        tc.generate()

    def build(self):
        meson = Meson(self)
        meson.configure()
        meson.build()
