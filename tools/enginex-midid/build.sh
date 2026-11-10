#!/bin/sh
set -eu
cd "$(dirname "$0")"

# Native build (for testing and x86_64 simulation)
gcc -std=c11 -O2 -Wall -Wextra -Werror enginex_midid.c -lm -o enginex_midid

# Run test suite
gcc -std=c11 -O2 -Wall -Wextra -Werror -DTESTING test_enginex_midid.c -lm -o test_enginex_midid
./test_enginex_midid

echo "enginex_midid: build & tests completed successfully"
