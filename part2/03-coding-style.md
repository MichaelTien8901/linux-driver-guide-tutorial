---
layout: default
title: "2.3 Coding Style"
parent: "Part 2: Kernel Fundamentals"
nav_order: 3
---

# Kernel Coding Style

The Linux kernel has strict coding conventions. Following them is essential for upstream acceptance and code readability.

## Why Coding Style Matters

- **Consistency**: 20+ years of kernel code follows these rules
- **Readability**: Easier to review and maintain
- **Upstream acceptance**: Non-compliant patches are rejected
- **Tooling**: checkpatch.pl enforces style automatically

## Core Rules

### Indentation: Tabs, Not Spaces

```c
/* CORRECT: Tab indentation */
static int foo(void)
{
→       if (condition) {
→       →       do_something();
→       }
}

/* WRONG: Space indentation */
static int foo(void)
{
    if (condition) {
        do_something();
    }
}
```

- Use **tabs** (8 characters wide)
- Tabs for indentation, spaces for alignment

### Line Length: 80-100 Characters

Keep lines under 80 characters when possible, 100 max:

```c
/* CORRECT: Break long lines sensibly */
ret = some_really_long_function_name(argument1, argument2,
                                     argument3, argument4);

/* WRONG: Excessively long line */
ret = some_really_long_function_name(argument1, argument2, argument3, argument4, argument5);
```

### Brace Placement

Opening braces on the same line as the statement:

```c
/* CORRECT */
if (condition) {
        do_something();
} else {
        do_other();
}

for (i = 0; i < n; i++) {
        process(i);
}

/* WRONG: K&R style for if/for/while */
if (condition)
{
        do_something();
}
```

**Exception**: Functions have opening brace on new line:

```c
/* CORRECT for functions */
static int my_function(int x)
{
        return x * 2;
}

/* WRONG for functions */
static int my_function(int x) {
        return x * 2;
}
```

### Single-Statement Bodies

Omit braces for single statements (usually):

```c
/* CORRECT */
if (condition)
        do_something();

for (i = 0; i < n; i++)
        process(i);

/* Also CORRECT (some prefer braces always) */
if (condition) {
        do_something();
}
```

**Exception**: If one branch has braces, use them on all:

```c
/* CORRECT */
if (condition) {
        do_something();
        do_more();
} else {
        do_other();  /* Braces because if-branch has them */
}
```

### Spaces

**Around keywords:**
```c
if (condition)    /* Space after if */
for (i = 0; ...)  /* Space after for */
while (running)   /* Space after while */
switch (value)    /* Space after switch */
return value;     /* Space after return if returning value */
```

**Around operators:**
```c
x = a + b;        /* Spaces around binary operators */
x += 1;           /* Spaces around assignment */
if (a == b)       /* Spaces around comparison */
```

**No space after function names:**
```c
function(args);   /* CORRECT */
function (args);  /* WRONG */
```

**No space inside parentheses:**
```c
if (condition)    /* CORRECT */
if ( condition )  /* WRONG */
```

### Naming Conventions

**Functions and variables: lowercase with underscores**
```c
static int read_sensor_data(void);
int buffer_size = 1024;
```

**Macros and constants: UPPERCASE**
```c
#define MAX_BUFFER_SIZE 4096
#define DRIVER_NAME "my_driver"
```

**Types: lowercase with _t suffix (or no suffix)**
```c
typedef struct {
        int x, y;
} point_t;
```

**Global variables: Prefix with module name**
```c
static int mydrv_debug_level = 0;
static struct device *mydrv_dev;
```

## Function Style

### Declaration

```c
/* Return type on same line if it fits */
static int my_function(int arg1, char *arg2)

/* Or on separate line if long */
static struct some_very_long_type *
create_new_object(struct parent *parent, const char *name)
```

### Argument Alignment

```c
/* Align continued arguments */
ret = register_device(dev, name,
                      flags, mode,
                      callback);
```

### Function Length

- Aim for functions under **40-50 lines**
- One function, one purpose
- If it's too long, split it

## Comments

### C-style Comments

```c
/* This is a single-line comment */

/*
 * This is a multi-line comment.
 * Note the leading asterisks aligned.
 */
```

**Never use C++ comments in C files:**
```c
// WRONG: Don't use C++ style comments
```

### Function Documentation

Use kernel-doc format for exported functions:

```c
/**
 * my_function - Brief description
 * @arg1: Description of first argument
 * @arg2: Description of second argument
 *
 * Longer description of what this function does.
 * Can span multiple paragraphs.
 *
 * Context: Can be called from process context only.
 * Return: 0 on success, negative errno on failure.
 */
int my_function(int arg1, char *arg2)
{
        ...
}
```

## Using checkpatch.pl

The kernel includes a style checker:

```bash
# Check a file
/kernel/linux/scripts/checkpatch.pl --file mydriver.c

# Check a patch
/kernel/linux/scripts/checkpatch.pl mypatch.patch

# Stricter checking
/kernel/linux/scripts/checkpatch.pl --strict --file mydriver.c
```

### Common checkpatch Warnings

| Warning | Fix |
|---------|-----|
| "code indent should use tabs" | Replace spaces with tabs |
| "line over 80 characters" | Break the line |
| "space required after ','" | Add space |
| "braces {} not necessary" | Remove braces from single statement |
| "trailing whitespace" | Remove trailing spaces |
| "Missing Signed-off-by" | Add SOB line to patch |

## SPDX License Identifier

Every source file must start with SPDX identifier:

```c
// SPDX-License-Identifier: GPL-2.0
/*
 * my_driver.c - My awesome driver
 */
```

Common licenses:
- `GPL-2.0` - GNU GPL version 2 only
- `GPL-2.0-only` - Same as above (explicit)
- `GPL-2.0-or-later` - GPL v2 or any later version
- `GPL-2.0 OR BSD-2-Clause` - Dual-licensed

## Kernel-Specific Patterns

### Error Codes

Return negative errno values:

```c
if (error)
        return -EINVAL;  /* Not EINVAL, not -1 */
```

### Goto for Cleanup

Standard cleanup pattern:

```c
static int init_device(void)
{
        int ret;

        ret = allocate_resources();
        if (ret)
                return ret;

        ret = setup_hardware();
        if (ret)
                goto err_setup;

        ret = register_device();
        if (ret)
                goto err_register;

        return 0;

err_register:
        teardown_hardware();
err_setup:
        free_resources();
        return ret;
}
```

### NULL Checks

```c
/* CORRECT */
if (!ptr)
        return -EINVAL;

if (ptr)
        use_ptr(ptr);

/* Also acceptable but verbose */
if (ptr == NULL)
        return -EINVAL;
```

### Boolean Expressions

```c
/* CORRECT: Use bare expression */
if (is_valid)
        ...

/* WRONG: Redundant comparison */
if (is_valid == true)
        ...
```

## Summary

Key style rules:
- Tabs for indentation (8 spaces wide)
- 80-100 character line limit
- K&R brace style for control structures, function-style for functions
- Spaces around operators and keywords
- lowercase_with_underscores for names
- Use checkpatch.pl before submitting

## Resources

- [Kernel Coding Style](https://www.kernel.org/doc/html/latest/process/coding-style.html)
- `scripts/checkpatch.pl` in kernel source

## Next

Learn about [kernel data types]({% link part2/04-data-types.md %}) and endianness handling.
