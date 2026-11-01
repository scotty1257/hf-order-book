// fn configure_script() {
//   todo
// }

// fn pprint(msg: String, color: String) {
//   let c = case color {
//     "black" -> ansi.bg_black
//     "blue" -> ansi.blue
//     "green" -> ansi.green
//     "yellow" -> ansi.yellow
//     "red" -> ansi.red
//     "white" -> ansi.bright_white
//     _ -> ansi.bright_magenta
//   }

//   msg
//   |> c
//   |> io.print
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
//   pprint(">> Usage:", "green")
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

// main

// let args: List(String) = argv.load().arguments

// rprocess_arg(args)

// Nil
