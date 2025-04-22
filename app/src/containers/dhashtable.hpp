#pragma once
// INFO:found this on sum random gogle search
// this algorithm (k=33) was first reported by dan bernstein many years ago in comp.lang.c. another version of this
// algorithm (now favored by bernstein) uses xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic of number 33 (why it
// works better than many other constants, prime or not) has never been adequately explained.

#include "core/dmemory.hpp"
#include "core/logger.hpp"
#include "defines.hpp"
#include <cstddef>

#define MAX_KEY_LENGTH 64
#define DEFAULT_HASH_TABLE_SIZE 20
#define DEFAULT_HASH_TABLE_RESIZE_FACTOR 2

template <typename value> class dhashtable
{
  private:
    u64  capacity;
    u64  element_size;
    u64  max_length;
    u64  num_elements_in_table;
    u64 *table;

  public:
    u64 hash_func(const char *key)
    {
        u64 hash = 5381;
        u32 c;

        while ((c = *key++))
        {
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        }
        return hash;

        return hash;
    }

    dhashtable()
    {
        element_size          = sizeof(value);
        capacity              = DEFAULT_HASH_TABLE_SIZE * element_size;
        max_length            = DEFAULT_HASH_TABLE_SIZE;
        num_elements_in_table = 0;
        table                 = (u64 *)dallocate(capacity, MEM_TAG_DHASHTABLE);
        dzero_memory(table, capacity);
    }
    dhashtable(u64 table_size)
    {
        element_size          = sizeof(value);
        capacity              = table_size * element_size;
        max_length            = table_size;
        num_elements_in_table = 0;
        table                 = (u64 *)dallocate(capacity, MEM_TAG_DHASHTABLE);
        dzero_memory(table, capacity);
    }
    ~dhashtable()
    {
        dfree(table, capacity, MEM_TAG_DHASHTABLE);
        element_size          = 0;
        capacity              = 0;
        max_length            = 0;
        num_elements_in_table = 0;
    }

    void insert(const char *key, value type)
    {
        if (key == nullptr)
        {
            DERROR("Key is nullptr. must have a key.");
            return;
        }
        u64 hash_code  = hash_func(key);
        hash_code     %= max_length;
        if (num_elements_in_table + 1 > max_length)
        {
            DWARN("Hashtable does not have enough capacity to insert element. Resizing");
            u64 *new_table = (u64 *)dallocate(capacity * DEFAULT_HASH_TABLE_RESIZE_FACTOR, MEM_TAG_DHASHTABLE);
            dcopy_memory(new_table, table, capacity);
            dfree(table, capacity, MEM_TAG_DHASHTABLE);
            capacity *= DEFAULT_HASH_TABLE_RESIZE_FACTOR;
            table     = new_table;
        }
        u64 *entry_ptr = table + hash_code;
        if (*entry_ptr != 0)
        {
            DWARN("Collison!!!. Overriding the value for index %d", hash_code);
            num_elements_in_table--;
        }
        dcopy_memory(entry_ptr, &type, element_size);
        num_elements_in_table++;
        return;
    }
    value find(const char *key)
    {
        if (!key)
        {
            DFATAL("Key is nullptr");
        }
        u64 hash_code   = hash_func(key);
        hash_code      %= max_length;
        u64 *entry_ptr  = table + hash_code;

        if (*entry_ptr == INVALID_ID_64)
        {
            DWARN("There is a no entry for hash code %d", hash_code);
        }
        value type = *(value *)entry_ptr;
        return type;
    }
    void erase(const char *key)
    {
        u64 hash_code   = hash_func(key);
        hash_code      %= max_length;
        u64 *entry_ptr  = table + hash_code;

        if (*entry_ptr == INVALID_ID_64)
        {
            DWARN("There is a no entry for hash code %d", hash_code);
            return;
        }
        dset_memory_value(entry_ptr, INVALID_ID_64, element_size);
        num_elements_in_table--;
        return;
    }

    void clear()
    {
        DWARN("Clearing everything!!!");
        dzero_memory(table, capacity);
    }
    u64 size()
    {
        return max_length;
    }
    u64 num_elements()
    {
        return num_elements_in_table;
    }
    u64 max_capacity()
    {
        return capacity;
    }
};
