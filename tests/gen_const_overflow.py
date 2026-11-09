#!/usr/bin/env python3
"""Generate VekLang programs that exercise the bytecode format's size limits.

The compiler used to store every constant index in a single byte and every
jump target in two bytes, truncating both silently. Any program past those
limits was corrupted at compile time: the VM executed the wrong constant or
jumped to garbage, with no error or crash. The operands are now checked and
constant indices are 16 bits wide.

    python3 tests/gen_const_overflow.py consts 300   # 300 constants, prints s299
    python3 tests/gen_const_overflow.py consts 70000 # exceeds the 16-bit pool
    python3 tests/gen_const_overflow.py jump 20000   # loop body > 64KB of code
"""
import sys

mode = sys.argv[1] if len(sys.argv) > 1 else "consts"
n = int(sys.argv[2]) if len(sys.argv) > 2 else 300

if mode == "consts":
    # Each 'let' puts a string literal and a variable name in the constant pool.
    print("func main() int {")
    for i in range(n):
        print(f'    let str s{i} = "S{i}";')
    print(f"    print(s{n - 1});")
    print("    ret 0;")
    print("}")
elif mode == "jump":
    # A single loop whose body is so large that the loop's exit jump target
    # falls outside the 16-bit address space. Uses statements that produce
    # several bytes of code per constant so the code size limit is hit before
    # the constant pool limit.
    print("func main() int {")
    print("    let int total = 0;")
    print("    for (let int i = 0; i < 1; i += 1) {")
    for _ in range(n):
        print("        total = total + 1;")
    print("    }")
    print("    print(total);")
    print("    ret 0;")
    print("}")
else:
    sys.exit(f"unknown mode: {mode}")
