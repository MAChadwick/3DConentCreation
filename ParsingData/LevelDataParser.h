#pragma once
#include <iostream>
#include <sstream>
#include <string>

class LevelDataParser
{
public:
	struct Mesh
	{
		GW::MATH::GMATRIXF world = GW::MATH::GIdentityMatrixF;
		std::string meshModel = "";
	};

	// TRUE: See all data Parsed in the console
	// FALSE: No data is outputed tp the console
	bool muteOutput = true;

	std::vector<Mesh> meshes = std::vector<Mesh>();

	const std::vector<Mesh> GetMeshes()
	{
		return meshes;
	}

	void ParseGameData(const char* filePath)
	{
		// Open file here
		std::ifstream dataParser(filePath);

		std::string data;
		std::getline(dataParser, data, '\n'); // First line is trash

		// Start parsing data
		while (!dataParser.eof())
		{
			// Format of data should be
			// Object Type
			// Mesh Name
			// <Matrix 4x4 (0.0000, 0.0000, 0.0000, 0.0000)
			//			   (data)
			//			   (data)
			//			   (data)>

			// Object Type
			std::getline(dataParser, data, '\n');
			if (data.compare("MESH") == 0)
			{
				Mesh newMesh;

				// Mesh Name
				std::getline(dataParser, newMesh.meshModel, '\n');

				// Detect duplicates. Could potentially be fooled if a period is in the correct place.
				if (newMesh.meshModel.at(newMesh.meshModel.length() - 4) == '.')
				{
					// Drop numbers for duplicate meshes
					newMesh.meshModel = newMesh.meshModel.substr(0, newMesh.meshModel.length() - 4);
				}

				// Append file path to mesh model
				newMesh.meshModel = "../h2b/" + newMesh.meshModel + ".h2b";

				int dataIndex = 0;

				// Parse matrix data into matrix
				for (int i = 0; i < 4; i++)
				{
					// Remove starting matrix text, '(',')', '<', '>'
					std::getline(dataParser, data, ')');
					data = data.substr(data.find_first_of('(') + 1);
					std::string ogData = data;

					for (int x = 0; x < 4; x++)
					{
						// Get float from substring
						int commaIndex = data.find(',');

						if (commaIndex == std::string::npos) commaIndex = data.length();

						std::string temp = data.substr(0, commaIndex);
						float value = std::stof(temp);

						newMesh.world.data[dataIndex] = value;

						// Set data to new substring excluding the previously read value
						commaIndex = data.find(',') + 1;
						if (commaIndex == std::string::npos) commaIndex = data.length() - 1;

						data = data.substr(commaIndex);
						dataIndex++;
					}
				}
				// Add mesh to parsed meshes
				meshes.push_back(newMesh);
			}
		}

		// Data dump of Parsed Meshes
		if (!muteOutput)
		{
			for (std::vector<Mesh>::iterator itr = meshes.begin(); itr != meshes.end(); itr++)
			{
				std::cout << ".h2b filepath: " << itr->meshModel << std::endl << "World Matrix: \n";

				for (int i = 0; i < 16; i += 4)
				{
					std::cout << itr->world.data[i] << ", " << itr->world.data[i + 1] << ", " << itr->world.data[i + 2] << ", " << itr->world.data[i + 3] << "\n";
				}

				std::cout << "\n\n";
			}
		}
	}

	~LevelDataParser()
	{
	}
};

