#pragma once
#include "TraverseHandler.h"
#include "lib/libxl/libxl.h"
#include <vector>

#define CLIENT_FLAG "client"
#define SERVER_FLAG "server"
#define ALL_FLAG "all"

#define TYPE_INT "int"
#define TYPE_FLOAT "float"
#define TYPE_STR "string"
#define TYPE_TEXT "map"
#define TYPE_BOOL "bool"

class ExcelBook;
class ExcelSheet;

class ExcelParser : public TraverseDir {
public:
	ExcelParser(const std::string& input, const std::string& clientOutput, const std::string& serverOutput, const std::string& exportFlag);
	~ExcelParser();
	virtual bool handleFile(const std::string& file, const std::string& root, const std::string& relateDir);
	void exportSheet(ExcelSheet* sheet);
private:
	std::string _clientOutput;
	std::string _serverOutput;
	std::string _exportFlag;
};

class CellValue;
class ExcelCell;

class ExcelBook {
public:
	ExcelBook(const std::string& file);
	~ExcelBook();
	bool init();
	const std::vector<ExcelSheet*>& getSheets() { return _sheets; }
private:
	std::string _file;
	libxl::Book* _book;
	int _sheetCount;
	std::vector<ExcelSheet*> _sheets;
};

class ExcelSheet {
public:
	ExcelSheet(libxl::Book* book, int index);
	~ExcelSheet();
	bool init();
	int getFirstRow() const { return _firstRow; }
	int getLastRow() const { return _lastRow; }
	int getFirstCol() const { return _firstCol; }
	int getLastCol() const { return _lastCol; }
	const std::string& getName() const { return _name; }
	const std::vector<std::vector<ExcelCell*>>& getDataList() const { return _dataList; }
	const std::vector<std::string>& getHeaderList() const { return _fieldHeader; }
	const std::vector<std::string>& getTypeList() const { return _fieldTypes; }
	const std::vector<bool>& getClientExportList() const { return _clientExportFlag; }
	const std::vector<bool>& getServerExportList() const { return _serverExportFlag; }
private:
	void setExportColor(libxl::Color color, const std::string& str);
private:
	libxl::Sheet* _sheet;
	libxl::Book* _book;
	std::string _name;
	int _sheetIndex;
	int _firstRow, _lastRow, _firstCol,  _lastCol;
	std::vector<std::vector<ExcelCell*>> _dataList;
	std::vector<std::string> _fieldHeader;
	std::vector<std::string> _fieldTypes;
	std::vector<bool> _clientExportFlag;
	std::vector<bool> _serverExportFlag;
	libxl::Color _clientColor;
	libxl::Color _serverColor;
	libxl::Color _allColor;
};

class ExcelCell {
public:
	ExcelCell(libxl::Sheet* sheet, int row, int col);
	~ExcelCell();
	CellValue* readValue();
	CellValue* getValue() { return _cellValue; };
	libxl::Color getColor() { return _color; };
private:
	int _row, _col;
	libxl::Sheet* _sheet;
	libxl::CellType _cellType;
	CellValue* _cellValue;
	libxl::Color _color;
};

class CellValue {
public:
	enum {
		NIL,
		BOOL,
		FLOAT,
		INT,
		STRING
	};
	CellValue();
	~CellValue();
	void setBool(bool v);
	void setFloat(float v);
	void setInt(int v);
	void setString(const char* str, size_t len);
	bool getBool();
	float getFloat();
	int getInt();
	const char* getString();
	bool isBool();
	bool isFloat();
	bool isInt();
	bool isStr();
	bool isNil();
	std::string print();
private:
	int _type;
	void* _data;
	int _dataLen;
};

class LuaWriter {
public:
	LuaWriter(const std::string& tableName, const std::string& outfile);
	~LuaWriter();
	void write(const std::vector<std::vector<ExcelCell*>>& dataList,
		const std::vector<std::string>& headerList,
		const std::vector<std::string>& typeList,
		const std::vector<bool>& exportList);
	std::string writeRow(const std::vector<ExcelCell*>& rowList,
		const std::vector<std::string>& headerList,
		const std::vector<std::string>& typeList,
		const std::vector<bool>& exportList);
	std::string writeField(const std::string& field, CellValue* value, const std::string& fieldType);
	std::string toString(CellValue* value);
	int toNumber(CellValue* value);
private:
	std::string _outfile;
	std::string _tableName;
};


static const char* IncludeH = R"(
#include <string>
#include <unordered_map>
#include "luaext/GameLua.h"
#include "luaext/LuaWrapper.h"
#include "config.h"

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
)";

static const char* IncludeCpp = R"(
#include "#configname.h"
)";

static const char* ItemNumberDef =R"(
	#type #name;
)";

static const char* ItemDef = R"(
struct #confignameItem{
#content
};
)";

static const char* ConfigDef = R"(
class #configname {
public:
	static #confignameItem* Get(#type index);
	static void Clear();
private:
	static std::unordered_map<#type, #confignameItem*> _items;
};
)";

static const char* ConfigUnpack = R"(
	Lua_GetField(L, -1, "#fieldname");
	Lua_Unpack(L, item->#fieldname);
)";

static const char* ConfigComp = R"(
std::unordered_map<#type, #confignameItem*> #configname::_items = {};
#confignameItem* #configname::Get(#type index) {
	auto iter = _items.find(index);
	if (iter != _items.end()) {
		return iter->second;
	}
	lua_State* L = GameLua::GetInstance()->GetLuaState();
	int ret = Lua_GlobalTop(L, "#configname", index);
	if (ret != 2) {
		lua_pop(L, ret);
		return NULL;
	}
	#confignameItem* item = new #confignameItem;
	#content
	lua_pop(L, ret);
	_items.insert(std::make_pair(index, item));
	return item;
}
void #configname::Clear() {
	for (auto iter = _items.begin(); iter != _items.end(); ++iter) {
		delete iter->second;
	}
	_items.clear();
}
)";

class CppParser {
public:
	CppParser(const std::string& tableName, const std::string& outfile);
	~CppParser();
	void write(const std::vector<std::string>& headerList,
		const std::vector<std::string>& typeList,
		const std::vector<bool>& exportList);
private:
	std::string _outfile;
	std::string _tableName;
};