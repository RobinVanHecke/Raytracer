#pragma once
#include <cassert>
#include <iostream>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{
		}


		Vector3 origin{};
		float fovAngle{ 90.f };

		Vector3 forward{ Vector3::UnitZ };
		Vector3 up{ Vector3::UnitY };
		Vector3 right{ Vector3::UnitX };

		float totalPitch{ 0.f };
		float totalYaw{ 0.f };

		const float movementSpeed{ 10.f };
		const float rotationSpeed{ 1.f };

		Matrix cameraToWorld{};


		Matrix CalculateCameraToWorld()
		{
			//todoDone: W2

			Vector3 tempRight = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			Vector3 tempUp = Vector3::Cross(forward, tempRight).Normalized();

			cameraToWorld = 
			{
				{tempRight, 0},
				{tempUp, 0},
				{forward,0},
				{origin, 1}
			};

			return cameraToWorld;
		}

		void Update(const Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			if (pKeyboardState[SDL_SCANCODE_W])
				origin.z += movementSpeed * deltaTime;
			else if (pKeyboardState[SDL_SCANCODE_S])
				origin.z -= movementSpeed * deltaTime;

			if (pKeyboardState[SDL_SCANCODE_A])
				origin.x -= movementSpeed * deltaTime;
			else if (pKeyboardState[SDL_SCANCODE_D])
				origin.x += movementSpeed * deltaTime;

			//Mouse Input
			int mouseX{}, mouseY{};

			switch (SDL_GetRelativeMouseState(&mouseX, &mouseY))
			{
			case SDL_BUTTON(1):
				origin = forward.Normalized() * static_cast<float>(mouseY) * deltaTime;
				totalYaw -= static_cast<float>(mouseX) * rotationSpeed * deltaTime;
				break;

			case SDL_BUTTON(3):
				totalYaw -= static_cast<float>(mouseX) * rotationSpeed * deltaTime;
				totalPitch -= static_cast<float>(mouseY) * rotationSpeed * deltaTime;
				break;

			case SDL_BUTTON(1) | SDL_BUTTON(3):
				origin += up.Normalized() * static_cast<float>(mouseY) * deltaTime;
				break;

			default:;
			}

			const Matrix finalRotation = { Matrix::CreateRotation(totalPitch,totalYaw,0) };
			forward = finalRotation.TransformVector(Vector3::UnitZ).Normalized();

			//todo: W2
			//assert(false && "Not Implemented Yet");
		}
	};
}
