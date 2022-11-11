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
			const float nominator{ Vector3::Dot(plane.origin - ray.origin, plane.normal.Normalized()) };
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
		//TRIANGLE HIT-TESTS
		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			switch (triangle.cullMode)
			{
			case TriangleCullMode::BackFaceCulling:
				if (ignoreHitRecord)
				{
					if (Vector3::Dot(triangle.normal, ray.direction) < 0)
						return false;
				}
				else if (!ignoreHitRecord)
				{
					if (Vector3::Dot(triangle.normal, ray.direction) > 0)
						return false;
				}
				break;


			case TriangleCullMode::FrontFaceCulling:
				if (ignoreHitRecord)
				{
					if (Vector3::Dot(triangle.normal, ray.direction) > 0)
						return false;
				}
				else if (!ignoreHitRecord)
				{
					if (Vector3::Dot(triangle.normal, ray.direction) < 0)
						return false;
				}
				break;

			case TriangleCullMode::NoCulling:
				break;
			}

			if (Vector3::Dot(triangle.normal, ray.direction) == 0.f)
				return false;

			const Vector3 center{ (triangle.v0 + triangle.v1 + triangle.v2) / 3 };

			const Vector3 l{ center - ray.origin };

			const float t{ Vector3::Dot(l, triangle.normal) / Vector3::Dot(ray.direction, triangle.normal) };

			if (t < ray.min || t > ray.max)
				return false;

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

			if (!ignoreHitRecord)
			{
				hitRecord.origin = p;
				hitRecord.normal = triangle.normal;
				hitRecord.t = t;
				hitRecord.didHit = true;
				hitRecord.materialIndex = triangle.materialIndex;
				return true;
			}

			return true;
			
		}

		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Triangle(triangle, ray, temp, true);
		}


		inline bool HitTest_TriangleMesh(const TriangleMesh& triangleMesh, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{

			for (size_t i = 0; i < triangleMesh.indices.size(); i += 3)
			{
				const Vector3 pointA{ triangleMesh.transformedPositions[triangleMesh.indices[i]] };
				const Vector3 pointB{ triangleMesh.transformedPositions[triangleMesh.indices[i + 1]] };
				const Vector3 pointC{ triangleMesh.transformedPositions[triangleMesh.indices[i + 2]] };

				Triangle triangle{ pointA, pointB, pointC };

				triangle.cullMode = triangleMesh.cullMode;
				triangle.normal = triangleMesh.normals[i / 3];
				triangle.materialIndex = triangleMesh.materialIndex;

				if (HitTest_Triangle(triangle, ray, hitRecord, ignoreHitRecord))
					return true;
			}

			return false;
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