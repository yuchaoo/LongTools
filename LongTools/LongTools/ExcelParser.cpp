#include <codecvt>
#include <sstream> 
#include "ExcelParser.h"

std::string wstr2str(const std::wstring& str) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.to_bytes(str);
}

std::wstring str2wstr(const std::string& str) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(str);
}

string GbkToUtf8(const std::string& strGBK)
{
   string strOutUTF8 = "";
   if (strGBK.empty()) {
	   return "";
   }

   WCHAR * str1;
   int n = MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, NULL, 0);
   str1 = new WCHAR[n];
   MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, str1, n);
   n = WideCharToMultiByte(CP_UTF8, 0, str1, -1, NULL, 0, NULL, NULL);
   char* str2 = new char[n];
   WideCharToMultiByte(CP_UTF8, 0, str1, -1, str2, n, NULL, NULL);
   strOutUTF8 = str2;
   delete[]str1;
   delete[]str2;
   return strOutUTF8;
}

string Utf8ToGbk(const std::string& strUTF8)
{
    string strOutGBK = "";
	if (strUTF8.empty()) {
		return "";
	}

    WCHAR * str1;
    int n = MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, NULL, 0);
    str1 = new WCHAR[n];
    MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, str1, n);
    n = WideCharToMultiByte(CP_ACP, 0, str1, -1, NULL, 0, NULL, NULL);
    char* str2 = new char[n];
    WideCharToMultiByte(CP_ACP, 0, str1, -1, str2, n, NULL, NULL);
    strOutGBK = str2;
    delete[]str1;
    delete[]str2;
    return strOutGBK;
}

ExcelParser::ExcelParser(const std::string& input, const std::string& clientOutput, const std::string& serverOutput, const std::string& exportFlag)
:TraverseDir(input)
,_clientOutput(clientOutput)
,_serverOutput(serverOutput)
,_exportFlag(exportFlag){

}

ExcelParser::~ExcelParser() {

}

bool ExcelParser::handleFile(const std::string& file, const std::string& root, const std::string& relateDir) {
	Log("handler file:%s",file.c_str());
	ExcelBook book(file);
	if (!book.init()) {
		Log("init excel book failed");
		return false;
	}
	const std::vector<ExcelSheet*>& sheets = book.getSheets();
	for (size_t i = 0; i < sheets.size(); ++i) {
		exportSheet(sheets[i]);
	}
	return true;
}

void ExcelParser::exportSheet(ExcelSheet* sheet) {
	const std::string& name = sheet->getName();
	std::string lowerName = name;
	std::transform(name.begin(), name.end(), lowerName.begin(),std::tolower);
	if (lowerName.find("config") == -1) {
		Log("skip the sheet:%s",name.c_str());
		return;
	}
	if (_exportFlag == CLIENT_FLAG || _exportFlag == ALL_FLAG) {
		std::string outfile = _clientOutput + "/" + name + ".lua";
		LuaWriter clientWriter(name, outfile);
		clientWriter.write(sheet->getDataList(), sheet->getHeaderList(), sheet->getTypeList(), sheet->getClientExportList());
	}
	if (_exportFlag == SERVER_FLAG || _exportFlag == ALL_FLAG) {
		std::string outfile = _serverOutput + "/" + name + ".lua";
		LuaWriter serverWriter(name, outfile);
		serverWriter.write(sheet->getDataList(), sheet->getHeaderList(), sheet->getTypeList(), sheet->getServerExportList());
	}
}


/********************************************/

ExcelBook::ExcelBook(const std::string& file)
	:_file(file){
	_book = xlCreateXMLBook();
	_book->setKey("TommoT", "windows-2421220b07c2e10a6eb96768a2p7r6gc");
}

ExcelBook::~ExcelBook() {
	delete _book;
}

bool ExcelBook::init() {
	if (!_book) {
		Log("book is null");
		return false;
	}

	if (!_book->load(_file.c_str())) {
		Log("load excel file : %s failed, msg:%s",_file.c_str(), _book->errorMessage());
		return false;
	}

	_sheetCount = _book->sheetCount();
	_sheets.resize(_sheetCount);
	for (size_t i = 0; i < _sheetCount; ++i) {
		ExcelSheet* sheet = new ExcelSheet(_book, i);
		if (!sheet->init()) {
			Log("init sheet failed, index:%d", i);
			delete sheet;
			continue;
		}
		_sheets[i] = sheet;
	}

	return true;
}

