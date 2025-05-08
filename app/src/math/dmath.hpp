#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
// HACK:
#pragma once
#include "core/dmemory.hpp"
#include "defines.hpp"
#include "dmath_types.hpp"
#include <cmath>

#define D_PI 3.14159265358979323846f
#define D_PI_2 2.0f * D_PI
#define D_HALF_PI 0.5f * D_PI
#define D_QUARTER_PI 0.25f * D_PI
#define D_ONE_OVER_PI 1.0f / D_PI
#define D_ONE_OVER_TWO_PI 1.0f / D_PI_2
#define D_SQRT_TWO 1.41421356237309504880f
#define D_SQRT_THREE 1.73205080756887729352f
#define D_SQRT_ONE_OVER_TWO 0.70710678118654752440f
#define D_SQRT_ONE_OVER_THREE 0.57735026918962576450f
#define D_DEG2RAD_MULTIPLIER D_PI / 180.0f
#define D_RAD2DEG_MULTIPLIER 180.0f / D_PI

// The multiplier to convert seconds to milliseconds.
#define D_SEC_TO_MS_MULTIPLIER 1000.0f

// The multiplier to convert milliseconds to seconds.
#define D_MS_TO_SEC_MULTIPLIER 0.001f

// A huge number that should be larger than any valid number used.
#define D_INFINITY 1e30f

// Smallest positive number where 1.0 + FLOAT_EPSILON != 0
#define D_FLOAT_EPSILON 1.192092896e-07f

inline vec2 vec2_normalized(vec2 a)
{
    const f32 length = a.magnitude();
    f32       x      = a.x / length;
    f32       y      = a.y / length;
    return vec2(x, y);
}
/**
 * @brief Compares all elements of vector_0 and vector_1 and ensures the difference
 * is less than tolerance.
 *
 * @param vector_0 The first vector.
 * @param vector_1 The second vector.
 * @param tolerance The difference tolerance. Typically K_FLOAT_EPSILON or similar. @return True if within tolerance;
 * otherwise false. */
inline bool vec2_compare(vec2 vector_0, vec2 vector_1, f32 tolerance)
{
    if (fabs(vector_0.x - vector_1.x) > tolerance)
    {
        return false;
    }

    if (fabs(vector_0.y - vector_1.y) > tolerance)
    {
        return false;
    }

    return true;
}

inline vec3 vec3_normalized(vec3 a)
{
    const f32 length = a.magnitude();
    f32       x      = a.x / length;
    f32       y      = a.y / length;
    f32       z      = a.z / length;
    return vec3(x, y, z);
}
inline f32 vec3_dot(vec3 vector_0, vec3 vector_1)
{
    f32 p  = 0;
    p     += vector_0.x * vector_1.x;
    p     += vector_0.y * vector_1.y;
    p     += vector_0.z * vector_1.z;
    return p;
}
inline vec3 vec3_cross(vec3 vector_0, vec3 vector_1)
{
    return (vec3){vector_0.y * vector_1.z - vector_0.z * vector_1.y, vector_0.z * vector_1.x - vector_0.x * vector_1.z,
                  vector_0.x * vector_1.y - vector_0.y * vector_1.x};
}

inline const bool vec3_compare(vec3 vector_0, vec3 vector_1, f32 tolerance)
{
    if (fabs(vector_0.x - vector_1.x) > tolerance)
    {
        return false;
    }

    if (fabs(vector_0.y - vector_1.y) > tolerance)
    {
        return false;
    }

    if (fabs(vector_0.z - vector_1.z) > tolerance)
    {
        return false;
    }

    return true;
}
inline f32 vec3_distance(vec3 vector_0, vec3 vector_1)
{
    vec3 d = (vec3){vector_0.x - vector_1.x, vector_0.y - vector_1.y, vector_0.z - vector_1.z};
    return d.magnitude();
}

inline vec4 vec4_normalized(vec4 a)
{
    const f32 length = a.magnitude();
    f32       x      = a.x / length;
    f32       y      = a.y / length;
    f32       z      = a.z / length;
    f32       w      = a.w / length;
    return vec4(x, y, z, w);
}
inline f32 vec4_dot_f32(vec4 a, vec4 b)
{
    f32 p = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    return p;
}

