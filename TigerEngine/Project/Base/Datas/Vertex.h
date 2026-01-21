#pragma once
#include "../pch.h"

struct BoneWeightVertexData
{
    Vector3 position;
    Vector3 normal;
    Vector3 tangent;
    Vector3 bitangent;
    Vector2 texture;
	int BlendIndeces[4] = {};	// 
	float BlendWeights[4] = {};	// 

	void AddBoneData(int boneIndex, float weight)
	{
		assert(BlendWeights[0] == 0.0f ||
			BlendWeights[1] == 0.0f ||
			BlendWeights[2] == 0.0f ||
			BlendWeights[3] == 0.0f);

		for (int i = 0; i < 4; i++)
		{
			if (BlendWeights[i] == 0.0f)
			{
				BlendIndeces[i] = boneIndex;
				BlendWeights[i] = weight;
				return;
			}
		}
	}
};


// Only Position
struct Position_Vertex
{
    Vector3 position;
};

// Particle Quad
struct ParticleQuadVertex
{
    Vector2 corner; // (-0.5 ~ 0.5) : 빌보드용 로컬 오프셋
    Vector2 uv;
};

// ParticleInstance
struct ParticleInstance
{
    Vector3 pos;
    float   rotation;
    Vector2 size;
    float   frame;
    Vector4 color;
};