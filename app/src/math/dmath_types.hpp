#pragma once
#include "core/dmemory.hpp"
#include "defines.hpp"
#include <cmath>

struct vec2
{
    union {
        f32 elements[2];
        struct
        {
            union {
                // The first element.
                f32 x, r, s, u;
            };
            union {
                // The second element.
                f32 y, g, t, v;
            };
        };
    };

    vec2() : x(0), y(0) {};
    vec2(f32 x, f32 y) : x(x), y(y) {};

    inline void operator+=(const vec2 &vec)
    {
        this->x += vec.x;
        this->y += vec.y;
    }
    inline void operator-=(const vec2 &vec)
    {
        this->x -= vec.x;
        this->y -= vec.y;
    }
    inline void operator*=(const f32 n)
    {
        this->x *= n;
        this->y *= n;
    }
    inline void operator/=(const f32 n)
    {
        this->x /= n;
        this->y /= n;
    }
    inline vec2 operator+(const vec2 &vec)
    {
        return vec2(this->x + vec.x, this->y + vec.y);
    }
    inline vec2 operator-(const vec2 &vec)
    {
        return vec2(this->x - vec.x, this->y - vec.y);
    }
    inline vec2 operator*(const f32 n)
    {
        return vec2(this->x * n, this->y * n);
    }
    inline vec2 operator/(const f32 n)
    {
        return vec2(this->x / n, this->y / n);
    }
    inline f32 magnitude()
    {
        return sqrtf(this->x * this->x + this->y * this->y);
    }
    inline void normalize()
    {
        const f32 length  = this->magnitude();
        this->x          /= length;
        this->y          /= length;
    }
};

struct vec3
{
    // An array of x, y, z
    union {
        f32 elements[3];
        struct
        {
            union {
                // The first element.
                f32 x, r, s, u;
            };
            union {
                // The second element.
                f32 y, g, t, v;
            };
            union {
                // The third element.
                f32 z, b, p, w;
            };
        };
    };

    vec3() : x(0), y(0), z(0) {};
    vec3(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {};

    inline vec3 operator+(const vec3 &vec)
    {
        return vec3(this->x + vec.x, this->y + vec.y, this->z + vec.z);
    }
    inline vec3 operator-(const vec3 &vec)
    {
        return vec3(this->x - vec.x, this->y - vec.y, this->z - vec.z);
    }
    inline vec3 operator*(const f32 n)
    {
        return vec3(this->x * n, this->y * n, this->z * n);
    }
    inline vec3 operator/(const f32 n)
    {
        return vec3(this->x / n, this->y / n, this->z / n);
    }
    inline void operator+=(const vec3 &vec)
    {
        this->x += vec.x;
        this->y += vec.y;
        this->z += vec.z;
    }
    inline void operator-=(const f32 n)
    {
        this->x -= n;
        this->y -= n;
        this->z -= n;
    }
    inline void operator-=(const vec3 &vec)
    {
        this->x -= vec.x;
        this->y -= vec.y;
        this->z -= vec.z;
    }
    inline void operator*=(const f32 n)
    {
        this->x *= n;
        this->y *= n;
        this->z *= n;
    }
    inline void operator/=(const f32 n)
    {
        this->x /= n;
        this->y /= n;
        this->z /= n;
    }
    inline f32 magnitude()
    {
        return sqrtf(this->x * this->x + this->y * this->y + this->z * this->z);
    }
    inline void normalize()
    {
        const f32 length  = this->magnitude();
        this->x          /= length;
        this->y          /= length;
        this->z          /= length;
    }
    // return true if 'this' vector is bigger
};

struct vec4
{
    union {
        // An array of x, y, z, w
        f32 elements[4];
        struct
        {
            union {
                // The first element.
                f32 x, r, s;
            };
            union {
                // The second element.
                f32 y, g, t;
            };
            union {
                // The third element.
                f32 z, b, p;
            };
            union {
                // The fourth element.
                f32 w, a, q;
            };
        };
    };

    vec4() : x(0), y(0), z(0), w(0) {};
    vec4(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) {};