inline mat4 mat4_mul(mat4 matrix_0, mat4 matrix_1)
{
    mat4 out_matrix = mat4();

    const f32 *m1_ptr  = matrix_0.data;
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

inline mat4 mat4_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near_clip, f32 far_clip)
{
    mat4 out_matrix = mat4();

    f32 lr = 1.0f / (left - right);
    f32 bt = 1.0f / (bottom - top);
    f32 nf = 1.0f / (near_clip - far_clip);

    out_matrix.data[0]  = -2.0f * lr;
    out_matrix.data[5]  = -2.0f * bt;
    out_matrix.data[10] = 2.0f * nf;

    out_matrix.data[12] = (left + right) * lr;
    out_matrix.data[13] = (top + bottom) * bt;
    out_matrix.data[14] = (far_clip + near_clip) * nf;
    return out_matrix;
}

/**
 * @brief Creates and returns a perspective matrix. Typically used to render 3d scenes.
 *
 * @param fov_radians The field of view in radians.
 * @param aspect_ratio The aspect ratio.
 * @param near_clip The near clipping plane distance.
 * @param far_clip The far clipping plane distance.
 * @return A new perspective matrix.
 */
inline mat4 mat4_perspective(f32 fov_radians, f32 aspect_ratio, f32 near_clip, f32 far_clip)
{
    f32  half_tan_fov = tanf(fov_radians * 0.5f);
    mat4 out_matrix;
    dzero_memory(out_matrix.data, sizeof(f32) * 16);
    out_matrix.data[0]  = 1.0f / (aspect_ratio * half_tan_fov);
    out_matrix.data[5]  = -(1.0f / half_tan_fov);
    out_matrix.data[10] = -((far_clip + near_clip) / (far_clip - near_clip));
    out_matrix.data[11] = -1.0f;
    out_matrix.data[14] = -((2.0f * far_clip * near_clip) / (far_clip - near_clip));
    return out_matrix;
}

/**
 * @brief Creates and returns a look-at matrix, or a matrix looking
 * at target from the perspective of position.
 *
 * @param position The position of the matrix.
 * @param target The position to "look at".
 * @param up The up vector.
 * @return A matrix looking at target from the perspective of position.
 */
inline mat4 mat4_look_at(vec3 position, vec3 target, vec3 up)
{
    mat4 out_matrix;
    vec3 z_axis;
    z_axis.x = target.x - position.x;
    z_axis.y = target.y - position.y;
    z_axis.z = target.z - position.z;

    z_axis.normalize();
    vec3 x_axis = vec3_normalized(vec3_cross(z_axis, up));
    vec3 y_axis = vec3_cross(x_axis, z_axis);

    out_matrix.data[0]  = x_axis.x;
    out_matrix.data[1]  = y_axis.x;
    out_matrix.data[2]  = -z_axis.x;
    out_matrix.data[3]  = 0;
    out_matrix.data[4]  = x_axis.y;
    out_matrix.data[5]  = y_axis.y;
    out_matrix.data[6]  = -z_axis.y;
    out_matrix.data[7]  = 0;
    out_matrix.data[8]  = x_axis.z;
    out_matrix.data[9]  = y_axis.z;
    out_matrix.data[10] = -z_axis.z;
    out_matrix.data[11] = 0;
    out_matrix.data[12] = -vec3_dot(x_axis, position);
    out_matrix.data[13] = -vec3_dot(y_axis, position);
    out_matrix.data[14] = vec3_dot(z_axis, position);
    out_matrix.data[15] = 1.0f;

    return out_matrix;
}

/**
 * @brief Returns a transposed copy of the provided matrix (rows->colums)
 *
 * @param matrix The matrix to be transposed.
 * @return A transposed copy of of the provided matrix.
 */
