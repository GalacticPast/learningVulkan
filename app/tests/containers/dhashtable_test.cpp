#include "dhashtable_test.hpp"
#include "../tests/expect.hpp"
#include "containers/dhashtable.hpp"
#include "core/dstring.hpp"
#include "math/dmath.hpp"
#include "platform/platform.hpp"

static const char ascii_chars[95] = {'X', 'r', 'F', 'm', 'S', '{', 'K',  '5', 'T', 'B', 'A', '7', '>', '\'', 'j', '/',
                                     'p', '3', '=', '^', 'W', '&', '|',  'd', '}', 'C', '@', 'h', 'n', 'G',  '0', 'U',
                                     '2', '!', 'Z', 'c', 'g', 'b', 'v',  'J', 'O', 'i', 's', 'N', 'L', '#',  '(', 'a',
                                     'x', '9', 'e', 'D', 't', '~', 'R',  'M', 'H', '6', 'E', '.', 'V', 'l',  '1', 'z',
                                     'w', 'f', 'q', 'I', '<', 'y', 'Q',  'P', 'Y', 'u', 'k', 'o', '8', '4',  '"', ',',
                                     '}', ']', '_', '+', '(', ':', '\\', '`', '*', ' ', ';', '-', '$', '[',  '?'};

struct test_struct
{
    char *key;
    u32   value;
};

bool hashtable_create_and_destroy()
{
    dhashtable<u32> num_table;

    u64             size     = 64 * 1024 * 1024;
    u64             capacity = size * sizeof(u64);
    dhashtable<u64> int_table(size);

    expect_should_be(20, num_table.size());
    expect_should_be(size, int_table.size());
    expect_should_be(capacity, int_table.max_capacity());

    return true;
}

void get_random_string(dstring *out_string)
{
    char key[MAX_KEY_LENGTH] = {};
    u32  index;

    for (u32 i = 0; i < MAX_KEY_LENGTH - 1; i++)
    {
        index  = drandom_in_range(0, 94);
        key[i] = ascii_chars[index];
    }
    key[MAX_KEY_LENGTH - 1] = '\0';

    *out_string = key;

    return;
}

bool hashtable_insert_and_find()
{
    u64             size     = 4096;
    u64             capacity = size * sizeof(u64);
    dhashtable<u64> int_table(size);

    darray<dstring> keys(size);

    for (u64 i = 0; i < size; i++)
    {
        dstring *key = &keys[i];
        get_random_string(key);
    }

    for (u64 i = 0; i < size / 2; i++)
    {
        dstring *key = &keys[i];
        int_table.insert(key->c_str(), i);
    }

    for (u64 i = 0; i < size / 2; i++)
    {
        dstring *key   = &keys[i];
        u64      value = int_table.find(key->c_str());
        expect_should_be(i, value);
    }

    return true;
}

bool hashtable_erase_test()
{
    u64             size     = 4096;
    u64             capacity = size * sizeof(u64);
    dhashtable<u64> int_table(size);

    darray<dstring> keys(size);

    for (u64 i = 0; i < size; i++)
    {
        dstring *key = &keys[i];
        get_random_string(key);
    }

    dstring *key;
    for (u64 i = 0; i < size; i++)
    {
        key = &keys[i];
        int_table.insert(key->c_str(), i);
    }

    for (u64 i = 0; i < size; i++)
    {
        u64 value = int_table.find(keys[i].c_str());
        expect_should_be(i, value);
    }
    key = &keys[10];
    int_table.erase(key->c_str());

    u64 value = 0;
    expect_should_be(value, int_table.find(key->c_str()));

    return true;
}

void dhashtable_register_tests()
{
    test_manager_register_tests(hashtable_create_and_destroy, "hashtable create and destroy");
    test_manager_register_tests(hashtable_insert_and_find, "hashtable insert and findtest");
    test_manager_register_tests(hashtable_erase_test, "hashtable erase test");
}
