# VekLang syntax guide

VekLang is statically typed: declarations name a type explicitly. Programs use `func main() int` as their entry point and return values with `ret`.

## Variables and arrays

Use `let` for variables and `const` for values that must not be reassigned. Supported types include `int`, `int32`, `int64`, `float`, `float32`, `float64`, `str`, `char`, and `bool`.

```cpp
let int count = 3;
let float int_to_float = (float)count;
const str label = "Vek";
let int scores[] = {90, 45, 85};
scores.push(100);
let int last = scores.pop();
```

Arrays can also declare a fixed initial size with `let int values[3];`. Index an array with square brackets, for example `scores[0]`.

`.length()` returns the element count of an array or the character count of a string as an `int`.

```cpp
print(scores.length());   // 4
print("Vek".length());    // 3
```

## Control flow

Use `if`, `else if`, and `else` with braces around each block.

```cpp
if (count > 0) {
    print("positive");
} else {
    print("not positive");
}
```

## Functions and output

Functions declare parameter types before parameter names and put the return type after the parameter list.

```cpp
func double_value(int value) int {
    ret value * 2;
}

func main() int {
    print(double_value(21));
    ret 0;
}
```
