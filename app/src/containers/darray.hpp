
#pragma once
#include "core/dasserts.hpp"
#include "core/dmemory.hpp"
#include "core/logger.hpp"
#include "memory/arenas.hpp"

#include "defines.hpp"

#define DEFAULT_DARRAY_SIZE 10
#define DEFAULT_DARRAY_RESIZE_FACTOR 2

template <typename T> class darray
{
  public:
    u64    capacity;
    u64    element_size;
    u64    length;
    arena *arena = nullptr;

    T *data;
    // this actually does nothing, this is just here so that the compiler doesnt complain about it
    darray();
    ~darray();

    // returns a copy of the value
    T    operator[](u64 index) const;
    void operator=(const darray<T> &in_darray);

    T &operator[](u64 index);

    // c_init will make the array length to 0 so this is for pushing back
    // call this wehn you are pushing stuff into the array
    void c_init(struct arena *arena);
    void c_init(struct arena *arena, u64 size);

    // resrve will reserve the speciefied size
    // call this for when you want to index into the array like [] this
    void reserve(struct arena *arena, u64 size);
    void reserve(struct arena *arena);

    void resize(u64 size);

    void push_back(T a);
    T    pop_back();
    T    pop_at(u32 index);
    u64  size();
    void clear();
};

template <typename T> darray<T>::darray()
{
    element_size = sizeof(T);
    capacity     = 0;
    length       = 0;
    data         = 0;
    arena        = nullptr;
    DTRACE("This doesnt do anything, this should not be called by anyone except the compiler.");
}
template <typename T> darray<T>::~darray()
{
    element_size = 0;
    capacity     = 0;
    length       = 0;
    data         = 0;
    arena        = nullptr;
    DTRACE("This doesnt do anything, this should not be called by anyone except the compiler.");
}

template <typename T> void darray<T>::c_init(struct arena *in_arena)
{
    if (data)
    {
        DASSERT_MSG(!data, "THE ARRAY HAS BEEN ALREADY INITIALIZED.");
    }
    element_size = sizeof(T);
    capacity     = DEFAULT_DARRAY_SIZE * element_size;
    length       = 0;
    arena        = in_arena;
    data         = static_cast<T *>(dallocate(arena, capacity, MEM_TAG_DARRAY));
}

template <typename T> void darray<T>::reserve(struct arena *in_arena, u64 size)
{
    if (data)
    {
        DASSERT_MSG(!data, "THE ARRAY HAS BEEN ALREADY INITIALIZED.");
    }
    element_size = sizeof(T);
    capacity     = size * element_size;
    if (capacity < DEFAULT_DARRAY_SIZE * element_size)
    {
        capacity = DEFAULT_DARRAY_SIZE * element_size;
    }
    length = size;
    arena  = in_arena;
    data   = static_cast<T *>(dallocate(arena, capacity, MEM_TAG_DARRAY));
}

template <typename T> void darray<T>::reserve(struct arena *in_arena)
{
    if (data)
    {
        DASSERT_MSG(!data, "THE ARRAY HAS BEEN ALREADY INITIALIZED.");
    }
    element_size = sizeof(T);
    capacity     = element_size;
    if (capacity < DEFAULT_DARRAY_SIZE * element_size)
    {
        capacity = DEFAULT_DARRAY_SIZE * element_size;
    }
    length = DEFAULT_DARRAY_SIZE;
    arena  = in_arena;
    data   = static_cast<T *>(dallocate(arena, capacity, MEM_TAG_DARRAY));
}

template <typename T> void darray<T>::c_init(struct arena *in_arena, u64 size)
{
    element_size = sizeof(T);
    capacity     = size * element_size;
    if (capacity < DEFAULT_DARRAY_SIZE * element_size)
    {
        capacity = DEFAULT_DARRAY_SIZE * element_size;
    }
    length = 0;
    arena  = in_arena;
    data   = static_cast<T *>(dallocate(arena, capacity, MEM_TAG_DARRAY));
}

template <typename T> void darray<T>::operator=(const darray<T> &in_darray)
{
    DASSERT_MSG(length == 0,
                "There is still some data in the array, cannot assign another array into a already initialized array.");

    if (data)
    {
        dfree(data, capacity, MEM_TAG_DARRAY);
        capacity = in_darray.capacity;
    }
    arena        = in_darray.arena;

    data = static_cast<T *>(dallocate(arena, in_darray.capacity, MEM_TAG_DARRAY));

    dcopy_memory(data, in_darray.data, in_darray.capacity);
    length       = in_darray.length;
    element_size = in_darray.element_size;
    return;
}
template <typename T> T darray<T>::operator[](u64 index) const
{
    if ((index != 0 && index >= length) || index >= capacity)
    {
        DASSERT_MSG(index >= length, "Index is out of bounds....");
    }
    T *elem = &data[index];
    return *elem;
}

