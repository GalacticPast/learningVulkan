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
        bool   is_initialized = false;
        char   key[MAX_KEY_LENGTH];
        T      type;
        entry *next = nullptr;
        entry *prev = nullptr;
    };
    entry *table;

    entry *default_entry = nullptr;

    u64  hash_func(const char *key);
    u64  _find_empty_spot(u64 hash_code);
    void _im_paranoid();
    void _resize_and_rehash();

    u64 capacity;
    u64 element_size;
    u64 max_length;
    u64 num_elements_in_table;

  public:
    dhashtable();
    dhashtable(u64 table_size);

    void c_init();
    void c_init(u64 size);

    ~dhashtable();

    void _print_hashtable();

    u64 max_capacity();
    u64 num_elements();
    u64 size();

    void clear();

    bool erase(const char *key);
    T   *find(const char *key);
    void insert(const char *key, T type);

    void set_default_value(T default_val);
};

template <typename T> u64 dhashtable<T>::_find_empty_spot(u64 hash_code)
{
    u64 i  = hash_code + 1;
    i     %= max_length;

    while (i != hash_code)
    {
        if (!table[i].is_initialized)
        {
            return i;
        }
        i++;
        i %= max_length;
    }
    DERROR("Couldn't find empty spot");
    return INVALID_ID_64;
}
template <typename T> void dhashtable<T>::_resize_and_rehash()
{
    DWARN("Hashtable does not have enough capacity to insert element. Resizing and rehashing");

    // get the pointer to the old table
    entry *old_table      = table;
    u64    old_capacity   = capacity;
    u64    old_max_length = max_length;

    entry *new_table  = (entry *)dallocate(capacity * DEFAULT_HASH_TABLE_RESIZE_FACTOR, MEM_TAG_DHASHTABLE);
    table             = new_table;
    capacity         *= DEFAULT_HASH_TABLE_RESIZE_FACTOR;
    max_length        = capacity / element_size;
    DTRACE("Num_elements_in_table before resize and rehash %d", num_elements_in_table);
    num_elements_in_table = 0;

    for (u64 i = 0; i < old_max_length; i++)
    {
        if (old_table[i].is_initialized)
        {
            old_table[i].next           = nullptr;
            old_table[i].prev           = nullptr;
            old_table[i].is_initialized = false;
            insert(old_table[i].key, old_table[i].type);
        }
    }
    dfree(old_table, old_capacity, MEM_TAG_DHASHTABLE);
}
template <typename T> void dhashtable<T>::_im_paranoid()
{
    for (u64 i = 0; i < max_length; i++)
    {
        dzero_memory(&table[i], sizeof(entry));
    }
}
template <typename T> void dhashtable<T>::_print_hashtable()
{
    for (u64 i = 0; i < max_length; i++)
    {
        if (table[i].is_initialized)
        {
            DTRACE("%s", table[i].key);
        }
    }
}

template <typename T> u64 dhashtable<T>::hash_func(const char *key)
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

template <typename T> dhashtable<T>::dhashtable()
{
    element_size          = sizeof(entry);
    capacity              = DEFAULT_HASH_TABLE_SIZE * element_size;
    max_length            = DEFAULT_HASH_TABLE_SIZE;
    num_elements_in_table = 0;
    default_entry         = nullptr;
    table                 = (entry *)dallocate(capacity, MEM_TAG_DHASHTABLE);
}

template <typename T> dhashtable<T>::dhashtable(u64 table_size)
{
    element_size          = sizeof(entry);
    capacity              = table_size * element_size;
    max_length            = table_size;
    num_elements_in_table = 0;
    table                 = (entry *)dallocate(capacity, MEM_TAG_DHASHTABLE);
    default_entry         = nullptr;
}

template <typename T> void dhashtable<T>::c_init()
{
    element_size          = sizeof(entry);
    capacity              = DEFAULT_HASH_TABLE_SIZE * element_size;
    max_length            = DEFAULT_HASH_TABLE_SIZE;
    num_elements_in_table = 0;
    default_entry         = nullptr;
    table                 = (entry *)dallocate(capacity, MEM_TAG_DHASHTABLE);
}
template <typename T> void dhashtable<T>::c_init(u64 table_size)
{
    element_size          = sizeof(entry);
    capacity              = table_size * element_size;
    max_length            = table_size;
    num_elements_in_table = 0;
    table                 = (entry *)dallocate(capacity, MEM_TAG_DHASHTABLE);
    default_entry         = nullptr;
}

template <typename T> dhashtable<T>::~dhashtable()
{
    dfree(table, capacity, MEM_TAG_DHASHTABLE);
    element_size          = 0;
    capacity              = 0;
    max_length            = 0;
    num_elements_in_table = 0;
}

template <typename T> void dhashtable<T>::set_default_value(T default_val)
{
    if (!default_entry)
    {
        default_entry = (entry *)dallocate(sizeof(entry), MEM_TAG_DHASHTABLE);
    }
    default_entry->type = default_val;
}

template <typename T> void dhashtable<T>::insert(const char *key, T type)
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

        DTRACE("Max capacity reached. Resizing and rehashing hahstable entries");
        _resize_and_rehash();

        hash_code  = hash_func(key);
        hash_code %= max_length;
    }

    entry *entry_ptr = &table[hash_code];
    if (table[hash_code].is_initialized)
    {

        entry *prev = nullptr;
        while (entry_ptr->next)
        {
            prev      = entry_ptr;
            entry_ptr = entry_ptr->next;
        }
        u64 get_empty_id = _find_empty_spot(hash_code);
        if (get_empty_id == INVALID_ID_64)
        {
            DFATAL("Table size: %d, elements_in_table: %d", max_length, num_elements_in_table);
        }
        DASSERT(get_empty_id != INVALID_ID_64);

        entry_ptr->next = &table[get_empty_id];
        entry_ptr->prev = prev;
        entry_ptr       = entry_ptr->next;
    }
    dcopy_memory(entry_ptr->key, (void *)key, strlen(key));
    entry_ptr->type           = type;
    entry_ptr->next           = nullptr;
    entry_ptr->is_initialized = true;

    num_elements_in_table++;
    return;
}

template <typename T> T *dhashtable<T>::find(const char *key)
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

template <typename T> bool dhashtable<T>::erase(const char *key)
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

template <typename T> void dhashtable<T>::clear()
{
    DWARN("Clearing everything!!!");
    dzero_memory(table, capacity);
}
template <typename T> u64 dhashtable<T>::size()
{
    return max_length;
}
template <typename T> u64 dhashtable<T>::num_elements()
{
    return num_elements_in_table;
}
template <typename T> u64 dhashtable<T>::max_capacity()
{
    return capacity;
}
