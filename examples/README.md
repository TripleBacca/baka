# Examples

Small programs that show off baka's distinctive features. Each one prints what it
demonstrates.

| File | Shows |
|---|---|
| `01_loop_labels.cpp` | Loop labels: `for (...; Name)`, `while (cond; Name)`, `do ... while (cond; Name)` with `break Name;` / `continue Name;` |
| `02_quad_equals.cpp` | `====` / `!===`: do two objects have the same runtime class? (objects, references and pointers) |
| `03_record_equality.cpp` | `==` / `!=` on whole structs and classes, compared field by field |
| `04_classes.cpp` | Access modifiers, inheritance, methods virtual by default, pure methods (`= 0`), `new(p, args)` / `delete(p)` |
| `05_lambdas_references.cpp` | Captureless lambdas, references and function pointers |

## Running them

```sh
make run file=examples/02_quad_equals.cpp
```

This compiles the program and runs it under QEMU in Docker (`make mips-runtime`
builds the image the first time). To see a single stage instead:

```sh
./build/baka --ir examples/03_record_equality.cpp       # typed three-address code
./build/baka --codegen examples/04_classes.cpp          # MIPS64 assembly
```

The programs also paste straight into Compiler Explorer (select C).

## Expected output

**01_loop_labels**
```
first > 4 is 5 at row 0, col 2
row 0 is all odd
row 1 has an even number, skipping it
row 2 has an even number, skipping it
do-while stopped at 3
```

**02_quad_equals**
```
same kind of animal
different kinds of animal
pet is a dog? 0
pet is a dog now? 1
rex !=== tom: 1
copy is still a dog? 0
```

**03_record_equality**
```
a == b: 1
after changing b.y, a != b: 1
lines with equal points: 1
two equal squares through Shape&: 1
after changing a side: 0
square vs plain shape: 0
```

**04_classes**
```
area 6
area 16
```

**05_lambdas_references**
```
answer 42, square(5) 25, next 10
after swap: a = 2, b = 1
a after writing through alias: 100
```

The rules behind each feature are in the wiki: loop labels in the main README,
`====` on page 20, record `==` on page 19, classes on pages 15 and 18, lambdas on
page 17 and references on page 16.
