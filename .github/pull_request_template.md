# Brief
(Brief should be a (preferably concise one-line) high-level description of what changed and what was added/removed, maybe even why)



> [!CAUTION]
> Breaks binary compatibility

# Description

(Individual high-level changes)

- Add Python GUI Demo functionality
- Rename a function
- ...

# Usage example

In C++ use:

```cpp
auto variable = "foo";
```

In terminal use:

```shell
cd folder
```

# Required application changes

(Changes to integration of openDAQ required for client integration to work after a version update, otherwise write "None.")

- (E.g. change renamed function name)
- ...

Instead of:

```cpp
foo();
```

Do:

```cpp
bar();
```

# Required module changes

(Describe how we broke module integration and how to fix it, otherwise write "None.")

- (E.g. change renamed function name)
- ...

Instead of:

```cpp
foo();
```

Do:

```cpp
bar();
```

# API changes

(An overview of changes on the interface level (abstract structs with pure virtual functions), if any, one function per line)

```diff
+ this
- that
```
