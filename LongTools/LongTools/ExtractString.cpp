#include "ExtractString.h"
#include <regex>
#include "lib/libxl/libxl.h"
#include "utils.h"

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

size_t utf8ToUnicode(unsigned int c, int charnum) {
	size_t n = c;
	if (charnum == 2) {
		n = (c & 0b00111111) << 2;
		n = n | (c & (0b00011111 << 8));
	}else if (charnum == 3) {
		n = (c & 0b00111111) << 2;
		n = (n | (c & (0b00111111 << 8))) << 2;
		n = (n | (c & (0b00001111 << 16))) >> 4;
	}else if (charnum == 4) {
		n = (c & 0b00111111) << 2;
		n = (n | (c & (0b00111111 << 8))) << 2;
		n = (n | (c & (0b00001111 << 16))) << 2;
		n = (n | (c & (0b00000111 << 24))) >> 6;
	}
	return n;
}

ExtractString::ExtractString(const std::string& dir, const std::string& inputfile, const std::string& filter, int type)
:TraverseDir(dir)
,_inputfile(inputfile)
,_type(type)
, _book(nullptr)
, _sheet(nullptr){
	splitFilter(filter, _filters, '@');
}

ExtractString::~ExtractString() {
	if (_book) {
		_book->release();
	}
}

bool ExtractString::init() {
	_book = xlCreateXMLBook();
	_book->setKey("TommoT", "windows-2421220b07c2e10a6eb96768a2p7r6gc");

	do {
		if (!_book->load(_inputfile.c_str())) {
			Log("load the excel file failed, %s", _inputfile.c_str());
			_book->release();
			_book = nullptr;
			break;
		}

		int num = _book->sheetCount();
		for (int i = 0; i < num; ++i) {
			libxl::Sheet* st = _book->getSheet(i);
			if (strcmp(st->name(), "Sheet1") == 0) {
				_sheet = st;
				break;
			}
		}
		if (!_sheet) {
			Log("load the excel Sheet1 failed");
			break;
		}

		size_t row = _sheet->lastRow();
		size_t col = _sheet->lastCol();
		for (size_t i = 1; i < row; ++i) {
			std::string s, d;
			libxl::CellType stype = _sheet->cellType(i, 0);
			if (stype == libxl::CellType::CELLTYPE_STRING){
				s = _sheet->readStr(i, 0);
			}

			libxl::CellType etype = _sheet->cellType(i, _type);
			if (etype == libxl::CellType::CELLTYPE_STRING) {
				d = _sheet->readStr(i, _type);
			}
			if (!s.empty()) {
				_result[s] = d;
			}
		}
	} while (0);
	return true;
}

bool ExtractString::handleFile(const std::string& file, const std::string& root, const std::string& relateDir) {
	Log("%s",file.c_str());
	std::string suffix = getSuffix(file);
	if (_filters.find(suffix) != _filters.end()) {
		size_t size = 0;
		auto data = getFileData(file.c_str(), size);
		std::string str(data.get(),size);

		if (isGBK((unsigned char*)str.c_str(), size) && !isUtf8((unsigned char*)str.c_str(), size)) {
			Log("%s is GBK , convert to utf8",file.c_str());
			str = GbkToUtf8(str);
			writeFileData(file, (unsigned char*)str.c_str(), str.size());
		}
		else if (str.size() >= 3 && (unsigned char)str[0] == 0XEF && (unsigned char)str[1] == 0XBB && (unsigned char)str[2] == 0XBF) {
			Log("%s has BOM, remove bom", file.c_str());
			str.replace(0, 3, "");
			writeFileData(file, (unsigned char*)str.c_str(), str.size());
		}

		if (suffix == "lua") {
			extractLuaFile(str);
		}
		if (suffix == "csd") {
			extractCsdFile(str);
		}
	}
	return true;
}

