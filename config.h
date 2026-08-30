#pragma once

#include <Arduino.h>
#include <FatFS.h>
#include "path.h"

String configPath(const String& path) {
	return joinPaths("/system/config", path) + ".nconf";
}

bool configExists(const String& path) {
	String fullPath = configPath(path);
	return FatFS.exists(fullPath);
}

void setConfig(const String& path, const String& value, bool override=true) {
	if (configExists(path) && !override) return;
	String fullPath = configPath(path);
	File file = FatFS.open(fullPath, "w");
	file.print(value);
	file.close();
}

String getConfig(const String& path) {
	if (!configExists(path)) return "";
	String fullPath = configPath(path);
	File file = FatFS.open(fullPath, "r");
	String str = file.readString();
	file.close();
	return str;
}