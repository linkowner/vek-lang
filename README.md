# VekLang

![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)
![Language](https://img.shields.io/badge/Language-C%2B%2B17-purple.svg)
![Type](https://img.shields.io/badge/Type-Bytecode%20VM-success.svg)

**Vek** is a fast, statically-typed, bytecode-compiled programming language with its own custom stack-based Virtual Machine (similar to Java and the JVM). It is designed to be simple, bridging the syntax of C++ and Java with some of the flexibility seen in JavaScript.

![demo](https://github.com/user-attachments/assets/4cc4a84d-66de-484e-90d9-a2a4c8d13456)

### Language Features
- [x] Variables & Math
- [x] If/Else Control Flow
- [x] Functions & Scopes
- [x] Dynamic Arrays (`push`, `pop`, indexing)
- [x] `while` and `for` loops
- [ ] `&&` and `||`
- [ ] Structs / Classes
- [ ] String manipulation methods (`.length()`, `.split()`)
  
**Code that shows capabilities (run with `./main {filename}.vek`)**
```cpp
// functions that have params and return types
func calculate_bonus(int score) int {
    let int bonus = 0;

    // Basic control flow
    if (score > 80) {
        bonus = 100;
    } else if (score > 50) {
        bonus = 50;
    } else {
        bonus = 10;
    }

    ret bonus;
}

func main() int {
    // Variables (void, bool, str, char, int/int64, int32, float/float32, float64)
    let str title = "VekLang"; // Type is NECESSARY.
    let bool is_good = true;
    
    print(title);
    print("Is VekLang good?");
    print(is_good);

    // Dynamic arrays
    let int scores[] = {90, 45, 85}; // Also `let int scores[3] = {90, 45, 85}` - completely same.
    print("Original Array:");
    print(scores);

    // Array methods (push and pop)
    scores.push(100);
    let int popped = scores.pop(); 
    
    print("We pushed 100 and popped it. The popped value is:");
    print(popped);

    // Array indexing
    let int first_score = scores[0];
    print("The first score is:");
    print(first_score);

    // Custom function using array data
    let int my_bonus = calculate_bonus(first_score);
    print("Bonus points earned:");
    print(my_bonus);

    print("EOF");

    ret 0;
}
```

## TODO
- [ ] Organize src .cpp files instead of using .hpp only.
- [ ] Add classes.
- [x] Implement while/for loops.
- [ ] Organize the code.
- [ ] CMake file.
- [ ] Compiler and VM supposed to be different things. (_vekvm {filename}.vekbc_ to run, _vek run/build_ - developer tool) (turn .vekbc is a bytecode file, with magic, 3 bytes in header.)

## Acknowledgements
This project was built using AI tools as an occasional educational aid and a minor coding assistant (primarily for boilerplate and formatting). 
The overall architecture, core compiler/VM design, implementation, and final debugging were entirely driven and executed by the author.
