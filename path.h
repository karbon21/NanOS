#pragma once

#include <Arduino.h>

String normalizePath(const String& path) {
	if (path.length() == 0) return "/";

    std::vector<String> parts;
    int startIndex = 0;

    while (startIndex < path.length()) {
        int slashIndex = path.indexOf('/', startIndex);
        String part;

        if (slashIndex == -1) {
            part = path.substring(startIndex);
            startIndex = path.length();
        } else {
            part = path.substring(startIndex, slashIndex);
            startIndex = slashIndex + 1;
        }

        if (part == "" || part == ".") {
            continue;
        } else if (part == "..") {
            if (!parts.empty()) {
                parts.pop_back();
            }
        } else {
            parts.push_back(part);
        }
    }

    String result = "";
    for (const String& p : parts) {
        result += "/" + p;
    }

    if (result == "") {
        return "/";
    }

    return result;
}

String joinPaths(const String& path1, const String& path2) {
	if (path1.isEmpty()) return path2;
    if (path2.isEmpty()) return path1;

    return normalizePath(path1 + "/" + path2);
}