pub type ProjectType {
  Cpp
  Node
  Python
}

pub type PackageCommand {
  Add
  Install
  Clean
}

pub fn cpp(pc: PackageCommand) {
  case pc {
    Add -> "conan add"
    Install -> "conan install"
    Clean -> "conan cache clean"
  }
}

pub fn node(pc: PackageCommand) {
  case pc {
    Add -> "bun add"
    Install -> "bun install"
    Clean -> "bun pm cache rm"
  }
}

pub fn python(pc: PackageCommand) {
  case pc {
    Add -> "uv add"
    Install -> "uv sync"
    Clean -> "uv cache clean"
  }
}

pub type PackageManager {
  PackageManager(add: String, install: String, clean: String)
}

pub fn manager(pm: ProjectType) -> PackageManager {
  case pm {
    Cpp -> PackageManager(cpp(Add), cpp(Install), cpp(Clean))
    Node -> PackageManager(node(Add), node(Install), node(Clean))
    Python -> PackageManager(python(Add), python(Install), python(Clean))
  }
}
// pub fn manager_2(pm: ProjectType) -> PackageManager {
//   case pm {
//     Cpp -> PackageManager(cpp(Add), cpp(Install), cpp(Clean))
//     Node -> PackageManager(node(Add), node(Install), node(Clean))
//     Python -> PackageManager(python(Add), python(Install), python(Clean))
//   }
// }
// pub fn add_command(pm: ProjectType) {
//   case pm {
//     Cpp -> result.unwrap(manager(pm), PackageManager("", "", ""))
//     Node -> result.unwrap(manager(pm), PackageManager("", "", ""))
//     Python -> result.unwrap(manager(pm), PackageManager("", "", ""))
//   }
// }
// pub fn manager(pm: PackageManager) {
//   case ppm {
//     Cpp ->
//       Cpp(
//         add: "conan ...",
//         install: "conan install . --output-folder build --build=missing",
//         clean: "conan cache clean",
//       )
//     _ -> Nil
//   }
// }
// pub fn manager(pm: PackageManager) -> String {
//   case pm {
//     Cpp -> "conan"
//     Node -> "bun"
//     Python -> "uv pip"
//   }
// }
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
