#pragma once

#include "Module.h"

class ModuleResources : public Module
{
public:
	ModuleResources();

	void createUploadBuffer();

	void createDefaultBuffer();
private:
};