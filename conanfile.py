from conan import ConanFile
from conan.tools.meson import MesonToolchain
from conan.tools.meson import Meson
from conan.tools.gnu import PkgConfigDeps

class TriglavEngine(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    requires = (
        "cgal/5.6",
        "entt/3.13.0",
        "freetype/2.13.2",
        "fmt/10.2.1",
        "gtest/1.14.0",
        "glm/0.9.9.8",
        "rapidyaml/0.5.0",
        "spdlog/1.13.0"
    )
    # We should use system vulkan

    def generate(self):
        pc = PkgConfigDeps(self)
        pc.generate()

        backend = 'ninja'
        if self.settings.os == "Windows":
            backend = 'vs2022'

        tc = MesonToolchain(self, backend=backend)
        tc.generate()

    def build(self):
        meson = Meson(self)
        meson.configure()
        meson.build()
