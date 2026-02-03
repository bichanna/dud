# dud

dud is basically C for people who are lazy but want a fraction of the power of C. It's a programming language infused with lots of personal preferences for syntax, safety, and metaprogramming.

## Goals

- Basically C with a bit nicer syntax
- Compiles to standard C code, ensuring compatibility
- Automatic memory management through reference counting that handle circular references
- Built-in implementations of dynamic arrays, hash maps, and other useful things
- Use functions, types, and other stuff from C without too much fuss
- Generics and protocols!
- Zig-like modules, no more header files!

## Examples

```
type User = struct {
  id: i32,
  name: String,
  boss: ^User,
}

fn main() {
  // Stack allocation (No RC overhead, fast!)
  let me: User = User(1, "bichanna", null);

  // Heap allocation (Automatically reference counted)
  let sister: ^User = heap User(2, "sister", null); // allocating on heap

  // Look Ma, no '->'!
  me.boss = sister;

  println("Hi! My name is {} and my boss is {}", me.name, me.boss.name);
}
```