template <typename T> T &darray<T>::operator[](u64 index)
{
    if ((index != 0 && index >= length) || index >= capacity)
    {
        DASSERT_MSG(index < length, "Index is out of bounds....");
    }

    T *elem = &data[index];
    return *elem;
}

template <typename T> void darray<T>::resize(u64 out_size)
{
    if (out_size < DEFAULT_DARRAY_SIZE)
    {
        length = out_size;
        DASSERT_MSG(data, "array hasnt been initalized yet.");
        return;
    }
    u64 new_capacity = (out_size * element_size);
    if (data)
    {
        if (capacity > new_capacity)
        {
            if (data && length > 0)
            {
                DASSERT_MSG(new_capacity > capacity, "Old capacity is bigger than the new capcity");
            }
            DERROR("Resizing array from %ldbytes to smaller %ldBytes", capacity, out_size);
        }
        void *buffer = dallocate(arena, element_size * out_size, MEM_TAG_DARRAY);
        if (!buffer)
        {
            DFATAL("Darray coulnd't allocate block for ::resize()");
        }
        dcopy_memory(buffer, data, new_capacity);
        dfree(data, capacity, MEM_TAG_DARRAY);
        data     = static_cast<T *>(buffer);
        capacity = new_capacity;
        length   = out_size;
    }
    else
    {
        DERROR("Array hasn't been initialized properly. Call c_init, reserve or plain old constructor with arenas to initalize it.");
        debugBreak();
    }
}

template <typename T> void darray<T>::clear()
{
    if (data)
    {
        dzero_memory(data, capacity);
    }
    length = 0;
}

template <typename T> u64 darray<T>::size()
{
    return length;
}

template <typename T> void darray<T>::push_back(T element)
{
    if(data)
    {
        if (length + 1 >= (capacity / element_size))
        {
            DWARN("Array size is not large enough to accomadate another element. Resizing....");
            void *buffer = dallocate(arena, DEFAULT_DARRAY_RESIZE_FACTOR * capacity, MEM_TAG_DARRAY);
            if (!buffer)
            {
                DFATAL("Darray coulnd't allocate block for ::push_back()");
            }

            dcopy_memory(buffer, data, capacity);
            dfree(data, capacity, MEM_TAG_DARRAY);

            capacity = capacity * DEFAULT_DARRAY_RESIZE_FACTOR;
            data     = static_cast<T *>(buffer);
            buffer   = 0;
        }
    }
    else
    {
        DERROR("Array hasn't been initialized properly. Call c_init, reserve or plain old constructor with arenas to initalize it.");
        return;
    }

    T *last_index = &data[length];
    dcopy_memory(last_index, &element, element_size);
    length++;
    last_index = 0;
}
template <typename T> T darray<T>::pop_back()
{
    if (length == 0)
    {
        DASSERT_MSG(length != 0, "No elements in darray to pop back.");
    }

    T *return_element = &data[length - 1];
    length--;
    return *return_element;
}

template <typename T> T darray<T>::pop_at(u32 index)
{
    if (index >= length || index >= capacity)
    {
        DASSERT_MSG(index > length, "Index is out of bounds....");
    }
    T *return_element      = &data[index];
    T  return_element_copy = *return_element;
    if (index == length - 1)
    {
        length--;
        return return_element_copy;
    }
    if (index == 0)
    {
        dcopy_memory(data, &return_element[1], (length - 1) * element_size);
        length--;
        return return_element_copy;
    }

    T *next_element = &data[index + 1];

    T *new_array = static_cast<T *>(dallocate(arena, capacity, MEM_TAG_DARRAY));
    if (!new_array)
    {
        DFATAL("Darray coulnd't allocate block for ::pop_at()");
    }
    dcopy_memory(new_array, data, index * element_size);
    dcopy_memory(&new_array[index], next_element, (length - index) * element_size);

    dfree(data, capacity, MEM_TAG_RENDERER);
    data = static_cast<T *>(new_array);

    length--;

    return return_element_copy;
}
