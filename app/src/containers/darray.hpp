#pragma once
#include "core/dasserts.hpp"
#include "core/dmemory.hpp"
#include "core/logger.hpp"
#include "defines.hpp"

#define DEFAULT_DARRAY_SIZE 1
#define DEFAULT_DARRAY_RESIZE_FACTOR 2

template <typename T> class darray
{
  public:
    u64 capacity;
    u64 element_size;
    u64 length;

    void *data;

    darray();
    darray(u64 size);
    ~darray();

    // returns a copy of the value
    T    operator[](u64 index) const;
    void operator=(darray<T> &in_darray);

    T &operator[](u64 index);

    u64  size();
    void resize(u64 size);
    void push_back(T a);
    T    pop_back();
    T    pop_at(u32 index);
};

template <typename T> darray<T>::darray(u64 size)
{
    element_size = sizeof(T);
    capacity     = size * element_size;
    if (capacity < DEFAULT_DARRAY_SIZE * element_size)
    {
        capacity = DEFAULT_DARRAY_SIZE * element_size;
    }
    length = size;
    data   = dallocate(capacity, MEM_TAG_DARRAY);
}
template <typename T> darray<T>::darray()
{
    element_size = sizeof(T);
    capacity     = DEFAULT_DARRAY_SIZE * element_size;
    length       = 0;
    data         = dallocate(capacity, MEM_TAG_DARRAY);
}
template <typename T> darray<T>::~darray()
{
    dfree(data, capacity, MEM_TAG_DARRAY);
    capacity     = 0;
    element_size = 0;
    length       = 0;
    data         = 0;
}

template <typename T> void darray<T>::operator=(darray<T> &in_darray)
{
    if (capacity < in_darray.capacity)
    {
        DASSERT_MSG(
            length == 0,
            "There is still some data in the array, cannot assign another array into a already initialized array.");
        if (data)
        {
            dfree(data, capacity, MEM_TAG_DARRAY);
        }
        data     = dallocate(in_darray.capacity, MEM_TAG_DARRAY);
        capacity = in_darray.capacity;
    }
    dcopy_memory(data, in_darray.data, in_darray.capacity);
    length       = in_darray.length;
    element_size = in_darray.element_size;
    return;
}
template <typename T> T darray<T>::operator[](u64 index) const
{
    if (index >= length || index >= capacity)
    {
        DASSERT_MSG(index >= length, "Index is out of bounds....");
    }
    T *elem = (T *)((u8 *)data + (index * element_size));
    return *elem;
}

template <typename T> T &darray<T>::operator[](u64 index)
{
    if (index >= length || index >= capacity)
    {
        DASSERT_MSG(index < length, "Index is out of bounds....");
    }
    T *elem = ((T *)((u8 *)data + (index * element_size)));
    return *elem;
}

template <typename T> void darray<T>::resize(u64 out_size)
{
    u64 new_capacity = (out_size * element_size);
    if (capacity > new_capacity)
    {
        if (data && length > 0)
        {
            DASSERT_MSG(new_capacity > capacity, "Old capacity is bigger than the new capcity");
        }
        DERROR("Resizing array from %ldbytes to smaller %ldBytes", capacity, out_size);
    }
    if (data)
    {
        void *buffer = dallocate(element_size * out_size, MEM_TAG_DARRAY);
        dcopy_memory(buffer, data, capacity);
        dfree(data, capacity, MEM_TAG_DARRAY);
        data = buffer;
    }
    else
    {
        data = dallocate(element_size * out_size, MEM_TAG_DARRAY);
    }
    capacity = element_size * out_size;
    length   = out_size;
}

template <typename T> u64 darray<T>::size()
{
    return length;
}

template <typename T> void darray<T>::push_back(T element)
{
    if (length + 1 >= (capacity / element_size))
    {
        DWARN("Array size is not large enough to accomadate another element. Resizing....");
        void *buffer = dallocate(DEFAULT_DARRAY_RESIZE_FACTOR * capacity, MEM_TAG_DARRAY);

        dcopy_memory(buffer, data, capacity);
        dfree(data, capacity, MEM_TAG_DARRAY);

        capacity = capacity * DEFAULT_DARRAY_RESIZE_FACTOR;
        data     = buffer;
        buffer   = 0;
    }

    void *last_index = (void *)((u8 *)data + (length * element_size));
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

    T *return_element = (T *)((u8 *)data + ((length - 1) * element_size));
    length--;
    return *return_element;
}

template <typename T> T darray<T>::pop_at(u32 index)
{
    if (index >= length || index >= capacity)
    {
        DASSERT_MSG(index > length, "Index is out of bounds....");
    }
    void *return_element      = ((u8 *)data + ((index)*element_size));
    T     return_element_copy = *(T *)return_element;
    if (index == length - 1)
    {
        length--;
        return return_element_copy;
    }
    if (index == 0)
    {
        dcopy_memory(data, (u8 *)return_element + element_size, (length - 1) * element_size);
        length--;
        return return_element_copy;
    }

    T *next_element = (T *)((u8 *)data + ((index + 1) * element_size));

    void *new_array = dallocate(capacity, MEM_TAG_DARRAY);
    dcopy_memory(new_array, data, index * element_size);
    dcopy_memory(((u8 *)new_array + (index * element_size)), next_element, (length - index) * element_size);

    dfree(data, capacity, MEM_TAG_RENDERER);
    data = new_array;

    length--;

    return return_element_copy;
}
