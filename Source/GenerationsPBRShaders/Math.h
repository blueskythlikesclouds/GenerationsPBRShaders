#pragma once

#undef near
#undef far
#undef min
#undef max

using Plane = Eigen::Hyperplane<float, 3>;
using AABB = Eigen::AlignedBox<float, 3>;

struct Frustum
{
    Plane Planes[6];

    Frustum(const Eigen::Matrix4f& matrix)
    {
        Planes[0].coeffs()[0] = matrix(3, 0) + matrix(0, 0);
        Planes[0].coeffs()[1] = matrix(3, 1) + matrix(0, 1);
        Planes[0].coeffs()[2] = matrix(3, 2) + matrix(0, 2);
        Planes[0].coeffs()[3] = matrix(3, 3) + matrix(0, 3);
                                                       
        Planes[1].coeffs()[0] = matrix(3, 0) - matrix(0, 0);
        Planes[1].coeffs()[1] = matrix(3, 1) - matrix(0, 1);
        Planes[1].coeffs()[2] = matrix(3, 2) - matrix(0, 2);
        Planes[1].coeffs()[3] = matrix(3, 3) - matrix(0, 3);
                                                       
        Planes[2].coeffs()[0] = matrix(3, 0) - matrix(1, 0);
        Planes[2].coeffs()[1] = matrix(3, 1) - matrix(1, 1);
        Planes[2].coeffs()[2] = matrix(3, 2) - matrix(1, 2);
        Planes[2].coeffs()[3] = matrix(3, 3) - matrix(1, 3);
                                                       
        Planes[3].coeffs()[0] = matrix(3, 0) + matrix(1, 0);
        Planes[3].coeffs()[1] = matrix(3, 1) + matrix(1, 1);
        Planes[3].coeffs()[2] = matrix(3, 2) + matrix(1, 2);
        Planes[3].coeffs()[3] = matrix(3, 3) + matrix(1, 3);
                                                       
        Planes[4].coeffs()[0] = matrix(3, 0) + matrix(2, 0);
        Planes[4].coeffs()[1] = matrix(3, 1) + matrix(2, 1);
        Planes[4].coeffs()[2] = matrix(3, 2) + matrix(2, 2);
        Planes[4].coeffs()[3] = matrix(3, 3) + matrix(2, 3);
                                                       
        Planes[5].coeffs()[0] = matrix(3, 0) - matrix(2, 0);
        Planes[5].coeffs()[1] = matrix(3, 1) - matrix(2, 1);
        Planes[5].coeffs()[2] = matrix(3, 2) - matrix(2, 2);
        Planes[5].coeffs()[3] = matrix(3, 3) - matrix(2, 3);

        for (int i = 0; i < 6; i++)
            Planes[i].normalize();
    }

    bool intersects(const AABB& box) const
    {
        for (auto& plane : Planes)
        {
            Eigen::Vector3f v(
                plane.normal().x() >= 0.0f ? box.min().x() : box.max().x(),
                plane.normal().y() >= 0.0f ? box.min().y() : box.max().y(),
                plane.normal().z() >= 0.0f ? box.min().z() : box.max().z());

            if (v.dot(plane.normal()) + plane.offset() >= 0)
                return true;
        }

        return false;
    }

    bool intersects(const Eigen::Vector3f& point) const 
    {
        for (auto& plane : Planes)
        {
            if (point.dot(plane.normal()) + plane.offset() <= 0)
                return false;
        }

        return true;
    }

    bool intersects(const Eigen::Vector3f& center, const float radius) const 
    {
        for (auto& plane : Planes)
        {
            if (center.dot(plane.normal()) + plane.offset() <= -radius)
                return false;
        }

        return true;
    }
};

struct OBB
{
    Eigen::Vector3f center;
    Eigen::Vector3f axis[3];
    Eigen::Vector3f halfExtents;

    OBB() {}

    OBB(const Eigen::Matrix4f& matrix, const float bounds = 1.0f)
    {
        center = matrix.col(3).head<3>();
        axis[0] = (matrix * Eigen::Vector4f::UnitX()).head<3>().normalized();
        axis[1] = (matrix * Eigen::Vector4f::UnitY()).head<3>().normalized();
        axis[2] = (matrix * Eigen::Vector4f::UnitZ()).head<3>().normalized();
        halfExtents = (Eigen::Vector3f(matrix.col(0).norm(), matrix.col(1).norm(), matrix.col(2).norm()) / bounds) / 2.0f;
    }

    Eigen::Vector3f closestPoint(const Eigen::Vector3f& point) const
    {
        const Eigen::Vector3f direction = point - center;

        const float x = std::clamp(direction.dot(axis[0]), -halfExtents[0], halfExtents[0]);
        const float y = std::clamp(direction.dot(axis[1]), -halfExtents[1], halfExtents[1]);
        const float z = std::clamp(direction.dot(axis[2]), -halfExtents[2], halfExtents[2]);

        return center + x * axis[0] + y * axis[1] + z * axis[2];
    }

    float closestPointDistanceSquared(const Eigen::Vector3f& point) const
    {
        return (point - closestPoint(point)).squaredNorm();
    }
};

inline Eigen::Vector3f transform(const Eigen::Vector3f& position, const Eigen::Matrix4f& matrix)
{
    return (matrix * Eigen::Vector4f(position.x(), position.y(), position.z(), 1)).head<3>();
}

inline Eigen::Vector3f transformNormal(const Eigen::Vector3f& normal, const Eigen::Matrix4f& matrix)
{
    return (matrix * Eigen::Vector4f(normal.x(), normal.y(), normal.z(), 0)).head<3>();
}

const Eigen::Vector3f AABB_CORNERS[8] =
{
    { +1.0f, +1.0f, +1.0f },
    { +1.0f, +1.0f, -1.0f },
    { +1.0f, -1.0f, +1.0f },
    { +1.0f, -1.0f, -1.0f },
    { -1.0f, +1.0f, +1.0f },
    { -1.0f, +1.0f, -1.0f },
    { -1.0f, -1.0f, +1.0f },
    { -1.0f, -1.0f, -1.0f }
};

inline float getAABBRadius(const AABB& aabb)
{
    float radius = 0;

    const Eigen::Vector3f center = aabb.center();
    for (size_t i = 0; i < 8; i++)
        radius = std::max<float>(radius, (center - aabb.corner((AABB::CornerType)i)).norm());

    return radius;
}

inline AABB getAABBFromOBB(const Eigen::Matrix4f& matrix, const float bounds, const float scale)
{
    AABB aabb;

    for (auto& corner : AABB_CORNERS)
    {
        aabb.extend(transform(corner * bounds, matrix) * scale);
    }

    return aabb;
}

#include <corecrt_math_defines.h>
#include "DllMods/Source/GenerationsFreeCamera/Math.h"

namespace Eigen
{
    template<typename Derived>
    Matrix<typename Derived::Scalar, 4, 4> CreateLookAtMatrix(Derived const& position, Derived const& target, Derived const& up)
    {
        Matrix<typename Derived::Scalar, 4, 4> matrix;
        Matrix<typename Derived::Scalar, 3, 3> rotation;
        rotation.col(2) = (position - target).normalized();
        rotation.col(0) = up.cross(rotation.col(2)).normalized();
        rotation.col(1) = rotation.col(2).cross(rotation.col(0));
        matrix.template topLeftCorner<3, 3>() = rotation.transpose();
        matrix.template topRightCorner<3, 1>() = -rotation.transpose() * position;
        matrix.row(3) << 0, 0, 0, 1;
        return matrix;
    }
}