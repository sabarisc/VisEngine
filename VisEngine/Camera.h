#pragma once
#include "VisEngineMath.h"

struct Camera
{
	Matrix43 world;

	Camera()
	{
		world.SetIdentity();
	}
};