/************************************************/

ExcelSheet::ExcelSheet(libxl::Book* book, int index)
:_book(book)
,_sheetIndex(index)
,_sheet(NULL)
, _dataList(NULL)
, _clientColor(libxl::Color::COLOR_WHITE)
,_serverColor(libxl::Color::COLOR_WHITE){

}

ExcelSheet::~ExcelSheet() {
	for (size_t i = 0; i < _dataList.size(); ++i) {
		for (size_t j = 0; j < _dataList[i].size(); ++j) {
			delete _dataList[i][j];
		}
	}
}

bool ExcelSheet::init() {
	_sheet = _book->getSheet(_sheetIndex);
	if (!_sheet) {
		Log("get excel book sheet failed, index:%d",_sheetIndex);
		return false;
	}
	_name = _book->getSheetName(_sheetIndex) ; 
	_firstRow = _sheet->firstRow();
	_lastRow = _sheet->lastRow();
	_firstCol = _sheet->firstCol();
	_lastCol = _sheet->lastCol();

	int row = _lastRow - _firstRow ;
	int col = _lastCol - _firstCol ;
	//解析导出标志

	int curRow = _firstRow;
	ExcelCell flagCell1(_sheet, curRow, 0);
	std::string flagStr1 = flagCell1.readValue()->getString();
	libxl::Color flagColor1 = flagCell1.getColor();
	setExportColor(flagColor1, flagStr1);

	curRow++;
	ExcelCell flagCell2(_sheet, curRow, 0);
	std::string flagStr2 = flagCell2.readValue()->getString();
	libxl::Color flagColor2 = flagCell2.getColor();
	setExportColor(flagColor2, flagStr2);
	
	curRow++;
	ExcelCell flagCell3(_sheet, curRow, 0);
	std::string flagStr3 = flagCell3.readValue()->getString();
	libxl::Color flagColor3 = flagCell3.getColor();
	setExportColor(flagColor3, flagStr3);

	//解析导出头
	curRow += 2;
	_fieldTypes.resize(col);
	_fieldHeader.resize(col);
	_clientExportFlag.resize(col);
	_serverExportFlag.resize(col);

	for (size_t i = 0; i < col; ++i) {
		ExcelCell cell(_sheet, curRow, i);
		_fieldHeader[i] = cell.readValue()->getString();
		libxl::Color color = cell.getColor();
		_clientExportFlag[i] = (color == _clientColor || color == _allColor);
		_serverExportFlag[i] = (color == _serverColor || color == _allColor);
	}

	//解析类型
	curRow++;
	for (size_t i = 0; i < col; ++i) {
		ExcelCell cell(_sheet, curRow, i);
		_fieldTypes[i] = cell.readValue()->getString();
	}

	//解析数据
	curRow++;
	_dataList.resize(_lastRow - curRow);
	for (size_t i = curRow; i < _lastRow; ++i) {
		int rowindex = i - curRow;
		auto& list = _dataList[rowindex];
		list.resize(col);
		for (size_t j = 0; j < col; ++j) {
			ExcelCell* cell = new ExcelCell(_sheet, i, _firstCol + j);
			cell->readValue();
			list[j] = cell;
		}
	}
	
	return true;
}

void ExcelSheet::setExportColor(libxl::Color color, const std::string& str) {
	if (str == SERVER_FLAG) {
		_serverColor = color;
	}
	if (str == CLIENT_FLAG) {
		_clientColor = color;
	}
	if (str == ALL_FLAG) {
		_allColor = color;
	}
}

ExcelCell::ExcelCell(libxl::Sheet* sheet, int row, int col)
:_sheet(sheet)
,_row(row)
,_col(col)
, _cellType(libxl::CellType::CELLTYPE_EMPTY)
,_color(libxl::Color::COLOR_WHITE){
	_cellValue = new CellValue();
}

ExcelCell::~ExcelCell() {
	delete _cellValue;
}

