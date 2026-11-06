# VekLang syntax guide

VekLang is statically typed: declarations name a type explicitly. Programs use `func main() int` as their entry point and return values with `ret`.

## Variables and arrays

Use `let` for variables and `const` for values that must not be reassigned. Supported types include `int`, `int32`, `int64`, `float`, `float32`, `float64`, `str`, `char`, and `bool`.

```cpp
let int count = 3;
const str label = "Vek";
let int scores[] = {90, 45, 85};
scores.push(100);
let int last = scores.pop();
```

Arrays can also declare a fixed initial size with `let int values[3];`. Index an array with square brackets, for example `scores[0]`.

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
