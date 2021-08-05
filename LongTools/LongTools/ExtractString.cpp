#include "ExtractString.h"
#include <regex>

int getNumOfOneChar(unsigned char c) {
	if ((0b11110000 & c) == 0b11110000) return 4;
	if ((0b11100000 & c) == 0b11100000) return 3;
	if ((0b11000000 & c) == 0b11000000) return 2;
	return 1;
}

bool isChineseUnicode(unsigned int n) {
	if (n >= 0X4E00 && n <= 0X9FA5) {
		return true;
	}
	if (n >= 0X9FA6 && n <= 0X9FEF) {
		return true;
	}
	return false;
}

size_t utf8ToUnicode(int c, int charnum) {
	if (charnum == 2) {
		return (c & (0b00011111 << 8)) | (c & 0b00111111);
	}
	if (charnum == 3) {
		return (c & (0b00001111 << 16)) | (c & (0b00111111 << 8)) | (c & 0b00111111);
	}
	if (charnum == 4) {
		return (c & (0b00000111 << 24)) | (c & (0b00001111 << 16)) | (c & (0b00111111 << 8)) | (c & 0b00111111);
	}
	return c;
}

bool ExtractString::handleFile(const std::string& file, const std::string& root, const std::string& relateDir) {

}

bool ExtractString::isContainChinese(const std::string& str) {
	size_t i = 0;
	while (i < str.size()) {
		int num = getNumOfOneChar(str[i]);
		unsigned int n = 0;
		for (int k = 0; k < num; k++) {
			n = (n << 8) | str[i + k];
		}
		n = utf8ToUnicode(n, num);
		if (isChineseUnicode(n)) {
			return true;
		}
		i += n;
	}
	return false;
}

void ExtractString::checkRegexMatch(const std::cmatch& mt) {
	for (auto iter = mt.begin(); iter != mt.end(); ++iter) {
		std::string str = iter->str();
		if (_result.find(str) == _result.end() && isContainChinese(str)) {
			_result.insert(str);
		}
	}
}

void ExtractString::extractLuaFile(const char* data, size_t size) {
	std::regex re("\"(.+)\"");
	std::cmatch cm;
	if (std::regex_match(data, cm, re)) {
		checkRegexMatch(cm);
	}
}

void ExtractString::extractCsbFile(const char* data, size_t size) {
	std::regex re("LabelText = \"(.+)\"");
	std::cmatch cm;
	if (std::regex_match(data, cm, re)) {
		checkRegexMatch(cm);
	}
}

void ExtractString::traverseEnd() {
	std::vector<std::string> list;
	for (auto iter = _result.begin(); iter != _result.end(); ++iter) {
		list.push_back(*iter);
	}
	std::sort(list.begin(), list.end());

}

void ExtractString::saveToExcel(const std::vector<std::string>& result) {

}