inline mat4 mat4_transposed(mat4 matrix)
{
    mat4 out_matrix     = mat4();
    out_matrix.data[0]  = matrix.data[0];
    out_matrix.data[1]  = matrix.data[4];
    out_matrix.data[2]  = matrix.data[8];
    out_matrix.data[3]  = matrix.data[12];
    out_matrix.data[4]  = matrix.data[1];
    out_matrix.data[5]  = matrix.data[5];
    out_matrix.data[6]  = matrix.data[9];
    out_matrix.data[7]  = matrix.data[13];
    out_matrix.data[8]  = matrix.data[2];
    out_matrix.data[9]  = matrix.data[6];
    out_matrix.data[10] = matrix.data[10];
    out_matrix.data[11] = matrix.data[14];
    out_matrix.data[12] = matrix.data[3];
    out_matrix.data[13] = matrix.data[7];
    out_matrix.data[14] = matrix.data[11];
    out_matrix.data[15] = matrix.data[15];
    return out_matrix;
}

/**
 * @brief Creates and returns an inverse of the provided matrix.
 *
 * @param matrix The matrix to be inverted.
 * @return A inverted copy of the provided matrix.
 */
inline mat4 mat4_inverse(mat4 matrix)
{
    const f32 *m = matrix.data;

    f32 t0  = m[10] * m[15];
    f32 t1  = m[14] * m[11];
    f32 t2  = m[6] * m[15];
    f32 t3  = m[14] * m[7];
    f32 t4  = m[6] * m[11];
    f32 t5  = m[10] * m[7];
    f32 t6  = m[2] * m[15];
    f32 t7  = m[14] * m[3];
    f32 t8  = m[2] * m[11];
    f32 t9  = m[10] * m[3];
    f32 t10 = m[2] * m[7];
    f32 t11 = m[6] * m[3];
    f32 t12 = m[8] * m[13];
    f32 t13 = m[12] * m[9];
    f32 t14 = m[4] * m[13];
    f32 t15 = m[12] * m[5];
    f32 t16 = m[4] * m[9];
    f32 t17 = m[8] * m[5];
    f32 t18 = m[0] * m[13];
    f32 t19 = m[12] * m[1];
    f32 t20 = m[0] * m[9];
    f32 t21 = m[8] * m[1];
    f32 t22 = m[0] * m[5];
    f32 t23 = m[4] * m[1];

    mat4 out_matrix;
    f32 *o = out_matrix.data;

    o[0] = (t0 * m[5] + t3 * m[9] + t4 * m[13]) - (t1 * m[5] + t2 * m[9] + t5 * m[13]);
    o[1] = (t1 * m[1] + t6 * m[9] + t9 * m[13]) - (t0 * m[1] + t7 * m[9] + t8 * m[13]);
    o[2] = (t2 * m[1] + t7 * m[5] + t10 * m[13]) - (t3 * m[1] + t6 * m[5] + t11 * m[13]);
    o[3] = (t5 * m[1] + t8 * m[5] + t11 * m[9]) - (t4 * m[1] + t9 * m[5] + t10 * m[9]);

    f32 d = 1.0f / (m[0] * o[0] + m[4] * o[1] + m[8] * o[2] + m[12] * o[3]);

    o[0]  = d * o[0];
    o[1]  = d * o[1];
    o[2]  = d * o[2];
    o[3]  = d * o[3];
    o[4]  = d * ((t1 * m[4] + t2 * m[8] + t5 * m[12]) - (t0 * m[4] + t3 * m[8] + t4 * m[12]));
    o[5]  = d * ((t0 * m[0] + t7 * m[8] + t8 * m[12]) - (t1 * m[0] + t6 * m[8] + t9 * m[12]));
    o[6]  = d * ((t3 * m[0] + t6 * m[4] + t11 * m[12]) - (t2 * m[0] + t7 * m[4] + t10 * m[12]));
    o[7]  = d * ((t4 * m[0] + t9 * m[4] + t10 * m[8]) - (t5 * m[0] + t8 * m[4] + t11 * m[8]));
    o[8]  = d * ((t12 * m[7] + t15 * m[11] + t16 * m[15]) - (t13 * m[7] + t14 * m[11] + t17 * m[15]));
    o[9]  = d * ((t13 * m[3] + t18 * m[11] + t21 * m[15]) - (t12 * m[3] + t19 * m[11] + t20 * m[15]));
    o[10] = d * ((t14 * m[3] + t19 * m[7] + t22 * m[15]) - (t15 * m[3] + t18 * m[7] + t23 * m[15]));
    o[11] = d * ((t17 * m[3] + t20 * m[7] + t23 * m[11]) - (t16 * m[3] + t21 * m[7] + t22 * m[11]));
    o[12] = d * ((t14 * m[10] + t17 * m[14] + t13 * m[6]) - (t16 * m[14] + t12 * m[6] + t15 * m[10]));
    o[13] = d * ((t20 * m[14] + t12 * m[2] + t19 * m[10]) - (t18 * m[10] + t21 * m[14] + t13 * m[2]));
    o[14] = d * ((t18 * m[6] + t23 * m[14] + t15 * m[2]) - (t22 * m[14] + t14 * m[2] + t19 * m[6]));
    o[15] = d * ((t22 * m[10] + t16 * m[2] + t21 * m[6]) - (t20 * m[6] + t23 * m[10] + t17 * m[2]));

    return out_matrix;
}

