#pragma once
#include "core/dasserts.hpp"
#include "core/dmemory.hpp"
#include "core/logger.hpp"
#include "defines.hpp"

#define DEFAULT_DARRAY_SIZE 10
#define DEFAULT_DARRAY_RESIZE_FACTOR 2

template <typename Type> class darray
{
  private:
    u64 capacity;
    u64 element_size;
    u64 length;

  public:
    void *array;

    darray();
    darray(u64 size);
    ~darray();

    // returns a copy of the value
    Type operator[](u64 index) const;

    Type &operator[](u64 index);

    u64  size();
    void push_back(Type a);
    Type pop_back();
    Type pop_at(u32 index);
};

template <typename T> darray<T>::darray(u64 size)
{
    element_size = sizeof(T);
    capacity     = size * element_size;
    length       = size;
    array        = dallocate(capacity, MEM_TAG_DARRAY);
}
template <typename T> darray<T>::~darray()
{
    dfree(array, element_size * capacity, MEM_TAG_DARRAY);
    capacity     = 0;
    element_size = 0;
    length       = 0;
    array        = 0;
}

template <typename T> darray<T>::darray()
{
    element_size = sizeof(T);
    capacity     = DEFAULT_DARRAY_SIZE * element_size;
    length       = 0;
    array        = dallocate(capacity, MEM_TAG_DARRAY);
}
template <typename T> T darray<T>::operator[](u64 index) const
{
    if (index >= length || index >= capacity)
    {
        DASSERT_MSG(index >= length, "Index is out of bounds....");
    }
    T *elem = (T *)((u8 *)array + (index * element_size));
    return *elem;
}

template <typename T> T &darray<T>::operator[](u64 index)
{
    if (index >= length || index >= capacity)
    {
        DASSERT_MSG(index < length, "Index is out of bounds....");
    }
    T *elem = ((T *)((u8 *)array + (index * element_size)));
    return *elem;
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

        dcopy_memory(buffer, array, capacity);
        dfree(array, capacity, MEM_TAG_DARRAY);

        capacity = capacity * DEFAULT_DARRAY_RESIZE_FACTOR;
        array    = buffer;
        buffer   = 0;
    }

    void *last_index = (void *)((u8 *)array + (length * element_size));
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

    T *return_element = (T *)((u8 *)array + ((length - 1) * element_size));
    length--;
    return *return_element;
}

template <typename T> T darray<T>::pop_at(u32 index)
{
    if (index >= length || index >= capacity)
    {
        DASSERT_MSG(index > length, "Index is out of bounds....");
    }
    void *return_element      = ((u8 *)array + ((index)*element_size));
    T     return_element_copy = *(T *)return_element;
    if (index == length - 1)
    {
        length--;
        return return_element_copy;
    }
    if (index == 0)
    {
        dcopy_memory(array, (u8 *)return_element + element_size, (length - 1) * element_size);
        length--;
        return return_element_copy;
    }

    T *next_element = (T *)((u8 *)array + ((index + 1) * element_size));

    void *new_array = dallocate(capacity, MEM_TAG_DARRAY);
    dcopy_memory(new_array, array, index * element_size);
    dcopy_memory(((u8 *)new_array + (index * element_size)), next_element, (length - index) * element_size);

    dfree(array, capacity, MEM_TAG_RENDERER);
    array = new_array;

    length--;

    return return_element_copy;
}
