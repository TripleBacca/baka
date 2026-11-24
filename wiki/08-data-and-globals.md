# 8. Data and globals

After the functions and the runtime, the printer emits every global variable and the
floating-point literal pool.

## How globals are initialized

The language rule is: every global starts zero-filled, then **every explicit global
initializer runs at program start, in source order**. That includes constant ones like
`int g = 5;` and `int* p = &arr[3];`.

- The IR lowerer collects all of these into one generated function, `.module_init`, which
  `__baka_start` calls before `main` ([page 9](09-runtime-and-startup.md)).
- After each global is initialized, its destructor (if any) is registered so it runs
  after `main`.
- In the data sections, ordinary globals are therefore just zero-filled storage.

## Which section each global goes in

| Section | Contents |
|---|---|
| `.rodata` | Read-only data fully known at compile time: string literals |
| `.bss` | Globals whose initial image is all zero, which in practice means every ordinary global, including `const` ones, since their values are written by `.module_init`; costs no space in the file |
| `.data` | Globals with a non-zero initial image |

## Building a global's initial image (backend capability)

The data emitter supports more than the lowerer currently produces:

1. Start from the global's byte image (for example, the characters of a string).
2. Overlay each initial value at its offset: integers and floats as little-endian bytes.
3. Values that are **addresses** of other globals or functions are emitted as
   `.dword symbol+offset`, and the linker fills them in.

Runs of zero bytes are emitted as `.zero N`; the rest as `.byte` lists. Today only string
literals use the byte image; steps 2 and 3 are ready for a future change that moves
constant initializers from start-up code into data.

## Symbol names

- Globals and functions defined in the program keep the IR's unique `name.id` spelling,
  for example `counter.12`, so they never collide with each other or with libc.
- Compiler-generated names that start with a dot get a `__baka.` prefix.
- Library functions keep their plain names (`printf`) so the linker finds them in glibc.
- Globals used only inside the file are not marked global in the assembly.

## The literal pool

Floating constants cannot be instruction immediates. Each distinct bit pattern is stored
once in `.rodata` under a local label and loaded from there. Zero is special-cased: it is
moved directly from `$zero`.