inline mat4 mat4_translation(vec3 position)
{
    mat4 out_matrix     = mat4();
    out_matrix.data[12] = position.x;
    out_matrix.data[13] = position.y;
    out_matrix.data[14] = position.z;
    return out_matrix;
}

/**
 * @brief Returns a scale matrix using the provided scale.
 *
 * @param scale The 3-component scale.
 * @return A scale matrix.
 */
inline mat4 mat4_scale(vec3 scale)
{
    mat4 out_matrix     = mat4();
    out_matrix.data[0]  = scale.x;
    out_matrix.data[5]  = scale.y;
    out_matrix.data[10] = scale.z;
    return out_matrix;
}

inline mat4 mat4_euler_x(f32 angle_radians)
{
    mat4 out_matrix = mat4();
    f32  c          = cosf(angle_radians);
    f32  s          = sinf(angle_radians);

    out_matrix.data[5]  = c;
    out_matrix.data[6]  = s;
    out_matrix.data[9]  = -s;
    out_matrix.data[10] = c;
    return out_matrix;
}
inline mat4 mat4_euler_y(f32 angle_radians)
{
    mat4 out_matrix = mat4();
    f32  c          = cosf(angle_radians);
    f32  s          = sinf(angle_radians);

    out_matrix.data[0]  = c;
    out_matrix.data[2]  = -s;
    out_matrix.data[8]  = s;
    out_matrix.data[10] = c;
    return out_matrix;
}
inline mat4 mat4_euler_z(f32 angle_radians)
{
    mat4 out_matrix = mat4();

    f32 c = cosf(angle_radians);
    f32 s = sinf(angle_radians);

    out_matrix.data[0] = c;
    out_matrix.data[1] = s;
    out_matrix.data[4] = -s;
    out_matrix.data[5] = c;
    return out_matrix;
}
inline mat4 mat4_euler_xyz(f32 x_radians, f32 y_radians, f32 z_radians)
{
    mat4 rx         = mat4_euler_x(x_radians);
    mat4 ry         = mat4_euler_y(y_radians);
    mat4 rz         = mat4_euler_z(z_radians);
    mat4 out_matrix = mat4_mul(rx, ry);
    out_matrix      = mat4_mul(out_matrix, rz);
    return out_matrix;
}

inline vec3 mat4_forward(mat4 matrix)
{
    vec3 forward;
    forward.x = -matrix.data[2];
    forward.y = -matrix.data[6];
    forward.z = -matrix.data[10];
    forward.normalize();
    return forward;
}

