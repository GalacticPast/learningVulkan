#pragma once
//this algorithm (k=33) was first reported by dan bernstein many years ago in comp.lang.c. another version of this algorithm (now favored by bernstein) uses xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic of number 33 (why it works better than many other constants, prime or not) has never been adequately explained.
//
//    unsigned long
//    hash(unsigned char *str)
//    {
//        unsigned long hash = 5381;
//        int c;
//
//        while (c = *str++)
//            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
//
//        return hash;
//    }
//

#define MAX_KEY_LENGTH 64
#define DEFAULT_HASH_TABLE_SIZE 20 

template <const char* key, typename value>class dhashtable
{
    public:
        u64 capacity;
        u64 element_size;

}; 
