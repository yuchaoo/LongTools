#pragma once

#include "TraverseHandler.h"

class ExtractString
	:public TraverseDir {
public:
	ExtractString(const std::string& dir, const std::string& outfile, const std::string& filter);
	~ExtractString();
	virtual bool handleFile(const std::string& file, const std::string& root, const std::string& relateDir);
	virtual void traverseEnd();
private:
	bool isContainChinese(const std::string& str);
	void extractLuaFile(const char* data, size_t size);
	void extractCsbFile(const char* data, size_t size);
	void checkRegexMatch(const std::cmatch& mt);
	void saveToExcel(const std::vector<std::string>& result);
private:
	std::string _outfile;
	std::set<std::string> _filters;
	std::set<std::string> _result;
};