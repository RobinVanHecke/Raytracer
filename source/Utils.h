#pragma once
#include <cassert>
#include <fstream>

#include "Math.h"
#include "DataTypes.h"

namespace dae
{
	namespace GeometryUtils
	{
#pragma region Sphere HitTest
		//SPHERE HIT-TESTS

		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			const Vector3 sphereToRay = ray.origin - sphere.origin;

			const float a = Vector3::Dot(ray.direction, ray.direction);
			const float b = 2.0f * Vector3::Dot(ray.direction, sphereToRay);
			const float c = Vector3::Dot(sphereToRay, sphereToRay) - Square(sphere.radius);

			const float discriminant = Square(b) - 4.0f * a * c;

			float t0, t1;

			if (discriminant < 0) 
				return false;

			if (discriminant > 0)
			{
				float q;

				if (b > 0)
					q = -0.5f * (b + sqrt(discriminant));
				else
					q = -0.5f * (b - sqrt(discriminant));

				t0 = q / a;
				t1 = c / q;
			}

			else
				t0 = t1 = -0.5f * b / a;

			//ensures that the smallest value is always used
			if (t0 > t1) 
				std::swap(t0, t1);

			else if (t0 < 0) 
			{
				t0 = t1;  //if t0 is negative, let's use t1 instead

				if (t0 < 0) 
					return false;  //both t0 and t1 are negative 
			}

			const float t = t0;

			if (t < ray.min || t > ray.max)
				return false;

			if (!ignoreHitRecord)
			{
				hitRecord.origin = ray.origin + ray.direction * t;
				hitRecord.didHit = true;
				hitRecord.t = t;
				hitRecord.materialIndex = sphere.materialIndex;
				hitRecord.normal = Vector3{ (hitRecord.origin - sphere.origin).Normalized() };
				return true;
			}

			return true;
		}

		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Sphere(sphere, ray, temp, true);
		}
#pragma endregion
#pragma region Plane HitTest
		//PLANE HIT-TESTS
		inline bool HitTest_Plane(const Plane& plane, const Ray& ray, HitRecord& hitRecord, const bool ignoreHitRecord = false)
		{
			const float nominator{ Vector3::Dot(plane.origin - ray.origin, plane.normal) };
			const float denominator{ Vector3::Dot(ray.direction, plane.normal) };
			const float t{ nominator / denominator };

			if (t < ray.min || t > ray.max)
				return false;

			if (constexpr float epsilon{ FLT_EPSILON }; t > epsilon)
			{
				if (!ignoreHitRecord)
				{
					hitRecord.origin = ray.origin + ray.direction * t;
					hitRecord.normal = plane.normal;
					hitRecord.t = t;
					hitRecord.didHit = true;
					hitRecord.materialIndex = plane.materialIndex;
					return true;
				}
			}
			return false;
		}

		inline bool HitTest_Plane(const Plane& plane, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Plane(plane, ray, temp, true);
		}
