# Data Structures Demo

Demonstrates kernel data structures: linked lists (`list_head`) and hash tables (`DEFINE_HASHTABLE`) with insert, search, iteration, and deletion.

## Files

- `data_structures_demo.c` - Module with list and hash table operations
- `Makefile` - Build configuration
- `README.md` - This file

## Features Demonstrated

1. **Linked lists (`list_head`)**
   - `LIST_HEAD()` static declaration
   - `list_add_tail()` for insertion
   - `list_for_each_entry()` for iteration
   - `list_for_each_entry_safe()` for safe deletion during iteration

2. **Hash tables**
   - `DEFINE_HASHTABLE()` declaration
   - `hash_add()` for insertion
   - `hash_for_each_possible()` for key-based lookup
   - `hash_for_each_safe()` for safe cleanup

## Building

```bash
make
```

## Testing

### Quick Test

```bash
make test
```

### Manual Testing

```bash
# Load module
sudo insmod data_structures_demo.ko

# Populate with sample data
echo "populate" | sudo tee /proc/data_structures_demo

# View all data (list + hash table)
cat /proc/data_structures_demo

# Search hash table by key
echo "find 200" | sudo tee /proc/data_structures_demo

# Clear all data
echo "clear" | sudo tee /proc/data_structures_demo

# Verify empty
cat /proc/data_structures_demo

# Check kernel log
dmesg | tail -10

# Unload
sudo rmmod data_structures_demo
```

## Key Takeaways

- Kernel lists are intrusive: embed `list_head` in your structure
- Use `list_for_each_entry_safe()` when removing entries during iteration
- Hash tables provide O(1) average lookup by key
- Always free all entries before module unload
