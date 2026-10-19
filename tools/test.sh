#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
"${CXX:-g++}" -std=c++11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined -fno-sanitize-recover=all -g -Ishared tests/test_protocol.cpp -o build/test_protocol
./build/test_protocol
"${CXX:-g++}" -std=c++11 -Wall -Wextra -Werror -Wformat=2 -pedantic -fsanitize=undefined -fno-sanitize-recover=all -g -Ishared tests/test_features.cpp -o build/test_features
./build/test_features > build/test-features.csv
python3 -m unittest discover -s tests -p 'test_*.py' -v
