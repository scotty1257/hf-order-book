#!/usr/bin/env bash
#
# gen_docs.sh
#
# Builds HTML documentation for a C++ project using Doxygen + Sphinx + Breathe.
# Installs uv and Doxygen if they are missing, and creates the Doxyfile,
# docs/conf.py and docs/index.rst on first run. Existing files are never
# overwritten, so you can edit them freely afterwards.
#
# Usage:
#   ./gen_docs.sh            Build the docs
#   ./gen_docs.sh --open     Build the docs and open them in a browser
#   ./gen_docs.sh --clean    Delete build output, then build
#
# Settings (override with environment variables):
#   PROJECT_NAME       Title shown in the docs     (default: folder name)
#   SOURCE_DIRECTORY   Folder containing headers   (default: include)

set -euo pipefail

script_directory="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$script_directory"

project_name="${PROJECT_NAME:-$(basename "$script_directory")}"
source_directory="${SOURCE_DIRECTORY:-include}"
docs_directory="docs"
build_directory="build"
doxygen_output_directory="$build_directory/doxygen"
html_output_directory="$build_directory/html"

should_open_browser="false"
should_clean="false"

for argument in "$@"; do
	case "$argument" in
	--open) should_open_browser="true" ;;
	--clean) should_clean="true" ;;
	-h | --help)
		sed -n '3,17p' "$0" | sed 's/^# \{0,1\}//'
		exit 0
		;;
	*)
		echo "Unknown option: $argument" >&2
		exit 1
		;;
	esac
done

print_step() {
	echo
	echo "==> $1"
}

command_exists() {
	command -v "$1" >/dev/null 2>&1
}

is_windows_shell() {
	case "$(uname -s)" in
	MINGW* | MSYS* | CYGWIN*) return 0 ;;
	*) return 1 ;;
	esac
}

ensure_uv() {
	if command_exists uv; then
		return
	fi

	print_step "uv not found, installing"
	if is_windows_shell; then
		powershell -ExecutionPolicy ByPass -NoProfile -Command "irm https://astral.sh/uv/install.ps1 | iex"
	else
		curl -LsSf https://astral.sh/uv/install.sh | sh
	fi
	export PATH="$HOME/.local/bin:$HOME/.cargo/bin:$PATH"

	if ! command_exists uv; then
		echo "uv was installed but is not on PATH. Open a new terminal and run this script again." >&2
		exit 1
	fi
}

ensure_doxygen() {
	if command_exists doxygen; then
		return
	fi

	# Windows: the installer puts Doxygen here but the current shell won't see it yet.
	local windows_doxygen_directory="/c/Program Files/doxygen/bin"
	if is_windows_shell && [ -x "$windows_doxygen_directory/doxygen.exe" ]; then
		export PATH="$windows_doxygen_directory:$PATH"
		return
	fi

	print_step "Doxygen not found, installing"
	if is_windows_shell && command_exists winget; then
		winget install --id DimitriVanHeesch.Doxygen --exact --accept-source-agreements --accept-package-agreements
		export PATH="$windows_doxygen_directory:$PATH"
	elif command_exists brew; then
		brew install doxygen
	elif command_exists apt-get; then
		sudo apt-get update
		sudo apt-get install -y doxygen
	elif command_exists dnf; then
		sudo dnf install -y doxygen
	elif command_exists pacman; then
		sudo pacman -S --noconfirm doxygen
	else
		echo "Could not install Doxygen automatically. Install it from https://www.doxygen.nl/download.html" >&2
		exit 1
	fi

	if ! command_exists doxygen; then
		echo "Doxygen was installed but is not on PATH. Open a new terminal and run this script again." >&2
		exit 1
	fi
}

ensure_source_directory() {
	if [ ! -d "$source_directory" ]; then
		echo "Source folder '$source_directory' not found." >&2
		echo "Create it, or run with SOURCE_DIRECTORY=your_folder ./gen_docs.sh" >&2
		exit 1
	fi
}

ensure_doxyfile() {
	if [ -f Doxyfile ]; then
		return
	fi

	print_step "Creating Doxyfile"
	cat >Doxyfile <<EOF
# Only the settings that matter for Breathe. Doxygen uses defaults for the rest.
PROJECT_NAME           = "$project_name"
INPUT                  = $source_directory
RECURSIVE              = YES
FILE_PATTERNS          = *.h *.hh *.hpp *.hxx *.cpp *.cc *.cxx
EXTRACT_ALL            = YES
OUTPUT_DIRECTORY       = $doxygen_output_directory
GENERATE_XML           = YES
GENERATE_HTML          = NO
GENERATE_LATEX         = NO
QUIET                  = YES
EOF
}

ensure_sphinx_files() {
	mkdir -p "$docs_directory"

	if [ ! -f "$docs_directory/conf.py" ]; then
		print_step "Creating $docs_directory/conf.py"
		cat >"$docs_directory/conf.py" <<EOF
project = "$project_name"
extensions = ["breathe"]

breathe_projects = {"project": "../$doxygen_output_directory/xml"}
breathe_default_project = "project"
breathe_default_members = ("members", "undoc-members")

html_theme = "furo"
html_title = "$project_name"
EOF
	fi

	if [ ! -f "$docs_directory/index.rst" ]; then
		print_step "Creating $docs_directory/index.rst"
		local title_underline
		title_underline="$(printf '%*s' "${#project_name}" '' | tr ' ' '=')"
		cat >"$docs_directory/index.rst" <<EOF
$project_name
$title_underline

..
   Replace doxygenindex with specific directives once you want control
   over the layout, for example:

   .. doxygenstruct:: LimitOrder
      :members:

.. doxygenindex::
EOF
	fi
}

run_doxygen() {
	print_step "Running Doxygen"
	rm -rf "$doxygen_output_directory"
	doxygen Doxyfile

	if [ ! -f "$doxygen_output_directory/xml/index.xml" ]; then
		echo "Doxygen did not produce $doxygen_output_directory/xml/index.xml. Check the Doxyfile." >&2
		exit 1
	fi
}

run_sphinx() {
	print_step "Running Sphinx"
	uv run --no-project --quiet \
		--with sphinx \
		--with breathe \
		--with furo \
		sphinx-build -b html "$docs_directory" "$html_output_directory"
}

open_in_browser() {
	local index_page="$html_output_directory/index.html"
	if is_windows_shell; then
		start "" "$index_page"
	elif command_exists open; then
		open "$index_page"
	elif command_exists xdg-open; then
		xdg-open "$index_page" >/dev/null 2>&1 &
	else
		echo "Open $index_page in your browser."
	fi
}

if [ "$should_clean" = "true" ]; then
	print_step "Cleaning $build_directory"
	rm -rf "$build_directory"
fi

ensure_uv
ensure_doxygen
ensure_source_directory
ensure_doxyfile
ensure_sphinx_files
run_doxygen
run_sphinx

print_step "Done: $script_directory/$html_output_directory/index.html"

if [ "$should_open_browser" = "true" ]; then
	open_in_browser
fi
