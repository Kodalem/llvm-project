# This is a helper file required due to LLVM lit tests not supporting the external commands anymore
# and everything needs to use internal commands, basically all because of `execute_external=False`

# Basically it ensures that llvm.memcpy/llvm.memset bounded tests without duplicating the .ll
# files too much, which was of previous external shell command behaviour by the use of `sed` within
# the `lit.local.cfg`, where , it reads the parameters from environment variables set explicitly on each RUN line
# and writes the fully substituted `.ll` into the build output directory

import os
import sys

if len(sys.argv) != 5:
    print("usage: generate_intrinsic_test.py <kind> <template> <output> <type>")
    sys.exit(1)

kind = sys.argv[1]
template_path = sys.argv[2]
output_path = sys.argv[3]
length_type = sys.argv[4]


def require_env(name):
    value = os.environ.get(name)
    if value is None:
        print("Missing environment variable: " + name)
        sys.exit(1)
    return value


with open(template_path) as template_file:
    contents = template_file.read()

contents = contents.replace("<type>", length_type)

if kind == "memset":
    replacements = {
        "<count>": require_env("MEMSET_COUNT"),
        "<alloc_count>": require_env("MEMSET_ALLOC_COUNT"),
        "<ptr_inc>": require_env("MEMSET_PTR_INC"),
        "<ptr_attr>": os.environ.get("MEMSET_PTR_ATTR", ""),
    }
elif kind == "memcpy":
    replacements = {
        "<count>": require_env("MEMCPY_COUNT"),
        "<alloc_count>": require_env("MEMCPY_ALLOC_COUNT"),
        "<dest_ptr_inc>": require_env("MEMCPY_DEST_PTR_INC"),
        "<dest_ptr_attr>": os.environ.get("MEMCPY_DEST_PTR_ATTR", ""),
        "<src_ptr_inc>": require_env("MEMCPY_SRC_PTR_INC"),
        "<src_ptr_attr>": os.environ.get("MEMCPY_SRC_PTR_ATTR", ""),
    }
else:
    print("Unknown intrinsic test kind: " + kind)
    sys.exit(1)

for placeholder, value in replacements.items():
    contents = contents.replace(placeholder, value)

os.makedirs(os.path.dirname(output_path), exist_ok=True)
with open(output_path, "w") as output_file:
    output_file.write(contents)
