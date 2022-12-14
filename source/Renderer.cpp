//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Material.h"
#include "Scene.h"
#include "Utils.h"

#include <future>
#include <ppl.h>

using namespace dae;

//#define ASYNC
#define PARALLEL_FOR

Renderer::Renderer(SDL_Window * pWindow) :
	m_pWindow(pWindow),
	m_pBuffer(SDL_GetWindowSurface(pWindow))
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	m_pBufferPixels = static_cast<uint32_t*>(m_pBuffer->pixels);
}

void Renderer::Render(Scene* pScene) const
{
	Camera& camera = pScene->GetCamera();
	camera.CalculateCameraToWorld();

	const float fovAngle = camera.fovAngle * TO_RADIANS;
	const float fov = tan(fovAngle / 2.f);

	const float aspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);
		
	auto& materials = pScene->GetMaterials();
	auto& lights = pScene->GetLights();

	const uint32_t numPixels = m_Width * m_Height;

#if defined(ASYNC)
	//Async

	const uint32_t numCores = std::thread::hardware_concurrency();
	std::vector<std::future<void>> async_futures{};
	const uint32_t numPixelsPerTask = numPixels / numCores;
	uint32_t numUnassignedPixels = numPixels % numCores;
	uint32_t currPixelIndex = 0;

	for (uint32_t coreId{0}; coreId < numCores; ++coreId)
	{
		uint32_t taskSize = numPixelsPerTask;
		if (numUnassignedPixels > 0)
		{
			++taskSize;
			--numUnassignedPixels;
		}

		async_futures.push_back(std::async(std::launch::async, [=, this]
			{
				// render all pixels for this task (currPixelIndex > currPixelIndex + taskSize)
				const uint32_t pixelIndexEnd = currPixelIndex + taskSize;
				for (uint32_t pixelIndex{ currPixelIndex }; pixelIndex < pixelIndexEnd; ++pixelIndex)
				{
					RenderPixel(pScene, pixelIndex, fov, aspectRatio, camera, lights, materials);
				}
			}));

		currPixelIndex += taskSize;

		// wait for async completion of all tasks
		for (const std::future<void>& f : async_futures)
			f.wait();
	}

#elif defined(PARALLEL_FOR)
	//Parallel for

	concurrency::parallel_for(0u, numPixels, [=, this](int i)
		{
			RenderPixel(pScene, i, fov, aspectRatio, camera, lights, materials);
		});

#else
	// no threading
	for (uint32_t i{0}; i < numPixels; ++i)
	{
		RenderPixel(pScene, i, fov, aspectRatio, camera, lights, materials);
	}

#endif

	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::RenderPixel(const Scene* pScene, const uint32_t pixelIndex, const float fov, const float aspectRatio, const Camera& camera,
                           const std::vector<Light>& lights, const std::vector<Material*>& materials) const
{
	const int px = static_cast<int>(pixelIndex) % m_Width;
	const int py = static_cast<int>(pixelIndex) / m_Width;

	const float rx = static_cast<float>(px) + 0.5f;
	const float ry = static_cast<float>(py) + 0.5f;

	const float cx = (2.f * rx / static_cast<float>(m_Width) - 1.f) * (aspectRatio * fov);
	const float cy = (1.f - 2.f * ry / static_cast<float>(m_Height)) * fov;

	Vector3 rayDirection{ cx, cy, 1 };
	rayDirection = camera.cameraToWorld.TransformVector(rayDirection);
	rayDirection.Normalize();

	const Ray viewRay{ camera.origin, rayDirection };
	ColorRGB finalColor{};
	HitRecord closestHit{};

	pScene->GetClosestHit(viewRay, closestHit);

	if (!closestHit.didHit)
		finalColor = colors::Black;

	else
	{
		for (const auto& light : lights)
		{
			Vector3 lightDir{ LightUtils::GetDirectionToLight(light, closestHit.origin + (closestHit.normal * 0.0001f)) };
			Vector3 normalizedLightDir{ lightDir.Normalized() };

			Ray lightRay{ closestHit.origin + (closestHit.normal * 0.0001f),normalizedLightDir,0.0001f, lightDir.Magnitude() };

			const float observedArea{ Vector3::Dot(closestHit.normal,normalizedLightDir) };
			if (observedArea < 0)
				continue;

			if (m_ShadowsEnabled)
				if (pScene->DoesHit(lightRay))
					continue;

			switch (m_CurrentLightingMode)
			{
			case LightingMode::ObservedArea:
				finalColor += {observedArea, observedArea, observedArea};
				break;

			case LightingMode::Radiance:
				finalColor += LightUtils::GetRadiance(light, closestHit.origin);
				break;

			case LightingMode::BRDF:
				finalColor += materials[closestHit.materialIndex]->Shade(closestHit, normalizedLightDir, -camera.forward);
				break;

			case LightingMode::Combined:
				finalColor += LightUtils::GetRadiance(light, closestHit.origin) * observedArea * materials[closestHit.materialIndex]->Shade(closestHit, normalizedLightDir, -camera.forward);
				break;
			}
		}
	}
	//Update Color in Buffer
	finalColor.MaxToOne();

	m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBuffer->format,
		static_cast<uint8_t>(finalColor.r * 255),
		static_cast<uint8_t>(finalColor.g * 255),
		static_cast<uint8_t>(finalColor.b * 255));
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBuffer, "RayTracing_Buffer.bmp");
}

void Renderer::CycleLightingMode()
{
	if (m_CurrentLightingMode == LightingMode::Combined)
	{
		m_CurrentLightingMode = LightingMode::ObservedArea;
		return;
	}

	int temp = static_cast<int>(m_CurrentLightingMode);
	++temp;
	m_CurrentLightingMode = static_cast<LightingMode>(temp);
}
