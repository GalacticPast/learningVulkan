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

#include "math/dmath.hpp"

#include <string.h>

#define MAX_KEY_LENGTH 64
#define DEFAULT_HASH_TABLE_SIZE 20
#define DEFAULT_HASH_TABLE_RESIZE_FACTOR 2

template <typename T> class dhashtable
{
  private:
    struct entry
    {
        bool   is_initialized    = false;
        entry *next              = nullptr;
        entry *prev              = nullptr;
        u32    unique_identifier = INVALID_ID;

        char key[MAX_KEY_LENGTH];

        T type;
    };
    entry *table;

    entry *default_entry = nullptr;

    u64 hash_func(const char *key);
    u64 hash_func(const u64 key);

    u64 _find_empty_spot(u64 hash_code);

    void _im_paranoid();
    void _print_hashtable();

    void _resize_and_rehash();

    u64    capacity;
    u64    element_size;
    u64    max_length;
    u64    num_elements_in_table;
    arena *arena = nullptr;

  public:
    bool is_non_resizable = false;

    dhashtable(struct arena *arena);
    dhashtable(struct arena *arena, u64 table_size);

    void c_init(struct arena *arena);
    void c_init(struct arena *arena, u64 size);

    u64 get_size_requirements(u64 size);
    // WARN: before you call this function you have to call the get size requirements function so that you can get the
    // correct memory size to pass to this fucntion!!!! WARNING IF YOU DONT THEN YOU WILL HAVE A SERIOUS BUG.
    void c_init(void *block, u64 size);

    ~dhashtable();

    u64 max_capacity();
    u64 num_elements();
    u64 size();

    void clear();

    bool erase(const char *key);
    bool erase(u64 key);

    T *find(const u64 key);
    T *find(const char *key);

    /* INFO: this will return the index of the table where the value will be inserted, this is kind of a bad idea
     * because when the hashtable resizes the previous returned values would not be valid. So this will only work for
     * fixed size hashtables. "DONE: Maybe I should add a flag indicating that its a fixed size hashtable??" Call this
     * only for fixed size hashtables */
    u64  insert(const u64 key, T type);
    void insert(const char *key, T type);

    bool update(const u64 key, T type);
    void update(const char *key, T type);

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

    entry *new_table =
        static_cast<entry *>(dallocate(arena, capacity * DEFAULT_HASH_TABLE_RESIZE_FACTOR, MEM_TAG_DHASHTABLE));
    table       = new_table;
    capacity   *= DEFAULT_HASH_TABLE_RESIZE_FACTOR;
    max_length  = capacity / element_size;
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

template <typename T> u64 dhashtable<T>::get_size_requirements(u64 table_size)
{
    return sizeof(entry) * table_size;
}

template <typename T> u64 dhashtable<T>::hash_func(const u64 key)
{
    // SplitMix64 (used in PCG and others)
    // -> https://github.com/indiesoftby/defold-splitmix64?tab=readme-ov-file
    u64 x  = key;
    x     += 0x9e3779b97f4a7c15;
    x      = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
    x      = (x ^ (x >> 27)) * 0x94d049bb133111eb;
    return x ^ (x >> 31);
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

template <typename T> dhashtable<T>::dhashtable(struct arena *arena)
{
    element_size          = sizeof(entry);
    capacity              = DEFAULT_HASH_TABLE_SIZE * element_size;
    max_length            = DEFAULT_HASH_TABLE_SIZE;
    num_elements_in_table = 0;
    default_entry         = nullptr;
    table                 = static_cast<entry *>(dallocate(arena, capacity, MEM_TAG_DHASHTABLE));
}

template <typename T> dhashtable<T>::dhashtable(struct arena *arena, u64 table_size)
{
    element_size          = sizeof(entry);
    capacity              = table_size * element_size;
    max_length            = table_size;
    num_elements_in_table = 0;
    table                 = static_cast<entry *>(dallocate(arena, capacity, MEM_TAG_DHASHTABLE));
    default_entry         = nullptr;
}

template <typename T> void dhashtable<T>::c_init(struct arena *arena)
{
    element_size          = sizeof(entry);
    capacity              = DEFAULT_HASH_TABLE_SIZE * element_size;
    max_length            = DEFAULT_HASH_TABLE_SIZE;
    num_elements_in_table = 0;
    default_entry         = nullptr;
    table                 = static_cast<entry *>(dallocate(arena, capacity, MEM_TAG_DHASHTABLE));
}
template <typename T> void dhashtable<T>::c_init(void *block, u64 table_size)
{
    element_size          = sizeof(entry);
    capacity              = table_size * element_size;
    max_length            = table_size;
    num_elements_in_table = 0;
    table                 = static_cast<entry *>(block);
    default_entry         = nullptr;
}

template <typename T> void dhashtable<T>::c_init(struct arena *arena, u64 table_size)
{
    element_size          = sizeof(entry);
    capacity              = table_size * element_size;
    max_length            = table_size;
    num_elements_in_table = 0;
    table                 = static_cast<entry *>(dallocate(arena, capacity, MEM_TAG_DHASHTABLE));
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
        default_entry = static_cast<entry *>(dallocate(arena, sizeof(entry), MEM_TAG_DHASHTABLE));
    }
    default_entry->type = default_val;
}

template <typename T> bool dhashtable<T>::update(const u64 key, T type)
{
    s64 dummy = key;
    if (key == INVALID_ID_64)
    {
        DERROR("Invalid key for updating the entry.");
    }
    u32 high = static_cast<u32>(dummy >> 32);
    u32 low  = static_cast<u32>(dummy);

    entry *entry_ptr = &table[low];
    if (table[low].is_initialized)
    {
        entry_ptr->type              = type;
    }
    else
    {
        DERROR("Coulndt associate a entry with key %llu, high %d low %d.",key, high, low);
        return false;
    }
    return true;
}
//
// should make the bottom 32 bits as the index for the hastable entry and the top 32 bits for the unique-id;
template <typename T> u64 dhashtable<T>::insert(const u64 key, T type)
{
    s64 dummy = key;
    if (key == INVALID_ID_64)
    {
        dummy = drandom_s64();
    }

    // generate a unqiue identifier and a index
    u64 hash_code = hash_func(dummy);

    // the high 32 bits will be the unique identifier that we will match for when the user queries for the data.
    // the low 32 bits will be the index for the query.
    u32 high = static_cast<u32>(dummy >> 32);
    u32 low  = static_cast<u32>(dummy);

    low %= max_length;

    if (num_elements_in_table + 1 > max_length)
    {

        if (is_non_resizable)
        {
            DASSERT_MSG(
                1 == 0,
                "Hashtable is signaled as non-resizable. Max entries reached. Increase the capacity of the table!!!");
        }
        DTRACE("Max capacity reached. Resizing and rehashing hahstable entries");
        _resize_and_rehash();

        hash_code = hash_func(dummy);

        high  = static_cast<u32>(dummy >> 32);
        low   = static_cast<u32>(dummy);
        low  %= max_length;
    }

    entry *entry_ptr = &table[low];
    if (table[low].is_initialized)
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

        entry_ptr->next = &table[get_empty_id];
        entry_ptr->prev = prev;
        entry_ptr       = entry_ptr->next;

        low = get_empty_id;
    }

    entry_ptr->unique_identifier = high;
    entry_ptr->type              = type;
    entry_ptr->next              = nullptr;
    entry_ptr->is_initialized    = true;

    num_elements_in_table++;

    u64 final_id = (static_cast<u64>(high) << 32) | low;
    DASSERT(final_id != INVALID_ID_64);

    return final_id;
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

        if (is_non_resizable)
        {
            DASSERT_MSG(
                1 == 0,
                "Hashtable is signaled as non-resizable. Max entries reached. Increase the capacity of the table!!!");
        }
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
    dcopy_memory(entry_ptr->key, static_cast<const void *>(key), strlen(key));
    entry_ptr->type           = type;
    entry_ptr->next           = nullptr;
    entry_ptr->is_initialized = true;

    num_elements_in_table++;
    return;
}

