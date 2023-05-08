#pragma once

class LevelDataParser
{
public:
	void ParseGameData(const char* filePath)
	{
		// Open file here
		std::ifstream dataParser("GameData.txt", std::ios::in);

		// Start parsing data
		while (!dataParser.eof())
		{

		}
	}
};

