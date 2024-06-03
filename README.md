# smallpp - Small Protobuf Protocol implementation.

Extremely tiny C++ single-header library that implements basics of Protobuf protocol and provides a generator that parses proto 2 syntax.

Meant to be used in embeded systems or places where using standard library is not ideal.

## Features
[X] Serializing and deserializing with zero allocations.
[X] Can be configured to not use standard library.
[X] Can be configured to use or disable virtual method table.
[X] Consists only of code. Doesn't provide ANY metadata or other garbage.
[X] Supports repeated fields (currently only via vectors from standard library).
[X] Supports custom allocators (you can even allocate on stack).
[X] Supports all the scalar types protobuf supports.
[X] Supports message fields.
[X] Supports enums.
[X] Follows proto 2 syntax.

## Cons
- Currently only works with proto 2 syntax.
- Doesn't support a lot of protobuf stuff (more on it you can find in [smallpp.h](./src/smallpp/smallpp.h).

## Usage
First use generator like this:
```
generator.exe <output dir> <files...>
```

To learn how to use messages please check [example](./src/example/main.cpp).
