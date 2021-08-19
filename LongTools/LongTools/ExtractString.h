#pragma once

#include "TraverseHandler.h"
#include <unordered_map>
#include "lib/libxl/libxl.h"

class ExtractString
	:public TraverseDir {
public:
	enum {
		kChinese,
		kVietnamese, //越南
		kJapanese,   //日本
		kKorea      //韩国
	};
	enum {
		kExtract = 0, //提取
		kReplace = 1, //替换
	};
	ExtractString(const std::string& dir, const std::string& inputfile, const std::string& filter, int type);
	~ExtractString();
	virtual bool init();
	virtual bool handleFile(const std::string& file, const std::string& root, const std::string& relateDir);
	virtual void traverseEnd();
private:
	bool isContainChinese(const std::string& str);
	void extractLuaFile(const std::string& wstr);
	void extractCsdFile(const std::string& wstr);
	void checkRegexMatch(const std::cmatch& mt);
	void extractString(const std::string& str);
	void addString(const std::string& str);
	void addString(std::string& str, size_t s, size_t e);
	bool isCanSkip(char c);
private:
	std::string _inputfile;
	std::set<std::string> _filters;
	std::map<std::string,std::string> _result;
	libxl::Book* _book;
	libxl::Sheet* _sheet;
	std::set<std::string> _addStr;
	int _type;
	int _handleType;
};