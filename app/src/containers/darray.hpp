#pragma once
#include "defines.hpp"

#define DEFAULT_DARRAY_SIZE 10
#define DEFAULT_DARRAY_RESIZE_FACTOR 2

template <typename Type> class darray
{
  private:
    u64 capacity;
    u64 element_size;
    u64 length;

    void *array;

  public:
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
