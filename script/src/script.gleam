import argv
import gleam/io
import gleam/list
import gleam/string
import gleam_community/ansi
import glint
import snag

/// a boolean flag with default False to control message capitalization.
///
pub fn caps_flag() -> glint.Flag(Bool) {
  glint.bool_flag("caps")
  |> glint.flag_default(False)
  |> glint.flag_help("Capitalize the hello message")
}

/// an int flag with default 1 to control how many times to repeat the message.
/// this flag is constrained to values greater than 0.
///
pub fn repeat_flag() -> glint.Flag(Int) {
  use n <- glint.flag_constraint(
    glint.int_flag("repeat")
    |> glint.flag_default(1)
    |> glint.flag_help("Repeat the message n-times"),
  )
  case n {
    _ if n > 0 -> Ok(n)
    _ -> snag.error("Value must be greater than 0.")
  }
}

pub fn app() {
  glint.new()
  |> glint.with_name("glow")
  |> glint.pretty_help(glint.default_pretty_help())
  |> glint.group_flag([], caps_flag())
  |> glint.group_flag([], repeat_flag())
}

pub fn main() -> Nil {
  glint.run_and_handle(app(), argv.load().arguments, io.println)
}
