#pragma once
#include "core/dmemory.hpp"
#include "defines.hpp"
#include <cmath>
namespace math
{
struct vec_2d
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

    vec_2d() : x(0), y(0) {};
    vec_2d(f32 x, f32 y) : x(x), y(y) {};

    inline void operator+=(const vec_2d &vec)
    {
        this->x += vec.x;
        this->y += vec.y;
    }
    inline void operator-=(const vec_2d &vec)
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
    inline vec_2d operator+(const vec_2d &vec)
    {
        return vec_2d(this->x + vec.x, this->y + vec.y);
    }
    inline vec_2d operator-(const vec_2d &vec)
    {
        return vec_2d(this->x - vec.x, this->y - vec.y);
    }
    inline vec_2d operator*(const f32 n)
    {
        return vec_2d(this->x * n, this->y * n);
    }
    inline vec_2d operator/(const f32 n)
    {
        return vec_2d(this->x / n, this->y / n);
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
};
} // namespace math

struct vertex
{
    math::vec3   position;
    math::vec3   normal;
    math::vec_2d tex_coord;
};
