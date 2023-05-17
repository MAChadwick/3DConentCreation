#pragma once

#include "h2bParser.h"
#include "../Gateware/Gateware.h"

class ObjectData
{

public:

	std::vector<H2B::MESH> objectMeshes;
	std::vector<H2B::VERTEX> objectVerts;
	std::vector<unsigned> objectIndices;
	std::vector<H2B::MATERIAL> objectMaterials;

	GW::MATH::GMATRIXF objectWorldMatrix;

	ObjectData (std::vector<H2B::MESH> newMeshes, std::vector<H2B::VERTEX> newVerts, std::vector<unsigned> newIndices, 
				std::vector<H2B::MATERIAL> newMaterials, GW::MATH::GMATRIXF newWorld)
	{
		objectMeshes = newMeshes;
		objectVerts = newVerts;
		objectIndices = newIndices;
		objectMaterials = newMaterials;
		objectWorldMatrix = newWorld;
	}

	void OverrideIndexOffset(int newOffsetStart)
	{
		int extraOffset = 0;

		for (std::vector<H2B::MESH>::iterator itr = objectMeshes.begin(); itr != objectMeshes.end(); itr++)
		{
			itr->drawInfo.indexOffset = newOffsetStart + extraOffset;
			extraOffset += itr->drawInfo.indexCount;
		}
	}
};