#pragma endregion
#pragma region Triangle HitTest
		inline bool SlabTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			// x
			const float tx1 = (mesh.transformedMinAABB.x - ray.origin.x) / ray.direction.x;
			const float tx2 = (mesh.transformedMaxAABB.x - ray.origin.x) / ray.direction.x;

			float tmin = std::min(tx1, tx2);
			float tmax = std::max(tx1, tx2);

			// y
			const float ty1 = (mesh.transformedMinAABB.y - ray.origin.y) / ray.direction.y;
			const float ty2 = (mesh.transformedMaxAABB.y - ray.origin.y) / ray.direction.y;

			tmin = std::max(tmin, std::min(ty1, ty2));
			tmax = std::min(tmax, std::max(ty1, ty2));

			// z
			const float tz1 = (mesh.transformedMinAABB.z - ray.origin.z) / ray.direction.z;
			const float tz2 = (mesh.transformedMaxAABB.z - ray.origin.z) / ray.direction.z;

			tmin = std::max(tmin, std::min(tz1, tz2));
			tmax = std::min(tmax, std::max(tz1, tz2));

			return tmax > 0 && tmax >= tmin;
		}

		//TRIANGLE HIT-TESTS
		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			const Vector3 center{ (triangle.v0 + triangle.v1 + triangle.v2) / 3 };
			const Vector3 l{ center - ray.origin };

			const float nominator{ Vector3::Dot(l, triangle.normal) };
			const float denominator{ 1 / Vector3::Dot(ray.direction, triangle.normal) };
			const float t{ nominator * denominator };

			if (Vector3::Dot(triangle.normal, ray.direction) == 0.f)
				return false;

			if (t <= ray.min || t >= ray.max)
				return false;

			switch (triangle.cullMode)
			{
			case TriangleCullMode::BackFaceCulling:
				if (Vector3::Dot(ray.direction, triangle.normal) > 0)
					return false;
				break;

			case TriangleCullMode::FrontFaceCulling:
				if (Vector3::Dot(ray.direction, triangle.normal) < 0)
					return false;
				break;

			case TriangleCullMode::NoCulling:
				break;
			}

			const Vector3 p{ ray.origin + t * ray.direction };

			const Vector3 edgeA{ triangle.v1 - triangle.v0 };
			const Vector3 edgeB{ triangle.v2 - triangle.v1 };
			const Vector3 edgeC{ triangle.v0 - triangle.v2 };

			const Vector3 pointToSideA{ p - triangle.v0 };
			const Vector3 pointToSideB{ p - triangle.v1 };
			const Vector3 pointToSideC{ p - triangle.v2 };

			if (Vector3::Dot(triangle.normal, Vector3::Cross(edgeA, pointToSideA)) < 0)
				return false;
			if (Vector3::Dot(triangle.normal, Vector3::Cross(edgeB, pointToSideB)) < 0)
				return false;
			if (Vector3::Dot(triangle.normal, Vector3::Cross(edgeC, pointToSideC)) < 0)
				return false;

			if (t > 0 && ignoreHitRecord)
				return  true;

			if (t > 0 && !ignoreHitRecord)
			{
				hitRecord.origin = p;
				hitRecord.normal = triangle.normal;
				hitRecord.t = t;
				hitRecord.didHit = true;
				hitRecord.materialIndex = triangle.materialIndex;

				return true;
			}

			return false;
		}

		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Triangle(triangle, ray, temp, true);
		}


		inline bool HitTest_TriangleMesh(const TriangleMesh& triangleMesh, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			//slabtest
			if (!SlabTest_TriangleMesh(triangleMesh, ray))
				return false;

			HitRecord tempHitRecord;

			for (size_t i = 0; i < triangleMesh.indices.size(); i += 3)
			{
				Triangle triangle{
					triangleMesh.transformedPositions[triangleMesh.indices[i]],
					triangleMesh.transformedPositions[triangleMesh.indices[i + 1]],
					triangleMesh.transformedPositions[triangleMesh.indices[i + 2]],
					triangleMesh.transformedNormals[i / 3]
				};

				triangle.cullMode = triangleMesh.cullMode;
				triangle.materialIndex = triangleMesh.materialIndex;

				if (HitTest_Triangle(triangle, ray, tempHitRecord))
				{
					if (ignoreHitRecord)
						return true;

					if (tempHitRecord.t < hitRecord.t)
						hitRecord = tempHitRecord;
				}
			}

			return hitRecord.didHit;
		}

		inline bool HitTest_TriangleMesh(const TriangleMesh& triangleMesh, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_TriangleMesh(triangleMesh, ray, temp, true);
		}

		
#pragma endregion
#pragma endregion
	}

	namespace LightUtils
	{
		//Direction from target to light
		inline Vector3 GetDirectionToLight(const Light& light, const Vector3 origin)
		{
			if (light.type == LightType::Directional)
				return Vector3{FLT_MAX, FLT_MAX, FLT_MAX};
			
			return { light.origin - origin };
		}

		inline ColorRGB GetRadiance(const Light& light, const Vector3& target)
		{
			const Vector3 pointToShade{ light.origin - target };

			if (light.type == LightType::Point)
				return { light.color * light.intensity / Vector3::Dot(pointToShade, pointToShade) };

			return{ light.color * light.intensity };
		}
	}

	namespace Utils
	{
		//Just parses vertices and indices
#pragma warning(push)
#pragma warning(disable : 4505) //Warning unreferenced local function
		static bool ParseOBJ(const std::string& filename, std::vector<Vector3>& positions, std::vector<Vector3>& normals, std::vector<int>& indices)
		{
			std::ifstream file(filename);
			if (!file)
				return false;

			std::string sCommand;
			// start a while iteration ending when the end of file is reached (ios::eof)
			while (!file.eof())
			{
				//read the first word of the string, use the >> operator (istream::operator>>) 
				file >> sCommand;
				//use conditional statements to process the different commands	
				if (sCommand == "#")
				{
					// Ignore Comment
				}
				else if (sCommand == "v")
				{
					//Vertex
					float x, y, z;
					file >> x >> y >> z;
					positions.push_back({ x, y, z });
				}
				else if (sCommand == "f")
				{
					float i0, i1, i2;
					file >> i0 >> i1 >> i2;

					indices.push_back((int)i0 - 1);
					indices.push_back((int)i1 - 1);
					indices.push_back((int)i2 - 1);
				}
				//read till end of line and ignore all remaining chars
				file.ignore(1000, '\n');

				if (file.eof()) 
					break;
			}

			//Precompute normals
			for (uint64_t index = 0; index < indices.size(); index += 3)
			{
				uint32_t i0 = indices[index];
				uint32_t i1 = indices[index + 1];
				uint32_t i2 = indices[index + 2];

				Vector3 edgeV0V1 = positions[i1] - positions[i0];
				Vector3 edgeV0V2 = positions[i2] - positions[i0];
				Vector3 normal = Vector3::Cross(edgeV0V1, edgeV0V2);

				if(isnan(normal.x))
				{
					int k = 0;
				}

				normal.Normalize();
				if (isnan(normal.x))
				{
					int k = 0;
				}

				normals.push_back(normal);
			}

			return true;
		}
#pragma warning(pop)
	}
}