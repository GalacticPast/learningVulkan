#pragma once
// INFO:found this on sum random gogle search
// this algorithm (k=33) was first reported by dan bernstein many years ago in comp.lang.c. another version of this
// algorithm (now favored by bernstein) uses xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic of number 33 (why it
// works better than many other constants, prime or not) has never been adequately explained.

#include "core/dasserts.hpp"
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
        bool   is_intialized = false;
        char   key[MAX_KEY_LENGTH];
        T      type;
        entry *next = nullptr;
        entry *prev = nullptr;
    };
    u64 _find_empty_spot(u64 hash_code)
    {
        for (u64 i = 0; i < max_length; i++)
        {
            if (table[i].is_intialized == false && table[i].key[0] == '\0' && !table[i].next && !table[i].prev)
            {
                return i;
            }
        }

        return INVALID_ID_64;
    }
    void _resize_and_rehash()
    {
        DWARN("Hashtable does not have enough capacity to insert element. Resizing and rehashing");

        // get the pointer to the old table
        entry *old_table    = table;
        u64    old_capacity = capacity;

        entry *new_table  = (entry *)dallocate(capacity * DEFAULT_HASH_TABLE_RESIZE_FACTOR, MEM_TAG_DHASHTABLE);
        table             = new_table;
        capacity         *= DEFAULT_HASH_TABLE_RESIZE_FACTOR;
        max_length        = capacity / element_size;

        for (u64 i = 0; i < max_length; i++)
        {
            if (old_table[i].key[0] != '\0')
            {
                insert(old_table[i].key, old_table->type);
            }
        }
        dfree(old_table, old_capacity, MEM_TAG_DHASHTABLE);
    }
    void _im_paranoid()
    {
        for (u64 i = 0; i < max_length; i++)
        {
            dzero_memory(&table[i], sizeof(entry));
        }
    }
    void _print_hashtable()
    {
        for (u64 i = 0; i < max_length; i++)
        {
            DTRACE("%s", table[i].key);
        }
    }

  public:
    u64    capacity;
    u64    element_size;
    u64    max_length;
    u64    num_elements_in_table;
    entry *default_entry = nullptr;
    entry *table;

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
        default_entry         = nullptr;
        table                 = (entry *)dallocate(capacity, MEM_TAG_DHASHTABLE);
    }

    dhashtable(u64 table_size)
    {
        element_size          = sizeof(entry);
        capacity              = table_size * element_size;
        max_length            = table_size;
        num_elements_in_table = 0;
        table                 = (entry *)dallocate(capacity, MEM_TAG_DHASHTABLE);
        default_entry         = nullptr;
    }

    ~dhashtable()
    {
        dfree(table, capacity, MEM_TAG_DHASHTABLE);
        element_size          = 0;
        capacity              = 0;
        max_length            = 0;
        num_elements_in_table = 0;
    }

    void set_default_value(T default_val)
    {
        if (!default_entry)
        {
            default_entry = (entry *)dallocate(sizeof(entry), MEM_TAG_DHASHTABLE);
        }
        default_entry->type = default_val;
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
            _resize_and_rehash();

            hash_code  = hash_func(key);
            hash_code %= max_length;
        }

        entry *entry_ptr = &table[hash_code];
        if (table[hash_code].is_intialized && table[hash_code].key[0] == '\0' && !table[hash_code].next &&
            !table[hash_code].prev)
        {
            DWARN("Collison!!!. Provided key%s and key%s have the same hash %d", key, entry_ptr->key, hash_code);

            entry *prev = nullptr;
            while (entry_ptr->next)
            {
                prev      = entry_ptr;
                entry_ptr = entry_ptr->next;
            }
            u64 get_empty_id = _find_empty_spot(hash_code);
            DASSERT(get_empty_id != INVALID_ID_64);

            entry_ptr->next = &table[get_empty_id];
            entry_ptr->prev = prev;
            entry_ptr       = entry_ptr->next;
            num_elements_in_table--;
        }
        dcopy_memory(entry_ptr->key, (void *)key, strlen(key));
        entry_ptr->type          = type;
        entry_ptr->next          = nullptr;
        entry_ptr->is_intialized = true;

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

        entry *entry_ptr = &table[hash_code];

        while (entry_ptr)
        {
            if (string_compare(key, entry_ptr->key))
            {
                break;
            }
            entry_ptr = entry_ptr->next;
        }
        if (!entry_ptr)
        {
            if (default_entry)
            {
                DWARN("Couldnt find key: %s. It doesnt exist in the hashtable, returning default_val", key);
                return &default_entry->type;
            }
            DWARN("Couldnt find key: %s. It doesnt exist in the hashtable, returning nullptr", key);
            return nullptr;
        }
        T *type_ref = &entry_ptr->type;
        return type_ref;
    }

    bool erase(const char *key)
    {
        u64 hash_code  = hash_func(key);
        hash_code     %= max_length;

        entry *entry_ptr = &table[hash_code];
        bool   found     = string_compare(key, entry_ptr->key);

        if (!found && entry_ptr->next)
        {
            entry *prev = entry_ptr;
            while (entry_ptr)
            {
                if (string_compare(key, entry_ptr->key))
                {
                    dset_memory_value(entry_ptr, 0, element_size);
                }
                prev      = entry_ptr;
                entry_ptr = entry_ptr->next;
            }
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
