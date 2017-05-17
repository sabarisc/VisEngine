#pragma once
#include "VisEngineMath.h"

#define MAX_NUM_VERTICES	32
#define MAX_NUM_INDICES		96 //is thrice the num vertices a good metric?

struct Model
{
	Vector3 vertices[MAX_NUM_VERTICES];
	Vector3 normals[MAX_NUM_VERTICES];
	Vector2 texcoords[MAX_NUM_VERTICES];
	unsigned short indices[MAX_NUM_INDICES]; //perhaps change this to WORD to conform with index in Render.h

	Matrix43 world;

	unsigned short numVerts;
	unsigned short numIndices;
};