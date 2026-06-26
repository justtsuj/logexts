#include "pch.h"
#include "logconf.h"
#include "nlohmann/json.hpp"

void LogConfig::ReadFromJSON(LPCTSTR filePath) {
	std::ifstream f(filePath);
	if (!f.good()) {
		OutputDebugString(TEXT("Error opening ini file\r\n"));
		return;
	}
	nlohmann::json data = nlohmann::json::parse(f);
	if (data.contains("excludeApis")) {
		for (const nlohmann::json& element : data.at("excludeApis")) {
			excludeApis.emplace_back(element);
		}
	}
	if (data.contains("excludeDlls")) {
		for (const nlohmann::json& element : data.at("excludeDlls")) {
			excludeDlls.emplace_back(element);
		}
	}
	isDebug = data.value("isDebug", false);
}