bool ExtractString::isContainChinese(const std::string& str) {
	size_t i = 0;
	while (i < str.size()) {
		int num = getNumOfOneChar(str[i]);
		unsigned int n = 0;
		for (int k = 0; k < num; k++) {
			n = (n << 8) | ((unsigned char)str[i + k]);
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
	for (auto iter = mt.begin() + 1; iter != mt.end(); ++iter) {
		std::string gbk = Utf8ToGbk(*iter);
		if (_result.find(gbk) == _result.end() && _addStr.find(gbk) == _addStr.end() && isGBK((unsigned char*)gbk.c_str(),gbk.size())) {
			trim(gbk);
			_addStr.insert(gbk);
		}
	}
}

void ExtractString::extractLuaFile(const std::string& str) {
	std::function<bool(const std::string&, size_t, size_t)> isInStr = [](const std::string& s, size_t pos, size_t len)->bool {
		int n1 = 0;
		int p1 = pos - 1;
		while (p1 >= 0) {
			if ((s[p1] == '"' || s[p1] == '\'') && (p1 == 0 || s[p1 - 1] != '\\')) ++n1;
			--p1;
		}
		int n2 = 0;
		int p2 = pos + len;
		while (p2 < s.size()) {
			if ((s[p2] == '"' || s[p2] == '\'') && (s[p2 - 1] != '\\')) ++n2;
			++p2;
		}
		return n1 % 2 != 0 && n2 % 2 != 0;
	};

	size_t pre = 0;
	size_t pos = str.find('\n');
	while (pos != -1) {
		if (pos > 0 && str[pos - 1] == '\\') {
			pre = pos + 1;
			pos = str.find('\n', pre);
		}
		else {
			std::string sub = str.substr(pre, pos - pre);
			size_t p = sub.find("--");
			if (p != -1 && !isInStr(sub, p, 2)) {
				sub = str.substr(0, p);
			}
			if (sub.size() > 0) {
				extractString(sub);
			}
			pre = pos + 1;
			pos = str.find('\n', pre);
		}
	}
	if (pre < str.size()) {
		std::string sub = str.substr(pre);
		size_t p = str.find("--");
		if (p != -1 && !isInStr(str, p, 2)) {
			sub = str.substr(0, p);
		}
		if (sub.size() > 0) {
			extractString(sub);
		}
	}
}

void ExtractString::extractCsdFile(const std::string& wstr) {
	std::regex re("LabelText=\\\"(.+?)\\\"");
	std::cmatch cm;
	const char* p = wstr.c_str();
	while (std::regex_search(p, cm, re)) {
		addString(cm[1]);
		p = cm[0].second;
	}
}

bool ExtractString::isCanSkip(char c) {
	if (c >= 'a' && c <= 'z') return false;
	if (c >= 'A' && c <= 'Z') return false;
	return true;
}

void ExtractString::extractString(const std::string& str) {
	size_t i  = 0;
	size_t p1 = -1, p2 = -1, p3 = -1;
	bool flag = false;
	
	while (i < str.size()) {
		int n = preNUm((unsigned char)str[i]);
		if (n == 1) {
			if ((str[i] == '"' || str[i] == '\'') && (i == 0 || str[i-1] != '\\')) {
				if (p1 != -1) {
					if (str[i] == str[p1]) {
						if (flag) {
							p2 = i;
							flag = false;
						}
						else {
							if (p2 != -1) {
								size_t pos = str.rfind("print", p1);
								if (pos == -1 || str.substr(pos, p1 - pos) != "print(") {
									addString(str.substr(p1 + 1, p2 - p1 - 1));
								}
							}
							p1 = i;
							p2 = -1;
						}
					}
				}
				else {
					p1 = i;
				}
			}
		}
		else {
			flag = true;
		}
		i += n;
	}
	if (p1 != -1 && p2 != -1) {
		size_t pos = str.rfind("print", p1);
		if (pos == -1 || str.substr(pos, p1 - pos) != "print(") {
			addString(str.substr(p1 + 1, p2 - p1 - 1));
		}
	}
}

void ExtractString::addString(const std::string& str) {
	std::string gbk = Utf8ToGbk(str);
	trim(gbk);
	if (!isGBK((unsigned char*)gbk.c_str(), gbk.size())) {
		return;
	}
	if (_result.find(gbk) == _result.end() && _addStr.find(gbk) == _addStr.end()) {
		_addStr.insert(gbk);
		Log("%s",gbk.c_str());
	}
}

void ExtractString::addString(std::string& str, size_t s, size_t e) {
	std::string utf8 = str.substr(s, e - s + 1);
	std::string gbk = Utf8ToGbk(utf8);
	trim(gbk);
	if (!isGBK((unsigned char*)gbk.c_str(), gbk.size())) {
		return;
	}
	if (_handleType == kExtract) {
		if (_result.find(gbk) == _result.end() && _addStr.find(gbk) == _addStr.end()) {
			_addStr.insert(gbk);
			Log("add string %s", gbk.c_str());
		}
	}
	else if (_handleType == kReplace) {
		auto iter = _result.find(gbk);
		if (iter != _result.end()) {
			std::string s = GbkToUtf8(iter->second);
			str.replace(str.find(utf8), utf8.size(), s);
			Log("replace string %s", gbk.c_str());
		}
	}
}

void ExtractString::traverseEnd() {
	static std::string title[] = { "中文","越文","日文","韩文" };
	if (!_book) {
		_book = xlCreateXMLBook();
		_book->setKey("TommoT", "windows-2421220b07c2e10a6eb96768a2p7r6gc");
	}
	if (!_sheet) {
		_sheet = _book->addSheet("Sheet1");
		for (size_t i = 0; i < sizeof(title) / sizeof(std::string); ++i) {
			_sheet->writeStr(0, i, Utf8ToGbk(title[i]).c_str());
		}
	}
	int nexRow = _sheet->lastRow();

	for (auto iter = _addStr.begin(); iter != _addStr.end(); ++iter) {
		_sheet->writeStr(nexRow++, 0, (*iter).c_str());
	}
	_book->save(_inputfile.c_str());
	_book->release();
	_book = nullptr;
}