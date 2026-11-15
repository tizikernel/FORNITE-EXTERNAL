#pragma once
#include <string>

namespace Backend
{
	bool ValidateKeyOnline(const std::string& key);
	std::string LastError();
	std::string MakeDeviceFingerprint();
}
