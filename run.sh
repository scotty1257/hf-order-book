#!/usr/bin/env bash
set -euo pipefail

readonly program_name=${0##*/}

verbose=0
config_file=""

usage() {
	cat <<EOF
Usage: $program_name [-h] [-v] [-c FILE] <command> [args...]

Commands:
  build   Build the project
  test    Run the tests (extra args are passed through as filters)
  clean   Remove build artifacts
  help    Show this help

Options:
  -h, --help          Show this help
  -v, --verbose       Verbose output
  -c, --config FILE   Read configuration from FILE

Run '$program_name <command> --help' for command-specific help.
EOF
}

die() {
	printf '%s: %s\n' "$program_name" "$*" >&2
	exit 2
}

require_value() {
	(($# >= 2)) && [[ -n $2 ]] || die "$1 requires a value"
}

log_verbose() {
	if ((verbose)); then
		printf '%s\n' "$*" >&2
	fi
}

command_build() {
	while (($#)); do
		case $1 in
		-h | --help)
			printf 'Usage: %s build\n' "$program_name"
			return 0
			;;
		*) die "build: unexpected argument: $1" ;;
		esac
	done
	log_verbose "build: config=${config_file:-<default>}"
	echo "building"
}

command_test() {
	local test_filters=()
	while (($#)); do
		case $1 in
		-h | --help)
			printf 'Usage: %s test [FILTER...]\n' "$program_name"
			return 0
			;;
		--)
			shift
			test_filters+=("$@")
			break
			;;
		-?*) die "test: unknown option: $1" ;;
		*) test_filters+=("$1") ;;
		esac
		shift
	done
	log_verbose "test: config=${config_file:-<default>} filters=${#test_filters[@]}"
	echo "testing ${test_filters[*]+${test_filters[*]}}"
}

command_clean() {
	while (($#)); do
		case $1 in
		-h | --help)
			printf 'Usage: %s clean\n' "$program_name"
			return 0
			;;
		*) die "clean: unexpected argument: $1" ;;
		esac
	done
	log_verbose "clean: config=${config_file:-<default>}"
	echo "cleaning"
}

main() {
	while (($#)); do
		case $1 in
		-h | --help)
			usage
			exit 0
			;;
		-v | --verbose) verbose=1 ;;
		-c | --config)
			require_value "$@"
			config_file=$2
			shift
			;;
		--config=*)
			config_file=${1#*=}
			[[ -n $config_file ]] || die "--config requires a value"
			;;
		--)
			shift
			break
			;;
		-?*) die "unknown option: $1 (see --help)" ;;
		*) break ;;
		esac
		shift
	done

	if (($# == 0)); then
		usage >&2
		exit 2
	fi

	if [[ -n $config_file && ! -r $config_file ]]; then
		die "config file not readable: $config_file"
	fi

	local command_name=$1
	shift
	case $command_name in
	build) command_build "$@" ;;
	test) command_test "$@" ;;
	clean) command_clean "$@" ;;
	help) usage ;;
	*) die "unknown command: $command_name (see --help)" ;;
	esac
}

main "$@"
