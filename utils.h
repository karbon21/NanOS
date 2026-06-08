#ifndef UTILS_H
#define UTILS_S

#include <vector>

/**
 * Splits a String into an array, ignoring multiple consecutive spaces.
 * @param input: The source String to split.
 * @return: The result vector.
 */
std::vector<String> splitString(String input) {
	std::vector<String> results;
	int strLen = input.length();
	int startIndex = 0;
	int stopIndex = 0;

	// Clean up the edges
	input.trim();

	// If the string is empty after trimming, return an empty vector
	if (input.length() == 0) {
		return results;
	}

	while (stopIndex != -1) {
		stopIndex = input.indexOf(' ', startIndex);
		
		// Determine the substring end point
		int endAt = (stopIndex == -1) ? strLen : stopIndex;
		String word = input.substring(startIndex, endAt);

		// Logic: Only push to vector if the word contains actual characters
		if (word.length() > 0) {
		results.push_back(word);
		}

		// Move search start to the next character
		startIndex = stopIndex + 1;
		
		// Safety break: if we've reached the end of the string
		if (stopIndex == -1) break;
	}

	return results;
}

#endif