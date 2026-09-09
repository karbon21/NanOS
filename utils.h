#pragma once

#include <vector>
#include <array>
#include <algorithm>

template <typename T, size_t N>
bool contains(const std::array<T, N>& arr, const T& target) {
    auto it = std::find(std::begin(arr), std::end(arr), target);
    return it != std::end(arr);
}

int countChars(const String& str, char c) {
	return std::count(str.c_str(), str.c_str() + str.length(), c);
}

std::vector<String> splitString(String input) {
	std::vector<String> results;
	int strLen = input.length();
	int startIndex = 0;
	int stopIndex = 0;

	input.trim();

	if (input.length() == 0) {
		return results;
	}

	while (stopIndex != -1) {
		stopIndex = input.indexOf(' ', startIndex);
		int endAt = (stopIndex == -1) ? strLen : stopIndex;
		String word = input.substring(startIndex, endAt);

		if (word.length() > 0) {
			results.push_back(word);
		}

		startIndex = stopIndex + 1;
		if (stopIndex == -1) break;
	}

	return results;
}