CellValue* ExcelCell::readValue() {
	_cellType = _sheet->cellType(_row, _col);
	libxl::Format* format = _sheet->cellFormat(_row, _col);

	switch (_cellType)
	{
	case libxl::CellType::CELLTYPE_BOOLEAN: {
		bool ret = _sheet->readBool(_row, _col);
		_cellValue->setBool(ret);
		break;
	}
	case libxl::CellType::CELLTYPE_NUMBER: {
		double ret = _sheet->readNum(_row, _col);
		libxl::NumFormat numft = libxl::NumFormat::NUMFORMAT_NUMBER;
		if (format) {
			numft = (libxl::NumFormat)format->numFormat();
		}
		if (numft == libxl::NumFormat::NUMFORMAT_NUMBER || numft == libxl::NumFormat::NUMFORMAT_NUMBER_SEP || numft == libxl::NumFormat::NUMFORMAT_ACCOUNT) {
			_cellValue->setInt((int)ret);
		}
		else if (numft == libxl::NumFormat::NUMFORMAT_NUMBER_D2 || numft == libxl::NumFormat::NUMFORMAT_NUMBER_SEP_D2) {
			_cellValue->setFloat(float(ret));
		}
		else if (numft == libxl::NumFormat::NUMFORMAT_PERCENT || numft == libxl::NumFormat::NUMFORMAT_PERCENT_D2) {
			_cellValue->setFloat(float(ret / 100));
		}
		else if (numft == libxl::NumFormat::NUMFORMAT_TEXT) {
			std::string ss = GbkToUtf8(_sheet->readStr(_row, _col));
			_cellValue->setString(ss.c_str(), ss.length());
		}
		else {
			_cellValue->setInt((int)ret);
		}
		break;
	}
	case libxl::CellType::CELLTYPE_STRING: {
		std::string ss = GbkToUtf8(_sheet->readStr(_row, _col));
		_cellValue->setString(ss.c_str(),ss.length());
		break;
	}
	default:
		break;
	}

	if (format) {
		_color = format->patternForegroundColor();
	}

	return _cellValue;
}

/**************************************************/

CellValue::CellValue()
:_type(NIL)
,_data(NULL)
,_dataLen(0){
	
}

CellValue::~CellValue() {
	if (_data) {
		delete[] _data;
	}
}

void CellValue::setBool(bool v) {
	if (_dataLen < sizeof(bool)) {
		_data = (char*)realloc(_data, sizeof(bool));
		_dataLen = sizeof(bool);
	}
	*(bool*)_data = v;
	_type = BOOL;
}

void CellValue::setFloat(float v) {
	if (_dataLen < sizeof(float)) {
		_data = (char*)realloc(_data, sizeof(float));
		_dataLen = sizeof(float);
		memset(_data, 0, _dataLen);
	}
	*(float*)_data = v;
	_type = FLOAT;
}

void CellValue::setInt(int v) {
	if (_dataLen < sizeof(int)) {
		_data = (char*)realloc(_data, sizeof(int));
		_dataLen = sizeof(int);
		memset(_data, 0, _dataLen);
	}
	*(int*)_data = v;
	_type = INT;
}

void CellValue::setString(const char* str, size_t len) {
	if (_dataLen < len + 1) {
		_data = (char*)realloc(_data, len + 1);
		_dataLen = len + 1;
	}
	memset(_data, 0, _dataLen);
	strncpy((char*)_data, str, len);
	_type = STRING;
}

bool CellValue::getBool() {
	if (_data && _type == BOOL) {
		return *(bool*)_data;
	}
	return false;
}

float CellValue::getFloat() {
	if (_data && _type == FLOAT) {
		return *(float*)_data;
	}
	return 0;
}

int CellValue::getInt() {
	if (_data && _type == INT) {
		return *(int*)_data;
	}
	return 0;
}

const char* CellValue::getString() {
	if (_data && _type == STRING) {
		return (const char*)_data;
	}
	return "";
}

bool CellValue::isBool() {
	return _type == BOOL;
}

bool CellValue::isFloat() {
	return _type == FLOAT;
}

bool CellValue::isInt() {
	return _type == INT;
}

bool CellValue::isStr() {
	return _type == STRING;
}

bool CellValue::isNil() {
	return _type == NIL;
}

std::string CellValue::print() {
	char text[256] = {0};
	if (_type == BOOL) {
		return getBool() ? "true" : "false";
	}
	else if (_type == INT) {
		sprintf(text,"%d",getInt());
	}
	else if (_type == FLOAT) {
		sprintf(text,"%f", getFloat());
	}
	else if (_type == STRING) {
		sprintf(text,"%s", getString());
	}
	else if (_type == NIL) {
		return "nil";
	}
	return text;
}

/***********************************************/

LuaWriter::LuaWriter(const std::string& tableName, const std::string& outfile)
:_outfile(outfile)
,_tableName(tableName){

}

LuaWriter::~LuaWriter() {

}

