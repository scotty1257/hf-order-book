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

// ///
// /// @Documentation
// pub fn call_external_command(cmd: String, args: List(String), dir: String) {
//   case shellout.command(run: cmd, with: args, in: dir, opt: []) {
//     Ok(output) -> io.println(output)
//     Error(#(exit_code, message)) ->
//       io.println_error(
//         cmd <> "failed (" <> int.to_string(exit_code) <> "): " <> message,
//       )
//   }
// }

// pub fn config_cmd() {
//   use <- glint.command_help("Configure a new project.")
//   use get_project_type <- glint.flag(
//     glint.string_flag("type")
//     |> glint.flag_help("(required) Project Type: <cpp|node|py>")
//     |> glint.flag_constraint(fn(t) {
//       case list.contains(["cpp", "node", "py"], t) {
//         True -> Ok(t)
//         False -> snag.error("Invalid project type")
//       }
//     }),
//   )

//   use get_is_debug <- glint.flag(
//     glint.bool_flag("debug")
//     |> glint.flag_help("(optional) Set project to debug mode.")
//     |> glint.flag_default(True),
//   )

//   use get_version <- glint.flag(
//     glint.string_flag("version")
//     |> glint.flag_help("(optional) Project configuration version")
//     |> glint.flag_default("1.0.0"),
//   )

//   use _named, _args, flags <- glint.command()

//   // let project_type = case get_project_type(flags) {
//   //   Ok(flags) -> flags
//   //   Error(_flags) -> {
//   //     io.println("Get project type error")
//   //     ""
//   //   }
//   // }

//   let project_type: Result(String, snag.Snag) = get_project_type(flags)
//   let is_debug: Result(Bool, snag.Snag) = get_is_debug(flags)
//   let project_version: Result(String, snag.Snag) = get_version(flags)

//   case project_type {
//     Ok("cpp") ->
//       call_external_command("echo", ["Testing this echo command"], ".")
//     Ok("node") ->
//       call_external_command("echo", ["Testing this echo command"], ".")
//     Ok("py") ->
//       call_external_command("echo", ["Testing this echo command"], ".")
//     Error(err) -> io.print(err.issue)
//     _ -> call_external_command("echo", ["Testing this echo command"], ".")
//   }
// }

// pub fn add_pkg_cmd() {
//   use <- glint.command_help("Configure a new project.")

//   use _, args, flags <- glint.command()

//   call_external_command("", [], ".")
// }

// pub fn test_cmd() {
//   use <- glint.command_help("Run tests.")

//   use get_run_all <- glint.flag(
//     glint.bool_flag("all")
//     |> glint.flag_default(True)
//     |> glint.flag_help("Run all tests in project"),
//   )

//   use _named, _args, flags <- glint.command()

//   let assert Ok(run_all) = get_run_all(flags)

//   case run_all {
//     True -> call_external_command("echo", ["Testing this echo command"], ".")
//     False -> call_external_command("echo", ["Testing this echo command"], ".")
//   }
// }

// pub fn run_cmd() {
//   use <- glint.command_help("Run the entrypoint application.")

//   use _, args, flags <- glint.command()
//   call_external_command("", [], ".")
// }

// pub fn build_cmd() {
//   use <- glint.command_help("Build all projects.")

//   use _, args, flags <- glint.command()
//   call_external_command("", [], ".")
// }

// pub fn clean_cmd() {
//   use <- glint.command_help("Clean build output directory.")

//   use _, args, flags <- glint.command()
//   call_external_command("", [], ".")
// }

// /// a boolean flag with default False to control message capitalization.
// ///
// pub fn caps_flag() -> glint.Flag(Bool) {
//   glint.bool_flag("caps")
//   |> glint.flag_default(False)
//   |> glint.flag_help("Capitalize the hello message")
// }

// /// an int flag with default 1 to control how many times to repeat the message.
// /// this flag is constrained to values greater than 0.
// ///
// pub fn repeat_flag() -> glint.Flag(Int) {
//   use n <- glint.flag_constraint(
//     glint.int_flag("repeat")
//     |> glint.flag_default(1)
//     |> glint.flag_help("Repeat the message n-times"),
//   )
//   case n {
//     _ if n > 0 -> Ok(n)
//     _ -> snag.error("Value must be greater than 0.")
//   }
// }

// pub fn app() {
//   glint.new()
//   |> glint.with_name("glow")
//   |> glint.global_help("Development project manager for calling all commmands")
//   |> glint.pretty_help(glint.default_pretty_help())
//   |> glint.add(at: ["config"], do: config_cmd())
//   |> glint.add(at: ["build"], do: build_cmd())
//   |> glint.add(at: ["test"], do: test_cmd())
//   |> glint.add(at: ["run"], do: run_cmd())
//   |> glint.add(at: ["clean"], do: clean_cmd())
//   |> glint.add(at: ["add"], do: add_pkg_cmd())
// }

// pub fn main() -> Nil {
//   glint.run(app(), argv.load().arguments)
// }
