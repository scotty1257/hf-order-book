pub fn cpp_pkg_mgr() -> String {
  // Conan
  ""
}

pub fn node_pkg_mgr() -> String {
  // Conan
  ""
}

pub fn py_pkg_mgr() -> String {
  // Conan
  ""
}

pub fn cfg_pkg_mgr(proj: String) -> fn() -> String {
  case proj {
    "cpp" -> cpp_pkg_mgr
    "node" -> node_pkg_mgr
    "py" -> py_pkg_mgr
    _ -> fn() { "None" }
  }
}

pub type PackageManager {
  Cpp
  Node
  Python
}

pub fn manager(pm: PackageManager) -> String {
  case pm {
    Cpp -> "conan"
    Node -> "bun"
    Python -> "uv pip"
  }
}
// $ conan install . --output-folder build --build=missing
// conan install . --output-folder build --build=missing
// $ cd build
// $ source conanbuild.sh
// Capturing current environment in deactivate_conanbuildenv-release-x86_64.sh
// Configuring environment variables
// $ cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
// $ cmake --build .
// ...
// Building with CMake version: 3.27.9
// ...
// [100%] Built target compressor
//
// $ ./compressor
// Uncompressed size is: 233
// Compressed size is: 147
// ZLIB VERSION: 1.3.1
// $ source deactivate_conanbuild.sh
//
// conan install . --output-folder build --build=missing
// cd build 
// source conanbuild.sh
// cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
// cmake --build .
// source deactivate_conanbuild.sh
