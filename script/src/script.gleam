// import argv
// import gleam/int
// import gleam/io
// import gleam/list
// import gleam/string
// import gleam_community/ansi
// import glint
// import glint/constraint
// import shellout
// import snag

// /// ### Call External Command
// /// Call an external command with familiar syntax
// /// 
// /// | Argument | Type | Description |
// /// | :--- | :---: | :--- |
// /// | **cmd** | `String` | *Name of program to execute* |
// /// | **args** | `List(String)` | *Program arguments* |
// /// | **dir** | `String` | *Execution directory* |
// pub fn call_external_command(cmd: String, args: List(String), dir: String) {
//   case shellout.command(run: cmd, with: args, in: dir, opt: []) {
//     Ok(output) -> io.println(output)
//     Error(#(exit_code, message)) ->
//       io.println_error(
//         cmd <> "failed (" <> int.to_string(exit_code) <> "): " <> message,
//       )
//   }
// }

// fn configure_script() {
//   todo
// }

// // fn printfmt(fmt: String) {
// //   todo
// //   // "{blue(fb):(ibus)[This is the msg]};"
// //   // ""
// //   {blue;fb;ibus;Something}
// // }

// fn pprint(msg: String, color: String) {
//   let c = case color {
//     "black" -> ansi.bg_black
//     "blue" -> ansi.blue
//     "green" -> ansi.green
//     "yellow" -> ansi.yellow
//     "red" -> ansi.red
//     "white" -> ansi.bright_white
//     "underline" -> ansi.underline
//     "bold" -> ansi.bold
//     "italic" -> ansi.bold
//     _ -> ansi.bright_magenta
//   }

//   msg
//   |> c
//   |> io.println
// }

// fn help_text() {
//   let t = [
//     "-c, --config",
//     "-h, --help",
//     "-b, --build",
//     "-r, --run",
//     "-f, --format",
//   ]

//   let s =
//     list.map(t, fn(q) { q |> string.append(",") |> string.pad_end(15, " ") })

//   list.each(s, fn(q) { pprint(q <> "\n", _) })
// }

// fn usage_text() {
//   pprint(">> No arguments provided!\n\n", "white")
//   pprint("Usage:", "green")
//   pprint(" glow ", "blue")
//   pprint("<COMMAND>\n", "yellow")
// }

// fn strip_hyphen(s) {
//   case s {
//     "-" <> rest -> strip_hyphen(rest)
//     _ -> s
//   }
// }

// fn clean_argument(arg: String) -> String {
//   arg
//   |> string.lowercase
//   |> string.trim
//   |> strip_hyphen
// }

// fn rprocess_arg(args) {
//   case args {
//     [] -> usage_text()
//     [first, ..rest] -> {
//       let first = clean_argument(first)

//       // io.print(first <> " ")
//       io.println("First argument: " <> first)
//       case first {
//         "h" | "help" -> help_text()
//         _ -> Nil
//       }
//       rprocess_arg(rest)
//     }
//   }
// }

// pub fn main() -> Nil {
//   let args: List(String) = argv.load().arguments

//   rprocess_arg(args)

//   pprint("Underline", "underline")
//   Nil
// }
import argv
import gleam/bool
import gleam/int
import gleam/io
import gleam/list
import gleam/result
import gleam/string
import gleam_community/ansi
import glint
import glint/constraint
import package
import package
import shellout
import snag

/// Call an external process using its PATH name
pub fn call_external_command(cmd: String, args: List(String), dir: String) {
  case shellout.command(run: cmd, with: args, in: dir, opt: []) {
    Ok(output) -> io.println(output)
    Error(#(exit_code, message)) ->
      io.println_error(
        cmd <> "failed (" <> int.to_string(exit_code) <> "): " <> message,
      )
  }
}

pub fn config_cmd() {
  use <- glint.command_help("Configure a new project.")

  use get_project_type <- glint.flag(
    glint.string_flag("type")
    |> glint.flag_help("(required) Project Type: <cpp|node|py>")
    |> glint.flag_constraint(fn(t) {
      case list.contains(["cpp", "node", "py"], t) {
        True -> Ok(t)
        False -> snag.error("Invalid project type")
      }
    }),
  )

  use get_is_debug <- glint.flag(
    glint.bool_flag("debug")
    |> glint.flag_help("(optional) Set project to debug mode.")
    |> glint.flag_default(True),
  )

  use get_version <- glint.flag(
    glint.string_flag("version")
    |> glint.flag_help("(optional) Project configuration version")
    |> glint.flag_default("1.0.0"),
  )

  use _named, _args, flags <- glint.command()

  let project_type: Result(String, snag.Snag) = get_project_type(flags)
  let is_debug: Result(Bool, snag.Snag) = get_is_debug(flags)
  let project_version: Result(String, snag.Snag) = get_version(flags)

  case project_type {
    Ok("cpp") ->
      call_external_command(
        "echo",
        [
          "C++ Configuration [Debug: "
          <> bool.to_string(result.is_ok(is_debug))
          <> ", "
          <> "Version: "
          <> result.unwrap(project_version, "none")
          <> "]",
        ],
        ".",
      )
    Ok("node") -> call_external_command("echo", ["Node Configuration"], ".")
    Ok("py") -> call_external_command("echo", ["Python Configuration"], ".")
    Error(err) -> io.print("YOINK" <> err.issue)
    _ -> call_external_command("echo", ["Testing this echo command"], ".")
  }
}

pub fn add_pkg_cmd() {
  use <- glint.command_help("Configure a new project.")

  use _, _args, _flags <- glint.command()

  call_external_command("", [], ".")
}

pub fn test_cmd() {
  use <- glint.command_help("Run tests.")

  use get_run_all <- glint.flag(
    glint.bool_flag("all")
    |> glint.flag_default(True)
    |> glint.flag_help("Run all tests in project"),
  )

  use _named, _args, flags <- glint.command()

  let assert Ok(run_all) = get_run_all(flags)

  case run_all {
    True -> call_external_command("echo", ["Testing this echo command"], ".")
    False -> call_external_command("echo", ["Testing this echo command"], ".")
  }
}

pub fn run_cmd() {
  use <- glint.command_help("Run the entrypoint application.")

  use _, _args, _flags <- glint.command()
  call_external_command("", [], ".")
}

pub fn build_cmd() {
  use <- glint.command_help("Build all projects.")

  use _, _args, _flags <- glint.command()
  call_external_command("", [], ".")
}

pub fn clean_cmd() {
  use <- glint.command_help("Clean build output directory.")

  use _, _args, _flags <- glint.command()
  call_external_command("", [], ".")
}

pub fn app() {
  glint.new()
  |> glint.with_name("glow")
  |> glint.global_help("Development project manager for calling all commmands")
  |> glint.pretty_help(glint.default_pretty_help())
  |> glint.add(at: ["config"], do: config_cmd())
  |> glint.add(at: ["build"], do: build_cmd())
  |> glint.add(at: ["test"], do: test_cmd())
  |> glint.add(at: ["run"], do: run_cmd())
  |> glint.add(at: ["clean"], do: clean_cmd())
  |> glint.add(at: ["add"], do: add_pkg_cmd())
}

pub fn main() -> Nil {
  glint.run(app(), argv.load().arguments)

  list.each([1, 2, 3], fn(a) { io.print(int.to_string(a)) })

  echo "pkg" <> package.manager(package.Cpp)

  Nil
}
