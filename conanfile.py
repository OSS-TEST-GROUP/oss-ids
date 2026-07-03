from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout


class DdsIdsRecipe(ConanFile):
    name = "dds-ids"
    version = "0.1"
    package_type = "application"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "with_tests": [True, False],
    }
    default_options = {
        "with_tests": True,
        "gtest/*:build_gmock": False,
    }

    def requirements(self):
        self.requires("fast-dds/3.2.1")
        self.requires("fast-cdr/2.3.0")
        self.requires("nlohmann_json/3.12.0")
        self.requires("cxxopts/3.3.1")
        self.requires("fmt/12.1.0")
        self.requires("spdlog/1.17.0")
        self.requires("libdds-ids/0.1.0")

        if self.options.with_tests:
            self.requires("gtest/1.14.0")

    def configure(self):
        self.options["spdlog"].external_fmt = True

    def build_requirements(self):
        self.tool_requires("fast-dds-gen/4.2.0")
        self.tool_requires("cmake/4.3.3")

    def validate(self):
        check_min_cppstd(self, 17)

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        toolchain = CMakeToolchain(self)
        toolchain.cache_variables["BUILD_TESTING"] = self.options.with_tests
        toolchain.cache_variables["CMAKE_CXX_STANDARD"] = "17"
        toolchain.cache_variables["CMAKE_CXX_EXTENSIONS"] = "ON"
        toolchain.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

