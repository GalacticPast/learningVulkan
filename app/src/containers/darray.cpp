#include "darray.hpp"
#include "core/dasserts.hpp"
#include "core/dmemory.hpp"

template <typename Type> darray<Type>::darray(u64 size)
{
    element_size = sizeof(Type);
    capacity     = size * element_size;
    length       = size;
    array        = dallocate(element_size * capacity, MEM_TAG_DARRAY);
}
template <typename Type> darray<Type>::~darray()
{
    dfree(array, element_size * capacity, MEM_TAG_DARRAY);
    capacity     = 0;
    element_size = 0;
    length       = 0;
    array        = 0;
}

template <typename Type> darray<Type>::darray()
{
    element_size = sizeof(Type);
    capacity     = DEFAULT_DARRAY_SIZE * element_size;
    length       = 0;
    array        = dallocate(capacity, MEM_TAG_DARRAY);
}
template <typename Type> Type darray<Type>::operator[](u64 index) const
{
    if (index >= length || index >= capacity)
    {
        DASSERT_MSG(index >= length, "Index is out of bounds....");
    }
    Type *elem = (Type *)((u8 *)array + (index * element_size));
    return *elem;
}

template <typename Type> Type &darray<Type>::operator[](u64 index)
{
    if (index >= length || index >= capacity)
    {
        DASSERT_MSG(index >= length, "Index is out of bounds....");
    }
    Type *elem = ((Type *)((u8 *)array + (index * element_size)));
    return *elem;
}

template <typename Type> u64 darray<Type>::size()
{
    return length;
}

template <typename Type> void darray<Type>::push_back(Type element)
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
template <typename Type> Type darray<Type>::pop_back()
{
    if (length == 0)
    {
        DWARN("No element in array to pop back. Returning...");
        return nullptr;
    }

    Type return_element = (u8 *)array + (length * element_size);
    length--;
    return return_element;
}

template <typename Type> Type darray<Type>::pop_at(u32 index)
{
    if (index > length || index > capacity)
    {
        DASSERT_MSG(index > length, "Index is out of bounds....");
    }
    Type  return_element_cpy = (u8 *)array + (index * element_size);
    Type *return_element_ptr = (u8 *)array + (index * element_size);
    Type *next_element       = return_element_ptr + (1 * element_size);

    void *buffer             = dallocate(element_size * (capacity * DEFAULT_DARRAY_RESIZE_FACTOR), MEM_TAG_DARRAY);
    dcopy_memory(buffer, array, index * element_size);
    dcopy_memory((u8 *)buffer + (index + 2) * element_size, next_element, (index + 1) * element_size);
    dfree(array, capacity * element_size);
    array  = buffer;
    buffer = 0;

    return return_element_cpy;
}