template <typename T> T *dhashtable<T>::find(const u64 key)
{
    if (key == INVALID_ID_64)
    {
        DFATAL("Key is invalid_id_64");
    }

    u32 high = static_cast<u32>(key >> 32);
    u32 low  = static_cast<u32>(key);

    entry *entry_ptr = &table[low];

    if (!table[low].is_initialized)
    {
        if (default_entry)
        {
            DWARN("Couldnt find unique_identifer: %d for index: %d. It doesnt exist in the hashtable, returning "
                  "default_val",
                  key);
            return &default_entry->type;
        }
        DERROR("The provided key %llu high: %d , low: %d, doesnt map to a entry. Returning nullptr", key, high, low);
        return nullptr;
    }
    if (entry_ptr->unique_identifier != high)
    {
        if (is_non_resizable)
        {
            DFATAL("The provided key %llu high: %d , low: %d, maps to a incorrent entry. You mayhave resized a "
                   "un-resizable array so that the hash codes were changed.",
                   key, high, low);
        }
        else
        {
            DFATAL(
                "The provided key %llu high: %d , low: %d, maps to a incorrent entry. Something is really wrong here.",
                key, high, low);
        }
    }

    T *type_ref = &entry_ptr->type;
    return type_ref;
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

template <typename T> bool dhashtable<T>::erase(u64 key)
{
    if (key == INVALID_ID_64)
    {
        DFATAL("Key is invalid_id_64");
    }

    u32 high = static_cast<u32>(key >> 32);
    u32 low  = static_cast<u32>(key);

    entry *entry_ptr = &table[low];

    if (!table[low].is_initialized)
    {
        DERROR("The provided key %llu high: %d , low: %d, doesnt map to a entry. Cannot erase with incorrect id.", key,
               high, low);
        return false;
    }
    if (entry_ptr->unique_identifier != high)
    {
        if (is_non_resizable)
        {
            DFATAL("The provided key %llu high: %d , low: %d, maps to a incorrent entry. You mayhave resized a "
                   "un-resizable array so that the hash codes were changed.",
                   key, high, low);
        }
        else
        {
            DFATAL(
                "The provided key %llu high: %d , low: %d, maps to a incorrent entry. Something is really wrong here.",
                key, high, low);
        }
    }

    dset_memory_value(entry_ptr, 0, element_size);
    num_elements_in_table--;
    return true;
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
