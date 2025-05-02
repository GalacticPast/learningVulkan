#pragma once
// INFO:found this on sum random gogle search
// this algorithm (k=33) was first reported by dan bernstein many years ago in comp.lang.c. another version of this
// algorithm (now favored by bernstein) uses xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic of number 33 (why it
// works better than many other constants, prime or not) has never been adequately explained.

#include "core/dmemory.hpp"
#include "core/dstring.hpp"
#include "core/logger.hpp"
#include "defines.hpp"
#include <string.h>

#define MAX_KEY_LENGTH 64
#define DEFAULT_HASH_TABLE_SIZE 20
#define DEFAULT_HASH_TABLE_RESIZE_FACTOR 2

template <typename T> class dhashtable
{
  private:
    struct entry
    {
        char key[MAX_KEY_LENGTH];
        T    type;
    };

  public:
    u64   capacity;
    u64   element_size;
    u64   max_length;
    u64   num_elements_in_table;
    void *table;

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
        element_size          = sizeof(entry);
        capacity              = DEFAULT_HASH_TABLE_SIZE * element_size;
        max_length            = DEFAULT_HASH_TABLE_SIZE;
        num_elements_in_table = 0;
        table                 = dallocate(capacity, MEM_TAG_DHASHTABLE);
        dzero_memory(table, capacity);
    }
    dhashtable(u64 table_size)
    {
        element_size          = sizeof(entry);
        capacity              = table_size * element_size;
        max_length            = table_size;
        num_elements_in_table = 0;
        table                 = dallocate(capacity, MEM_TAG_DHASHTABLE);
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

    void insert(const char *key, T type)
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
            u8 *new_table = (u8 *)dallocate(capacity * DEFAULT_HASH_TABLE_RESIZE_FACTOR, MEM_TAG_DHASHTABLE);
            dcopy_memory(new_table, table, capacity);
            dfree(table, capacity, MEM_TAG_DHASHTABLE);
            capacity *= DEFAULT_HASH_TABLE_RESIZE_FACTOR;
            table     = new_table;
        }

        entry *entry_ptr = (entry *)((u8 *)table + (hash_code * element_size));
        if (entry_ptr->key[0] != '\0')
        {
            DWARN("Collison!!!. Overriding the entry: key%s", entry_ptr->key);
            num_elements_in_table--;
        }
        entry insert{};
        dcopy_memory(entry_ptr->key, (void *)key, strlen(key));
        entry_ptr->type = type;

        num_elements_in_table++;
        return;
    }
    T *find(const char *key)
    {
        if (!key)
        {
            DFATAL("Key is nullptr");
        }
        u64 hash_code  = hash_func(key);
        hash_code     %= max_length;

        entry *entry_ptr = (entry *)((u8 *)table + (hash_code * element_size));

        if (!string_compare(key, entry_ptr->key))
        {
            DWARN("There is a no entry for key:%s Returning nullptr", key);
            return nullptr;
        }
        T *type_ref = &entry_ptr->type;
        return type_ref;
    }

    bool erase(const char *key)
    {
        u64 hash_code  = hash_func(key);
        hash_code     %= max_length;

        entry *entry_ptr = (entry *)((u8 *)table + (hash_code * element_size));

        if (!string_compare(key, entry_ptr->key))
        {
            DWARN("There is a no entry for key:%s not erasing anything", key);
            return false;
        }
        dset_memory_value(entry_ptr, 0, element_size);
        num_elements_in_table--;
        return true;
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
