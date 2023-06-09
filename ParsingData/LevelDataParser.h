#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <time.h>

class LevelDataParser
{

public:

	struct Mesh
	{
		bool enableCollisions = false;
		GW::MATH::GMATRIXF world = GW::MATH::GIdentityMatrixF;
		GW::MATH::GVECTORF maxAxis = { -9999, -9999, -9999, 1 };
		GW::MATH::GVECTORF minAxis = { 9999, 9999, 9999, 1 };
		GW::MATH::GVECTORF bounding[8];
		std::string meshModel = "";
	};

	struct Light
	{
		int type;
		float falloff;
		float padding[2] = { 1, 1 };
		GW::MATH::GQUATERNIONF rotation;
		GW::MATH::GVECTORF position;
		GW::MATH::GVECTORF color = { 0.75, 0.75, 0.75, 1 };
	};

	// TRUE: See all data Parsed in the console
	// FALSE: No data is outputed tp the console
	bool muteOutput = true;

	Mesh camera;

	std::vector<Mesh> meshes = std::vector<Mesh>();
	std::vector<Light> lights = std::vector<Light>();

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
		while (!dataParser.eof() && dataParser.is_open())
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

				// Collisions enabled bool
				std::getline(dataParser, data, '\n');
				if (data.compare("True") == 0) newMesh.enableCollisions = true;

				// Mesh Name
				std::getline(dataParser, newMesh.meshModel, '\n');

				// Detect duplicates. Could potentially be fooled if a period is in the correct place.
				if (newMesh.meshModel.length() > 3 && newMesh.meshModel.at(newMesh.meshModel.length() - 4) == '.')
				{
					// Drop numbers for duplicate meshes
					newMesh.meshModel = newMesh.meshModel.substr(0, newMesh.meshModel.length() - 4);
				}

				// Append file path to mesh model
				newMesh.meshModel = "../h2b/" + newMesh.meshModel + ".h2b";

				// Parse matrix here
				ReadStreamIntoMatrix(newMesh.world, dataParser, data);

				// Parse bounding box data
				ReadStreamIntoBoundingBox(dataParser, data, newMesh);

				// Add mesh to parsed meshes
				meshes.push_back(newMesh);
			}
			else if ((data.compare("LIGHT") == 0))
			{
				Light newLight;

				// Trash collisions line
				std::getline(dataParser, data, '\n');

				// Trash light name
				std::getline(dataParser, data, '\n');

				// Drop number for duplicate lights - Also trash for now
				if (data.length () > 3 && data.at(data.length() - 4) == '.')
				{
					data = data.substr(0, data.length() - 4);
				}

				// Parse matrix
				GW::MATH::GMATRIXF temp;
				ReadStreamIntoMatrix(temp, dataParser, data);
				newLight.position = temp.row4;

				// Parse Light data
				ReadStreamIntoLight(dataParser, data, newLight);

				// Parse rotation data - Formatted in w, x, y, z
				for (int x = 0; x < 4; x++)
				{
					int index = data.find_first_of('=') + 1;
					data = data.substr(index);
					std::string temp = data.substr(0, 6);

					float value = std::stof(data);
					newLight.rotation.data[(x + 3) % 4] = value;
				}

				// Trash last bit of ;ine from parsing
				std::getline(dataParser, data, '\n');

				// Get light attenuation data
				newLight.falloff = std::stof(data);

				// Push into light vector

				std::cout << "Added light #" << lights.size() << std::endl;
				lights.push_back(newLight);

			}
			else if ((data.compare("CAMERA") == 0))
			{
				std::getline(dataParser, data, '\n');

				ReadStreamIntoMatrix(camera.world, dataParser, data);
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

	private:
		void ReadStreamIntoMatrix(GW::MATH::GMATRIXF &world, std::ifstream& stream, std::string& data)
		{
			int dataIndex = 0;

			// Parse matrix data into matrix
			for (int i = 0; i < 4; i++)
			{

				// Remove starting matrix text, '(',')', '<', '>'
				std::getline(stream, data, ')');
				data = data.substr(data.find_first_of('(') + 1);
				std::string ogData = data;

				for (int x = 0; x < 4; x++)
				{
					// Get float from substring
					int commaIndex = data.find(',');

					if (commaIndex == std::string::npos) commaIndex = data.length();

					std::string temp = data.substr(0, commaIndex);
					float value = std::stof(temp);

					world.data[dataIndex] = value;

					// Set data to new substring excluding the previously read value
					commaIndex = data.find(',') + 1;
					if (commaIndex == std::string::npos) commaIndex = data.length() - 1;

					data = data.substr(commaIndex);
					dataIndex++;
				}

				// Trash last '>' left over from data parsing
				std::getline(stream, data, '\n');
			}
		}

		void ReadStreamIntoBoundingBox(std::ifstream& stream, std::string& data, Mesh& newMesh)
		{
			int dataIndex = 0;


			for (int i = 0; i < 8; i++)
			{

				std::getline(stream, data, '\n');

				std::string temp = data.substr(0, data.find_first_of(','));
				newMesh.bounding[i].data[0] = std::stof(temp);

				data = data.substr(data.find_first_of(',') + 1);
				temp = data.substr(0, data.find_first_of(','));
				newMesh.bounding[i].data[1] = std::stof(temp);

				data = data.substr(data.find_first_of(',') + 1);
				newMesh.bounding[i].data[2] = std::stof(data);
				newMesh.bounding[i].data[3] = 1;

				GW::MATH::GVector::VectorXMatrixF(newMesh.bounding[i], newMesh.world, newMesh.bounding[i]);

				// Easier to calculate bounding box with min/max values
				if (newMesh.bounding[i].x > newMesh.maxAxis.x) newMesh.maxAxis.x = newMesh.bounding[i].x;
				if (newMesh.bounding[i].x < newMesh.minAxis.x) newMesh.minAxis.x = newMesh.bounding[i].x;
				if (newMesh.bounding[i].y > newMesh.maxAxis.y) newMesh.maxAxis.y = newMesh.bounding[i].y;
				if (newMesh.bounding[i].y < newMesh.minAxis.y) newMesh.minAxis.y = newMesh.bounding[i].y;
				if (newMesh.bounding[i].z > newMesh.maxAxis.z) newMesh.maxAxis.z = newMesh.bounding[i].z;
				if (newMesh.bounding[i].z < newMesh.minAxis.z) newMesh.minAxis.z = newMesh.bounding[i].z;
			}
		}

		void ReadStreamIntoLight(std::ifstream &stream, std::string &data, Light &newLight)
		{
			// Get color line for lights
			std::getline(stream, data, '\n');

			if (data.compare("SUN") == 0)
				newLight.type = 0;
			else if (data.compare("POINT") == 0)
				newLight.type = 1;
			else if (data.compare("SPOT") == 0)
				newLight.type = 2;
			else
				newLight.type = 0;

			std::getline(stream, data, '\n');

			// Remove extra info and parse color value
			for (int x = 0; x < 3; x++)
			{
				int index = data.find_first_of('=') + 1;
				data = data.substr(index);
				std::string temp = data.substr(0, 6);

				float value = std::stof(data);
				newLight.color.data[x] = value;
			}

			// Finish line
			std::getline(stream, data, '\n');
		}

};

