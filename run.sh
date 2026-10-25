#!/usr/bin/env bash

set -euo pipefail

what_is_going_on="$1"

script_directory="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$script_directory"

if [[ "$what_is_going_on" == "config" ]]; then
	echo -e "\n================= CONFIGURE CMAKE [DEBUG] ================="
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
	echo -e ""
elif [[ "$what_is_going_on" == "build" ]]; then
	echo -e "\n================= CMAKE BUILD PROJECT DIR ================="
	cmake --build build/ --parallel 16
	echo -e ""
elif [[ "$what_is_going_on" == "test" ]]; then
	echo -e "\n================= RUN CMAKE & CTEST SUITE ================="
	./build/order_book_test --colour-mode=ansi
	echo -e ""
fi