/**
 * @brief Returns a backward vector relative to the provided matrix.
 *
 * @param matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
inline vec3 mat4_backward(mat4 matrix)
{
    vec3 backward;
    backward.x = matrix.data[2];
    backward.y = matrix.data[6];
    backward.z = matrix.data[10];
    backward.normalize();
    return backward;
}

/**
 * @brief Returns a upward vector relative to the provided matrix.
 *
 * @param matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
inline vec3 mat4_up(mat4 matrix)
{
    vec3 up;
    up.x = matrix.data[1];
    up.y = matrix.data[5];
    up.z = matrix.data[9];
    up.normalize();
    return up;
}

/**
 * @brief Returns a downward vector relative to the provided matrix.
 *
 * @param matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
inline vec3 mat4_down(mat4 matrix)
{
    vec3 down;
    down.x = -matrix.data[1];
    down.y = -matrix.data[5];
    down.z = -matrix.data[9];
    down.normalize();
    return down;
}

/**
 * @brief Returns a left vector relative to the provided matrix.
 *
 * @param matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
inline vec3 mat4_left(mat4 matrix)
{
    vec3 right;
    right.x = -matrix.data[0];
    right.y = -matrix.data[4];
    right.z = -matrix.data[8];
    right.normalize();
    return right;
}

/**
 * @brief Returns a right vector relative to the provided matrix.
 *
 * @param matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
inline vec3 mat4_right(mat4 matrix)
{
    vec3 left;
    left.x = matrix.data[0];
    left.y = matrix.data[4];
    left.z = matrix.data[8];
    left.normalize();
    return left;
}

// ------------------------------------------
// Quaternion
// ------------------------------------------

typedef vec4 quat;

inline quat quat_identity()
{
    return (quat){0, 0, 0, 1.0f};
}

inline f32 quat_normal(quat q)
{
    return sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
}

inline quat quat_normalize(quat q)
{
    f32 normal = quat_normal(q);
    return (quat){q.x / normal, q.y / normal, q.z / normal, q.w / normal};
}

inline quat quat_conjugate(quat q)
{
    return (quat){-q.x, -q.y, -q.z, q.w};
}

inline quat quat_inverse(quat q)
{
    return quat_normalize(quat_conjugate(q));
}

inline quat quat_mul(quat q_0, quat q_1)
{
    quat out_quaternion;

    out_quaternion.x = q_0.x * q_1.w + q_0.y * q_1.z - q_0.z * q_1.y + q_0.w * q_1.x;

    out_quaternion.y = -q_0.x * q_1.z + q_0.y * q_1.w + q_0.z * q_1.x + q_0.w * q_1.y;

    out_quaternion.z = q_0.x * q_1.y - q_0.y * q_1.x + q_0.z * q_1.w + q_0.w * q_1.z;

    out_quaternion.w = -q_0.x * q_1.x - q_0.y * q_1.y - q_0.z * q_1.z + q_0.w * q_1.w;

    return out_quaternion;
}

inline f32 quat_dot(quat q_0, quat q_1)
{
    return q_0.x * q_1.x + q_0.y * q_1.y + q_0.z * q_1.z + q_0.w * q_1.w;
}

inline mat4 quat_to_mat4(quat q)
{
    mat4 out_matrix = mat4();

    // https://stackoverflow.com/questions/1556260/convert-quaternion-rotation-to-rotation-matrix

    quat n = quat_normalize(q);

    out_matrix.data[0] = 1.0f - 2.0f * n.y * n.y - 2.0f * n.z * n.z;
    out_matrix.data[1] = 2.0f * n.x * n.y - 2.0f * n.z * n.w;
    out_matrix.data[2] = 2.0f * n.x * n.z + 2.0f * n.y * n.w;

    out_matrix.data[4] = 2.0f * n.x * n.y + 2.0f * n.z * n.w;
    out_matrix.data[5] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.z * n.z;
    out_matrix.data[6] = 2.0f * n.y * n.z - 2.0f * n.x * n.w;

    out_matrix.data[8]  = 2.0f * n.x * n.z - 2.0f * n.y * n.w;
    out_matrix.data[9]  = 2.0f * n.y * n.z + 2.0f * n.x * n.w;
    out_matrix.data[10] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.y * n.y;

    return out_matrix;
}

// Calculates a rotation matrix based on the quaternion and the passed in center point.
inline mat4 quat_to_rotation_matrix(quat q, vec3 center)
{
    mat4 out_matrix;

    f32 *o = out_matrix.data;
    o[0]   = (q.x * q.x) - (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
    o[1]   = 2.0f * ((q.x * q.y) + (q.z * q.w));
    o[2]   = 2.0f * ((q.x * q.z) - (q.y * q.w));
    o[3]   = center.x - center.x * o[0] - center.y * o[1] - center.z * o[2];

    o[4] = 2.0f * ((q.x * q.y) - (q.z * q.w));
    o[5] = -(q.x * q.x) + (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
    o[6] = 2.0f * ((q.y * q.z) + (q.x * q.w));
    o[7] = center.y - center.x * o[4] - center.y * o[5] - center.z * o[6];

    o[8]  = 2.0f * ((q.x * q.z) + (q.y * q.w));
    o[9]  = 2.0f * ((q.y * q.z) - (q.x * q.w));
    o[10] = -(q.x * q.x) - (q.y * q.y) + (q.z * q.z) + (q.w * q.w);
    o[11] = center.z - center.x * o[8] - center.y * o[9] - center.z * o[10];

    o[12] = 0.0f;
    o[13] = 0.0f;
    o[14] = 0.0f;
    o[15] = 1.0f;
    return out_matrix;
}

inline quat quat_from_axis_angle(vec3 axis, f32 angle, bool normalize)
{
    const f32 half_angle = 0.5f * angle;
    f32       s          = sinf(half_angle);
    f32       c          = cosf(half_angle);

    quat q = (quat){s * axis.x, s * axis.y, s * axis.z, c};
    if (normalize)
    {
        return quat_normalize(q);
    }
    return q;
}

inline quat quat_slerp(quat q_0, quat q_1, f32 percentage)
{
    quat out_quaternion;
    // Source: https://en.wikipedia.org/wiki/Slerp
    // Only unit quaternions are valid rotations.
    // Normalize to avoid undefined behavior.
    quat v0 = quat_normalize(q_0);
    quat v1 = quat_normalize(q_1);

    // Compute the cosine of the angle between the two vectors.
    f32 dot = quat_dot(v0, v1);

    // If the dot product is negative, slerp won't take
    // the shorter path. Note that v1 and -v1 are equivalent when
    // the negation is applied to all four components. Fix by
    // reversing one quaternion.
    if (dot < 0.0f)
    {
        v1.x = -v1.x;
        v1.y = -v1.y;
        v1.z = -v1.z;
        v1.w = -v1.w;
        dot  = -dot;
    }

    const f32 DOT_THRESHOLD = 0.9995f;
    if (dot > DOT_THRESHOLD)
    {
        // If the inputs are too close for comfort, linearly interpolate
        // and normalize the result.
        out_quaternion = (quat){v0.x + ((v1.x - v0.x) * percentage), v0.y + ((v1.y - v0.y) * percentage),
                                v0.z + ((v1.z - v0.z) * percentage), v0.w + ((v1.w - v0.w) * percentage)};

        return quat_normalize(out_quaternion);
    }

    // Since dot is in range [0, DOT_THRESHOLD], acos is safe
    f32 theta_0     = acosf(dot);           // theta_0 = angle between input vectors
    f32 theta       = theta_0 * percentage; // theta = angle between v0 and result
    f32 sin_theta   = sinf(theta);          // compute this value only once
    f32 sin_theta_0 = sinf(theta_0);        // compute this value only once

    f32 s0 = cosf(theta) - dot * sin_theta / sin_theta_0; // == sin(theta_0 - theta) / sin(theta_0)
    f32 s1 = sin_theta / sin_theta_0;

    return (quat){(v0.x * s0) + (v1.x * s1), (v0.y * s0) + (v1.y * s1), (v0.z * s0) + (v1.z * s1),
                  (v0.w * s0) + (v1.w * s1)};
}

/**
 * @brief Converts provided degrees to radians.
 *
 * @param degrees The degrees to be converted.
 * @return The amount in radians.
 */
inline f32 deg_to_rad(f32 degrees)
{
    return degrees * D_DEG2RAD_MULTIPLIER;
}

/**
 * @brief Converts provided radians to degrees.
 *
 * @param radians The radians to be converted.
 * @return The amount in degrees.
 */
inline f32 rad_to_deg(f32 radians)
{
    return radians * D_RAD2DEG_MULTIPLIER;
}
// if |a - b| < epsilon then we consider them to be equal so always negate the result if you want a > b
inline bool f32_compare(f32 a, f32 b, f32 tolerance)
{
    bool result = fabs(a - b) < tolerance;
    return result;
}

s32 drandom();
s32 drandom_in_range(s32 min, s32 max);

f32 fdrandom();
f32 fdrandom_in_range(f32 min, f32 max);

void scale_geometries(const struct geometry_config *config, vec3 scaling_factor);

#pragma clang diagnostic pop
