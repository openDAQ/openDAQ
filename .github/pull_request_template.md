# Brief

(Concise one-line description of what changed and what was added/removed, maybe even why)



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

# API changes

> [!NOTE]
> Modifying, removing, or adding a function to an interface inherited by another breaks binary compatibility of module shared libraries

(An overview of changes on the interface level (abstract structs with pure virtual functions), if any, one function per line)

```diff
+ this
- that
```

# Required application changes

(Changes required in openDAQ applications/executables)

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

(Changes required in openDAQ shared libraries/modules)

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