    inline vec4 operator+(const vec4 &vec)
    {
        return vec4(this->x + vec.x, this->y + vec.y, this->z + vec.z, this->w + vec.w);
    }
    inline vec4 operator-(const vec4 &vec)
    {
        return vec4(this->x - vec.x, this->y - vec.y, this->z - vec.z, this->w - vec.w);
    }
    inline vec4 operator*(const f32 n)
    {
        return vec4(this->x * n, this->y * n, this->z * n, this->w * n);
    }
    inline vec4 operator/(const f32 n)
    {
        return vec4(this->x / n, this->y / n, this->z / n, this->w / n);
    }
    inline void operator+=(const vec4 &vec)
    {
        this->x += vec.x;
        this->y += vec.y;
        this->z += vec.z;
        this->w += vec.w;
    }
    inline void operator-=(const vec4 &vec)
    {
        this->x -= vec.x;
        this->y -= vec.y;
        this->z -= vec.z;
        this->w -= vec.w;
    }
    inline void operator*=(const f32 n)
    {
        this->x *= n;
        this->y *= n;
        this->z *= n;
        this->w *= n;
    }
    inline void operator/=(const f32 n)
    {
        this->x /= n;
        this->y /= n;
        this->z /= n;
        this->w /= n;
    }
    inline f32 magnitude()
    {
        return sqrtf(this->x * this->x + this->y * this->y + this->z * this->z + this->w * this->w);
    }
    inline void normalize()
    {
        const f32 length  = this->magnitude();
        this->x          /= length;
        this->y          /= length;
        this->z          /= length;
        this->w          /= length;
    }
};
struct mat4
{
    f32 data[16];

    mat4()
    {
        dzero_memory(this->data, sizeof(f32) * 16);
        data[0]  = 1.0f;
        data[5]  = 1.0f;
        data[10] = 1.0f;
        data[15] = 1.0f;
    }

    inline mat4 operator+(mat4 matrix_1)
    {
        mat4 out_matrix = mat4();

        f32       *dst_ptr = out_matrix.data;
        const f32 *m1_ptr  = this->data;
        const f32 *m2_ptr  = matrix_1.data;

        for (s32 i = 0; i < 16; i++)
        {
            dst_ptr[i] = m1_ptr[i] + m2_ptr[i];
        }
        return out_matrix;
    }
    inline void operator+=(mat4 matrix_1)
    {
        f32       *m1_ptr = this->data;
        const f32 *m2_ptr = matrix_1.data;

        for (s32 i = 0; i < 16; i++)
        {
            m1_ptr[i] += m2_ptr[i];
        }
    };
    inline mat4 operator*(mat4 matrix_1)
    {
        mat4 out_matrix = mat4();

        const f32 *m1_ptr  = this->data;
        const f32 *m2_ptr  = matrix_1.data;
        f32       *dst_ptr = out_matrix.data;

        for (s32 i = 0; i < 4; ++i)
        {
            for (s32 j = 0; j < 4; ++j)
            {
                *dst_ptr = m1_ptr[0] * m2_ptr[0 + j] + m1_ptr[1] * m2_ptr[4 + j] + m1_ptr[2] * m2_ptr[8 + j] +
                           m1_ptr[3] * m2_ptr[12 + j];
                dst_ptr++;
            }
            m1_ptr += 4;
        }
        return out_matrix;
    }
    inline void operator*=(mat4 matrix_1)
    {
        mat4 out_matrix = mat4();

        const f32 *m1_ptr  = this->data;
        const f32 *m2_ptr  = matrix_1.data;
        f32       *dst_ptr = out_matrix.data;

        for (s32 i = 0; i < 4; ++i)
        {
            for (s32 j = 0; j < 4; ++j)
            {
                *dst_ptr = m1_ptr[0] * m2_ptr[0 + j] + m1_ptr[1] * m2_ptr[4 + j] + m1_ptr[2] * m2_ptr[8 + j] +
                           m1_ptr[3] * m2_ptr[12 + j];
                dst_ptr++;
            }
            m1_ptr += 4;
        }
        dcopy_memory(this->data, out_matrix.data, sizeof(f32) * 16);
    }
};

struct vertex_3D
{
    vec3 position;
    vec3 normal;
    vec2 tex_coord;
    vec4 tangent;
};

struct camera
{
    vec3 euler;
    vec3 position;
    vec3 up = vec3(0, 1, 0);
};

struct scene_global_uniform_buffer_object
{
    mat4 view;
    mat4 projection;
    vec4 ambient_color;
    alignas(16) vec3 camera_pos;
};
struct light_global_uniform_buffer_object
{
    alignas(16) vec3 direction;
    alignas(16) vec4 color;
};

struct object_uniform_buffer_object
{
    mat4 model;
};

struct vk_push_constant // aka push constants
{
    mat4 model; // 64 bytes
    vec4 diffuse_color;
    vec4 padding;
    vec4 padding2;
    vec4 padding3;
};