void LuaWriter::write(const std::vector<std::vector<ExcelCell*>>& dataList,
	const std::vector<std::string>& headerList,
	const std::vector<std::string>& typeList,
	const std::vector<bool>& exportList) {
	std::stringstream ss;
	if (_tableName.empty() || _outfile.empty()) {
		Log("the table name or outfile is empty");
		return;
	}

	ss << _tableName << " = {\n";
	for (size_t i = 0; i < dataList.size(); ++i) {
		std::string s = writeRow(dataList[i], headerList, typeList, exportList);
		ss << "\t" << s;
		if (i < dataList.size() - 1) {
			ss << ",\n";
		}
	}
	ss << "\n}\n" << "return " << _tableName;

	FILE* file = fopen(_outfile.c_str(), "wb");
	if (!file) {
		Log("open file failed,%s",_outfile.c_str());
		return;
	}
	std::string s = ss.str();
	fwrite(s.c_str(), s.size(), 1, file);
	fclose(file);
}

std::string LuaWriter::writeRow(const std::vector<ExcelCell*>& colList,
	const std::vector<std::string>& headerList,
	const std::vector<std::string>& typeList,
	const std::vector<bool>& exportList) {
	std::stringstream ss;
	
	int count = 0;
	int firstIndex = 0;
	for (size_t i = 0; i < exportList.size(); ++i) {
		if (exportList[i]) {
			if (count == 0) {
				firstIndex = i;
			}
			++count;
		}
	}

	if (count <= 0) {
		return "";
	}

	if (typeList[firstIndex] == TYPE_INT) {
		int key = toNumber(colList[firstIndex]->getValue());
		ss << "[" << key << "]=";
	}
	else if (typeList[firstIndex] == TYPE_STR) {
		std::string key = toString(colList[firstIndex]->getValue());
		ss << key << "=";
	}

	ss << "{";
	int index = 0;
	for (size_t i = 0; i < colList.size(); ++i) {
		if (exportList[i]) {
			ss << writeField(headerList[i], colList[i]->getValue(), typeList[i]);
			if (++index < count) {
				ss << ",";
			}
		}
	}
	ss << "}";
	return ss.str();
}

std::string LuaWriter::toString(CellValue* value) {
	std::stringstream ss;
	if (value->isBool()) {
		ss << value->getBool() ? "true" : "false";
	}
	else if (value->isFloat()) {
		ss << value->getFloat();
	}
	else if (value->isInt()) {
		ss << value->getInt();
	}
	else if (value->isStr()) {
		ss << value->getString();
	}
	return ss.str();
}

int LuaWriter::toNumber(CellValue* value) {
	if (value->isBool()) {
		return value->getBool() ? 1 : 0;
	}
	else if (value->isInt()) {
		return value->getInt();
	}
	else if (value->isStr()) {
		return atoi(value->getString());
	}
	return 0;
}

std::string LuaWriter::writeField(const std::string& field, CellValue* value, const std::string& fieldType) {
	if (value->isNil()) {
		return "";
	}

	std::stringstream ss;

	ss << field << "=";
	if (fieldType == TYPE_INT) {
		if (value->isInt()) {
			ss << value->getInt();
		}
		else if (value->isStr()) {
			ss << value->getString();
		}
		else if (value->isFloat()) {
			ss << (int)value->getFloat();
		}
		else if (value->isBool()) {
			ss << value->getBool() ? 1 : 0;
		}
	}
	else if (fieldType == TYPE_BOOL) {
		if (value->isBool()) {
			ss << value->getBool() ? "true" : "false";
		}
		else {
			ss << value->getString();
		}
	}
	else if (fieldType == TYPE_FLOAT) {
		if (value->isInt()) {
			ss << value->getInt();
		}
		else if (value->isFloat()) {
			ss << value->getFloat();
		}
		else if (value->isStr()) {
			ss << value->getString();
		}
	}
	else if (fieldType == TYPE_STR) {
		if (value->isBool()) {
			ss << "\"" << (value->getBool() ? "true" : "false") << "\"";
		}
		else if (value->isFloat()) {
			ss << "\"" << value->getFloat() << "\"";
		}
		else if (value->isInt()) {
			ss << "\"" << value->getInt() << "\"";
		}
		else if (value->isStr()) {
			ss << "\"" << value->getString() << "\"";
		}
	}
	else if (fieldType == TYPE_TEXT) {
		if (value->isStr()) {
			ss << value->getString();
		}
	}
	return ss.str();
}