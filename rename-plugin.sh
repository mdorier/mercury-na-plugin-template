#!/usr/bin/env bash
#
# Rename the template plugin from "abc" to a user-chosen name.
#
# Usage:  ./rename-plugin.sh <name>
#
# Example:
#   ./rename-plugin.sh xyz
#
# This will rename na_abc.c to na_xyz.c, rename test_abc_* to test_xyz_*,
# and replace every occurrence of "abc" (in the plugin context) with "xyz"
# in CMakeLists.txt, source, and test files.

set -euo pipefail

if [ $# -ne 1 ]; then
    echo "Usage: $0 <plugin-name>" >&2
    exit 1
fi

name="$1"

# Sanity-check: the name should be a valid C identifier fragment (lowercase
# alphanumeric + underscores, must not start with a digit).
if ! [[ "$name" =~ ^[a-z_][a-z0-9_]*$ ]]; then
    echo "Error: plugin name must match [a-z_][a-z0-9_]* (got \"$name\")" >&2
    exit 1
fi

cd "$(dirname "$0")"

# Detect the current plugin name by looking for na_*.c (excluding any build/).
old_src=$(find . -maxdepth 1 -name 'na_*.c' | head -1)
if [ -z "$old_src" ]; then
    echo "Error: could not find na_*.c source file in $(pwd)" >&2
    exit 1
fi

old_name="${old_src#./na_}"   # strip leading ./na_
old_name="${old_name%.c}"     # strip trailing .c

if [ "$old_name" = "$name" ]; then
    echo "Plugin is already named \"$name\", nothing to do."
    exit 0
fi

echo "Renaming plugin: $old_name -> $name"

# 1. Replace inside top-level file contents
for f in CMakeLists.txt "na_${old_name}.c"; do
    if [ -f "$f" ]; then
        sed -i "s/${old_name}/${name}/g" "$f"
        echo "  updated $f"
    fi
done

# 2. Rename the source file
if [ -f "na_${old_name}.c" ]; then
    mv "na_${old_name}.c" "na_${name}.c"
    echo "  renamed na_${old_name}.c -> na_${name}.c"
fi

# 3. Replace inside test file contents
if [ -d test ]; then
    for f in test/CMakeLists.txt test/test_${old_name}_*.c; do
        if [ -f "$f" ]; then
            sed -i "s/${old_name}/${name}/g" "$f"
            echo "  updated $f"
        fi
    done

    # 4. Rename test source files
    for f in test/test_${old_name}_*.c; do
        if [ -f "$f" ]; then
            new_f="${f/test_${old_name}_/test_${name}_}"
            mv "$f" "$new_f"
            echo "  renamed $(basename "$f") -> $(basename "$new_f")"
        fi
    done
fi

echo "Done. Remember to remove any old build directory before rebuilding."
