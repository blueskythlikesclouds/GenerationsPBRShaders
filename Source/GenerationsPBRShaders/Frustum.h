#pragma once

#undef near
#undef far
#undef min
#undef max

using Plane = Eigen::Hyperplane<float, 3>;
using Box = Eigen::AlignedBox<float, 3>;

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

	bool intersects(const Box& box) const
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