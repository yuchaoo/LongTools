#include<WinSock2.h>
#include "TraverseHandler.h"
#include "xxtea.h"
#include "md5.h"
#include "json/rapidjson.h"
#include "json/document.h"
#include "json/stringbuffer.h"
#include "json/prettywriter.h"
#include <thread>
#include "curl/curl.h"
#include <sstream>

#pragma comment(lib,"ws2_32.lib")

#define PNG_BYTES_TO_CHECK 8
const int len = 6;
unsigned char sign[len] = { 0x33, 0xed ,0xf6 ,0x11 ,0xde ,0x6e };
unsigned char key[len] = { 0x16, 0x3e ,0x72 ,0xf6 ,0xa3 ,0xd7 };

void Log(const char* format,...) {
	static char text[256];
	va_list args;
	va_start(args, format);
	vsnprintf(text, sizeof(text), format, args);
	va_end(args);
	printf("%s\n", text);
}

void split(const std::string& str, char c, std::vector<std::string>& result) {
	if (str.empty()) {
		return;
	}

	size_t pos = str.find(c);
	size_t pre = 0;
	while (pos != -1) {
		result.push_back(str.substr(pre, pos - pre));
		pre = pos + 1;
		pos = str.find(c, pre);
	}
	result.push_back(str.substr(pre));
}

std::string randomString(int minLen, int maxLen) {
	char text[128];
	int l = rand() % (maxLen - minLen) + minLen;
	l = min(l, sizeof(text));

	for (int i = 0; i < l; ++i) {
		text[i] = rand() % 256;
	}
	text[l] = 0;
	return std::string(text, len);
}

std::string randomVariableName(int minLen, int maxLen) {
	char text[128];
	int l = rand() % (maxLen - minLen + 1) + minLen;
	l = min(l, sizeof(text));

	const char mask[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

	for (size_t i = 0; i < l; ++i) {
		int index = rand() % strlen(mask);
		text[i] = mask[index];
	}
	text[l] = 0;
	return text;
}

int randomNumber(int minNum, int maxNum) {
	if (minNum == maxNum) {
		return minNum;
	}
	int l = maxNum - minNum + 1;
	return minNum + (rand() * (rand() % 9999)) % l;
}

void trim(string& str)
{
	if (str.empty()) {
		return;
	}
	int i = 0, j = 0;
	while (i < str.size()) {
		if (!(str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r')) {
			break;
		}
		++i;
	}
	if (i > 0) {
		for (; i < str.size(); ++i) {
			str[j++] = str[i];
		}
		str.resize(j);
	}
	
	i = str.size() - 1;
	while(i >= 0) {
		if (!(str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r')) {
			break;
		}
		--i;
	}
	if (i < str.size() - 1) {
		str.resize(i + 1);
	}
}

std::string regex_replace(const std::string& input, const std::regex& regex, std::function<std::string(std::smatch const& match, size_t pos)> format) {
	std::ostringstream output;
	std::sregex_iterator begin(input.begin(), input.end(), regex), end, last;
	for (; begin != end; begin++) {
		output << begin->prefix() << format(*begin, begin->position());
		last = begin;
	}
	if (last != end) {
		size_t pos = last->position();
		size_t size = last->str().size();
		output << input.substr(pos + last->str().size());
	}
	else {
		output << input;
	}
	return output.str();
}


TraverseDir::TraverseDir(const std::string& dir)
	:m_dir(dir) {

}

TraverseDir::~TraverseDir() {

}

bool TraverseDir::traverse() {
	if (!init()) {
		printf("traverse dir init failed\n");
		return false;
	}
	srand(time(NULL));
	if (doTraverse(m_dir, "")) {
		traverseEnd();
		return true;
	}
	return false;
}

bool TraverseDir::handleDirectory(const std::string& dir, const std::string& root, const std::string& relateDir) {
	return true;
}

bool TraverseDir::handleFile(const std::string& file, const std::string& root, const std::string& relateDir) {
	return true;
}

bool TraverseDir::doTraverse(const std::string& root, const std::string& relateDir) {
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	char DirSpec[MAX_PATH + 1];

	memset(DirSpec, 0, sizeof(DirSpec));
	strncpy(DirSpec, root.c_str(), root.length());
	strncat(DirSpec, "/*", 2);
	hFind = FindFirstFile(DirSpec, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		printf("Invalid file handle. Error is %u \n", GetLastError());
		return false;
	}

	do
	{
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
			if (strcmp(FindFileData.cFileName, ".") != 0 && strcmp(FindFileData.cFileName, "..") != 0) {
				std::string path = std::string(root) + "/" + FindFileData.cFileName;
				std::string releate = relateDir.empty() ? FindFileData.cFileName : relateDir + "/" + FindFileData.cFileName;
				if (!handleDirectory(path, m_dir, releate)) {
					printf("handle dir:%s failed\n", path.c_str());
					return false;
				}
				if (!doTraverse(path, releate)) {
					FindClose(hFind);
					return false;
				}
				handleDirectoryEnd(path, m_dir, releate);
			}
		}
		else {
			std::string file = std::string(root) + "/" + FindFileData.cFileName;
			std::string releate = relateDir.empty() ? FindFileData.cFileName : relateDir + "/" + FindFileData.cFileName;
			if (!handleFile(file, m_dir, releate)) {
				printf("handle file :%s failed\n", file.c_str());
				FindClose(hFind);
				return false;
			}
		}
	} while (FindNextFile(hFind, &FindFileData) != 0);
	FindClose(hFind);
	return true;
}

void TraverseDir::removeDirectory(const std::string& root) {
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	char DirSpec[MAX_PATH + 1];

	memset(DirSpec, 0, sizeof(DirSpec));
	strncpy(DirSpec, root.c_str(), root.length());
	strncat(DirSpec, "/*", 2);
	hFind = FindFirstFile(DirSpec, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		printf("Invalid file handle. Error is %u \n", GetLastError());
		return;
	}

	do {
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (strcmp(FindFileData.cFileName, ".") != 0 && strcmp(FindFileData.cFileName, "..") != 0) {
				std::string path = std::string(root) + "/" + FindFileData.cFileName;
				removeDirectory(path);
			}
		}
		else {
			std::string file = std::string(root) + "/" + FindFileData.cFileName;
			DeleteFileA(file.c_str());
		}
	} while (FindNextFile(hFind, &FindFileData) != 0);
	FindClose(hFind);
	RemoveDirectory(root.c_str());
}

std::string TraverseDir::getCurrentDirectory() {
	TCHAR exeFullPath[MAX_PATH];
	GetModuleFileName(NULL, exeFullPath, MAX_PATH);
	std::string strFullPath = (std::string)(exeFullPath);
	size_t pos = strFullPath.find_last_of("\\");
	return strFullPath.substr(0, pos);
}

bool TraverseDir::isExistDirectory(const std::string& path) {
	WIN32_FIND_DATA wfd;
	HANDLE hFind = FindFirstFile(path.c_str(), &wfd);
	if (hFind != INVALID_HANDLE_VALUE) {
		FindClose(hFind);
		return true;
	}
	return false;
}

bool TraverseDir::isExistFile(const std::string& path) {
	FILE* file = fopen(path.c_str(), "rb");
	if (!file) {
		return false;
	}
	fclose(file);
	return true;
}

void TraverseDir::checkDirWithFileName(const std::string& file, const std::string& root, const std::string& releate) {
	size_t pos = 0, pre = 0;
	pos = releate.find("/");
	std::string str = root;
	while (pos != -1) {
		std::string sub = releate.substr(pre, pos - pre);
		str += "/" + sub;
		if (!isExistDirectory(str)) {
			CreateDirectory(str.c_str(),NULL);
		}
		pre = pos + 1;
		pos = releate.find("/",pre);
	}
}

std::shared_ptr<char> TraverseDir::getFileData(const char* filepath, size_t& size) {
	FILE* file = fopen(filepath,"rb");
	if (!file) {
		printf("open file %s failed\n",filepath);
		return NULL;
	}
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);

	std::shared_ptr<char> p(new char[size]);
	fread(p.get(), 1, size, file);
	fclose(file);
	return p;
}

void TraverseDir::writeFileData(const std::string& filepath, const unsigned char* data, size_t size) {
	FILE* file = fopen(filepath.c_str(), "wb");
	if (!file) {
		printf("open file failed,%s\n",filepath.c_str());
		return;
	}
	fwrite(data, 1, size, file);
	fclose(file);
}

PatchInfo* TraverseDir::createPatchInfo(const std::string& path, const std::string& name) {
	FILE* file = fopen(path.c_str(), "rb");
	if (!file) {
		printf("add patch info failed -> %s\n", path.c_str());
		return NULL;
	}
	md5_state_t ctx;
	md5_byte_t md5[17];
	memset(md5, 0, sizeof(md5));

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* buffer = new char[size];
	fread(buffer, 1, size, file);

	md5_init(&ctx);
	md5_append(&ctx, (unsigned char *)buffer, size);
	md5_finish(&ctx, md5);

	char md5str[128];
	memset(md5str, 0, sizeof(md5str));
	for (int i = 0; i < 16; i++) {
		sprintf(md5str + i * 2, "%02X", md5[i]);
	}
	PatchInfo* info = new PatchInfo;
	info->name = name;
	info->md5 = (char*)md5str;
	info->size = size;

	delete[] buffer;
	fclose(file);
	return info;
}

std::string TraverseDir::getSuffix(const std::string& name) {
	return name.substr(name.rfind(".") + 1);
}

std::string TraverseDir::getMd5(const std::string& file) {
	md5_state_t ctx;
	md5_byte_t md5[17];
	memset(md5, 0, sizeof(md5));

	FILE* fp = fopen(file.c_str(), "rb");
	if (fp == NULL) {
		Log("open file failed, %s",file.c_str());
		return "";
	}

	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char* buffer = new char[size];
	fread(buffer, 1, size, fp);

	md5_init(&ctx);
	md5_append(&ctx, (unsigned char*)buffer, size);
	md5_finish(&ctx, md5);

	char md5str[128];
	memset(md5str, 0, sizeof(md5str));
	for (int i = 0; i < 16; i++) {
		sprintf(md5str + i * 2, "%02X", md5[i]);
	}
	fclose(fp);
	return md5str;
}

/**************************************************************/

EncryptHandler::EncryptHandler(const std::string& dir, const std::string& type)
:TraverseDir(dir){
	if (!type.empty()) {
		size_t pos = type.find("@");
		size_t pre = 0;
		while (pos != -1) {
			m_types.push_back(type.substr(pre, pos - pre));
			pre = pos + 1;
			pos = type.find("@",pre);
		}
		m_types.push_back(type.substr(pre));
	}
}

EncryptHandler::~EncryptHandler() {

}

bool EncryptHandler::traverse() {
	srand(time(NULL));
	return TraverseDir::traverse();
}

bool EncryptHandler::handleFile(const std::string& file, const std::string& root, const std::string& releate) {
	if (isNeedEncrypt(file.c_str())) {
		encryptFile(file.c_str());
	}
	return true;
}

void EncryptHandler::skipBOM(const char*& chunk, int& chunkSize){
	if (static_cast<unsigned char>(chunk[0]) == 0xEF &&
		static_cast<unsigned char>(chunk[1]) == 0xBB &&
		static_cast<unsigned char>(chunk[2]) == 0xBF){
		chunk += 3;
		chunkSize -= 3;
	}
}

void EncryptHandler::encryptFile(const char* path) {
	printf("encrypt file : %s\n", path);
	size_t size = 0;
	unsigned char* data = NULL;

	FILE* file = fopen(path, "rb");
	if (!file)
		return;

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	data = new unsigned char[size];
	fread(data, size, 1, file);
	if (memcmp(data, sign, sizeof(sign)) == 0) {
		printf("file:%s have encrypted...\n", path);
		fclose(file);
		delete[] data;
		return;
	}
	fclose(file);

	file = fopen(path, "wb");
	xxtea_long retSize = 0;

	std::string randomKey = getKeyByRandom(16);

	unsigned char* retData = xxtea_encrypt(data, size, (unsigned char*)randomKey.c_str(), randomKey.length(), &retSize);
	if (retData) {
		fwrite(sign, sizeof(key), 1, file);
		fwrite(randomKey.c_str(), randomKey.length(), 1, file);
		fwrite(retData, retSize, 1, file);
		free(retData);
	}
	fclose(file);
	delete[] data;
}

std::string EncryptHandler::getKeyByRandom(int len) {
	char text[128];
	for (int i = 0; i < len; ++i) {
		text[i] = rand() % 256;
	}
	text[len] = 0;
	return std::string(text, len);
}

void EncryptHandler::decryptFile(const char* path) {
	size_t size = 0;
	unsigned char* data = NULL;

	FILE* file = fopen(path, "rb");
	if (!file)
		return;

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	data = new unsigned char[size];
	fread(data, size, 1, file);
	if (memcmp(data, sign, sizeof(sign)) != 0) {
		fclose(file);
		delete[] data;
		return;
	}
	fclose(file);

	file = fopen(path, "wb");
	xxtea_long retSize = 0;

	std::string randomKey((char*)data + sizeof(sign), 16);
	unsigned char* retData = xxtea_decrypt(data + sizeof(sign), size - sizeof(sign), (unsigned char*)randomKey.c_str(), randomKey.length(), &retSize);
	if (retData) {
		fwrite(retData, retSize, 1, file);
		free(retData);
	}
	fclose(file);
	delete[] data;
}

bool EncryptHandler::isLuaFile(const char* filename) {
	std::string file(filename);
	size_t pos = file.rfind(".");
	if (pos != std::string::npos) {
		if (file.substr(pos) == ".lua")
			return true;
	}
	return false;
}

bool EncryptHandler::isNeedEncrypt(const char* filename) {
	if (m_types.empty()) {
		return true;
	}

	std::string file(filename);
	size_t pos = file.rfind(".");
	if (pos != std::string::npos) {
		std::string suffix = file.substr(pos+1);
		for (size_t i = 0; i < m_types.size(); ++i) {
			if (m_types[i] == suffix) {
				return true;
			}
		}
	}
	return false;
}

/***********************************************************/

CreatePatchHandler::CreatePatchHandler(const std::string& dir, const std::string& version, const std::string& lang)
:TraverseDir(dir)
, m_curLang(lang){
	char text[256];
	m_version = version;
	std::string exeDir = getCurrentDirectory();
	if (lang.empty()) {
		sprintf(text, "%s/version-%s", exeDir.c_str(), version.c_str());
	}
	else {
		sprintf(text, "%s/version-%s-%s", exeDir.c_str(), version.c_str(),lang.c_str());
	}
	m_versionDir = text;
	if (isExistDirectory(m_versionDir)) {
		removeDirectory(m_versionDir);
	}
	CreateDirectory(m_versionDir.c_str(), NULL);
}

CreatePatchHandler::~CreatePatchHandler() {
	for (auto iter = m_allPatchs.begin(); iter != m_allPatchs.end(); ++iter) {
		delete *iter;
	}
}

bool CreatePatchHandler::handleDirectory(const std::string& dir, const std::string& root, const std::string& releate) {
	std::string temDir = m_versionDir + "/" + releate;
	std::string lang = getFileLang(releate);
	if (m_curLang.empty() || lang.empty() || m_curLang == lang) {
		if (!isExistDirectory(temDir)) {
			CreateDirectory(temDir.c_str(), NULL);
		}
		if (!lang.empty()) {
			auto iter = m_langMap.find(lang);
			if (iter == m_langMap.end()) {
				m_langMap.insert(std::make_pair(lang, std::vector<PatchInfo*>()));
			}
		}
	}
	return true;
}

std::string CreatePatchHandler::getFileLang(const std::string& name) {
	const char* langDir = "res/lang/";
	size_t pos1 = name.find(langDir);
	if (pos1 != string::npos) {
		pos1 = pos1 + strlen(langDir);
		size_t pos2 = name.find("/", pos1);
		if (pos2 != std::string::npos) {
			std::string lang = name.substr(pos1, pos2 - pos1);
			return lang;
		}
		else {
			std::string lang = name.substr(pos1);
			return lang;
		}
	}
	return "";
}

bool CreatePatchHandler::handleFile(const std::string& file, const std::string& root, const std::string& releate) {
	if (releate == "res/version.json" || releate == "res/langcfg.json") {
		return true;
	}
	std::string tmpFile = m_versionDir + "/" + releate;
	PatchInfo* info = createPatchInfo(file, releate);
	if (!info) {
		printf("create patch %s failed\n", file.c_str());
		return false;
	}
	m_allPatchs.push_back(info);
	printf("copy file->%s\n", releate.c_str());

	std::string lang = getFileLang(releate);
	if (!m_curLang.empty()) {  //打游戏包
		size_t pos = tmpFile.rfind('.');
		if (pos != std::string::npos) {
			std::string suffix = tmpFile.substr(pos);
			if (suffix == ".png" || suffix == ".jpg") {
				tmpFile = tmpFile.substr(0, pos) + ".xyz";
			}
		}
		if (lang.empty()) {
			m_patchs.push_back(info);
			BOOL ret = CopyFile(file.c_str(), tmpFile.c_str(), false);
			if (!ret) {
				printf("copy file %s failed\n", file.c_str());
				return false;
			}
		}else if (lang == m_curLang) {
			auto iter = m_langMap.find(lang);
			iter->second.push_back(info);
			BOOL ret = CopyFile(file.c_str(), tmpFile.c_str(), false);
			if (!ret) {
				printf("copy file %s failed\n",file.c_str());
				return false;
			}
		}
	}
	else {//打patch包
		if (!lang.empty()) {
			auto iter = m_langMap.find(lang);
			iter->second.push_back(info);
		}
		else {
			m_patchs.push_back(info);
		}
		BOOL ret = CopyFile(file.c_str(), tmpFile.c_str(), false);
		if (!ret) {
			printf("copy file %s failed\n", file.c_str());
			return false;
		}
	}
	return true;
}



bool CreatePatchHandler::createPatchFile() {
	std::string versionFile;
	if (!m_curLang.empty()) {
		versionFile = m_versionDir + "/res/version.json";
	}
	else {
		versionFile = m_versionDir + "/version.json";
	}

	rapidjson::Document doc(rapidjson::kObjectType);
	doc.AddMember("version", m_version,doc.GetAllocator());
	rapidjson::Value files(rapidjson::kArrayType);
	for (size_t i = 0; i < m_patchs.size(); ++i) {
		rapidjson::Value v(rapidjson::kObjectType);
		v.AddMember("name", m_patchs[i]->name, doc.GetAllocator());
		v.AddMember("md5", m_patchs[i]->md5, doc.GetAllocator());
		v.AddMember("size", m_patchs[i]->size, doc.GetAllocator());
		files.PushBack(v, doc.GetAllocator());
	}
	doc.AddMember("files", files, doc.GetAllocator());
	rapidjson::StringBuffer buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);
	
	FILE* file = fopen(versionFile.c_str(), "w");
	if (!file) {
		printf("open file : %s failed", versionFile.c_str());
		return false;
	}
	fwrite(buffer.GetString(), 1, buffer.GetSize(), file);
	fclose(file);
	printf("create: %s secceed\n", versionFile.c_str());
	
	return true;
}

bool CreatePatchHandler::createLangConfigFile() {
	std::string langcfg;
	if (!m_curLang.empty()) {
		langcfg = m_versionDir + "/res/langcfg.json";
	}
	else {
		langcfg = m_versionDir + "/langcfg.json";
	}
	
	rapidjson::Document doc(rapidjson::kObjectType);
	for (auto iter = m_langMap.begin(); iter != m_langMap.end(); ++iter) {
		rapidjson::Value langLabel(rapidjson::kArrayType);
		for (size_t i = 0; i < iter->second.size(); ++i) {
			PatchInfo* info = iter->second[i];
			rapidjson::Value v(rapidjson::kObjectType);
			v.AddMember("name", info->name, doc.GetAllocator());
			v.AddMember("md5", info->md5, doc.GetAllocator());
			v.AddMember("size", info->size, doc.GetAllocator());
			langLabel.PushBack(v,doc.GetAllocator());
		}
		doc.AddMember(rapidjson::Value(iter->first.c_str(),iter->first.length()), langLabel, doc.GetAllocator());
	}
	rapidjson::StringBuffer buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	FILE* file = fopen(langcfg.c_str(), "w");
	if (!file) {
		printf("open file : %s failed", langcfg.c_str());
		return false;
	}
	fwrite(buffer.GetString(), 1, buffer.GetSize(), file);
	fclose(file);
	printf("create %s secceed\n",langcfg.c_str());
	return true;
}

bool CreatePatchHandler::traverse() {
	if (TraverseDir::traverse()) {
		printf("*******************start create patch version file*************\n");
		return createPatchFile() && createLangConfigFile();
	}
	else {
		printf("traverse dir failed\n");
		return false;
	}
}
/****************************************************/


CreateGamePkgHandler::CreateGamePkgHandler(const std::string& dir, std::string& version, const std::string& config, const std::string& lang, const std::string& subVersion, const std::string& flag)
	:TraverseDir(dir)
	, m_config(config)
	, m_version(version)
	, m_lang(lang)
	, m_subVersion(subVersion)
	, m_flag(flag) 
	, m_isAllInMain(false){
	char text[256];
	std::string exeDir = getCurrentDirectory();
	sprintf(text, "%s/version-%s", exeDir.c_str(), version.c_str());
	m_versionPatchDir = text;
	if (isExistDirectory(m_versionPatchDir)) {
		removeDirectory(m_versionPatchDir);
	}
	if (m_flag == "all" || m_flag == "patch") {
		CreateDirectory(m_versionPatchDir.c_str(), NULL);
	}

	sprintf(text, "%s/version-%s-%s", exeDir.c_str(), version.c_str(), lang.c_str());
	m_versionMainDir = text;
	if (isExistDirectory(m_versionMainDir)) {
		removeDirectory(m_versionMainDir);
	}
	if (m_flag == "all" || m_flag == "main") {
		CreateDirectory(m_versionMainDir.c_str(), NULL);
	}

	sprintf(text, "%s/version-%s-%s-sub", exeDir.c_str(), version.c_str(), lang.c_str());
	m_versionSubDir = text;
	if (isExistDirectory(m_versionSubDir)) {
		removeDirectory(m_versionSubDir);
	}
	if (m_flag == "all") {
		CreateDirectory(m_versionSubDir.c_str(), NULL);
	}
}

CreateGamePkgHandler::~CreateGamePkgHandler() {

}

bool CreateGamePkgHandler::init() {
	if (m_config == "") {
		m_isAllInMain = true;
		return true;
	}

	rapidjson::Document doc(rapidjson::kObjectType);
	FILE* file = fopen(m_config.c_str(), "rb");
	if (!file) {
		printf("Can not find the file:%s\n",m_config.c_str());
		return false;
	}
	fseek(file,0, SEEK_END);
	int size = ftell(file);
	fseek(file,0, SEEK_SET);

	std::shared_ptr<char> p( new char[size]);
	memset(p.get(), 0, size);
	fread(p.get(), 1, size, file);
	fclose(file);

	doc.Parse<0>(p.get(), size);
	if (doc.HasParseError()) {
		printf("Parse the json failed, file:%s, offset:%d, errcode:%d\n", m_config.c_str(),doc.GetErrorOffset(),(int)doc.GetParseError());
		fclose(file);
		return false;
	}

	if (doc.HasMember("used")) {
		const rapidjson::Value& usedFiles = doc["used"].GetArray();
		for (auto iter = usedFiles.Begin(); iter != usedFiles.End(); ++iter) {
			m_usedFiles.insert(iter->GetString());
		}
	}
	if (doc.HasMember("useddir")) {
		const rapidjson::Value& usedDir = doc["useddir"].GetArray();
		for (auto iter = usedDir.Begin(); iter != usedDir.End(); ++iter) {
			m_usedDirs.insert(iter->GetString());
		}
	}
	if (doc.HasMember("unuseddir")) {
		const rapidjson::Value& unusedDir = doc["unuseddir"].GetArray();
		for (auto iter = unusedDir.Begin(); iter != unusedDir.End(); ++iter) {
			m_unusedDirs.insert(iter->GetString());
		}
	}
	if (doc.HasMember("unusedfile")) {
		const rapidjson::Value& unusedFile = doc["unusedfile"].GetArray();
		for (auto iter = unusedFile.Begin(); iter != unusedFile.End(); ++iter) {
			m_unusedFiles.insert(iter->GetString());
		}
	}
	if (doc.HasMember("hero")) {
		const rapidjson::Value& heroValue = doc["hero"];
		for (auto iter = heroValue.MemberBegin(); iter != heroValue.MemberEnd(); ++iter) {
			for (auto it = iter->value.MemberBegin(); it != iter->value.MemberEnd(); ++it) {
				auto& files = it->value.GetArray();
				for (auto it = files.Begin(); it != files.End(); ++it) {
					m_unusedFiles.insert(it->GetString());
				}
			}
		}
	}
	return true;
}

bool CreateGamePkgHandler::handleDirectory(const std::string& dir, const std::string& root, const std::string& releate) {
	std::string tempDir = m_versionPatchDir + "/" + releate;
	if (m_flag == "all" || m_flag == "patch") {
		if (!isExistDirectory(tempDir)) {
			CreateDirectory(tempDir.c_str(), NULL);
		}
	}
	
	if (m_flag == "all" || m_flag == "main") {
		std::string langDir = m_versionMainDir + "/" + releate;
		if (!isExistDirectory(langDir)) {
			CreateDirectory(langDir.c_str(), NULL);
		}
	}
	return true;
}

bool CreateGamePkgHandler::handleFile(const std::string& file, const std::string& root, const std::string& releate) {
	if (releate == "res/version.json") {
		return true;
	}
	printf("%s\n", releate.c_str());

	PatchInfo* info = createPatchInfo(file, releate);
	m_allPatchs.push_back(info);
	std::regex reg("res/lang/([\\w-]+)/.+");
	std::cmatch cm;
	if (std::regex_match(releate.c_str(), cm, reg)) {
		std::string lang = cm[1];
		m_langPatchs[lang].push_back(info);

		if (m_flag == "all" || m_flag == "patch") {
			std::string toFile = m_versionPatchDir + "/" + releate;
			BOOL ret = CopyFile(file.c_str(), toFile.c_str(), false);
			if (!ret) {
				printf("copy file failed, file:%s\n", file.c_str());
				return false;
			}
		}

		if ((m_flag == "all" || m_flag == "main") && lang == m_lang) {
			std::string toFile = m_versionMainDir + "/" + releate;
			BOOL ret = CopyFile(file.c_str(), toFile.c_str(), false);
			if (!ret) {
				printf("copy file failed, file:%s\n", file.c_str());
				return false;
			}
		}
	}
	else {
		if (m_flag == "all" || m_flag == "patch") {
			std::string toFile = m_versionPatchDir + "/" + releate;
			if (!CopyFile(file.c_str(), toFile.c_str(), false)) {
				printf("copy file failed, file:%s\n", file.c_str());
				return false;
			}
		}
		
		bool isUsed = m_isAllInMain || isFileUsed(releate);
		if (isUsed) {
			m_mainPkgPatchs.push_back(info);
			if (m_flag == "all" || m_flag == "main") {
				std::string toFile = m_versionMainDir + "/" + releate;
				if (!CopyFile(file.c_str(), toFile.c_str(), false)) {
					printf("copy file failed, file:%s\n", file.c_str());
					return false;
				}
			}
		}
		else {
			m_subPkgPatchs.push_back(info);
			if (m_flag == "all") {
				checkDirWithFileName(file, m_versionSubDir, releate);
				std::string toFile = m_versionSubDir + "/" + releate;
				if (!CopyFile(file.c_str(), toFile.c_str(), false)) {
					printf("copy file failed, file:%s\n", file.c_str());
					return false;
				}
			}
		}
	}
	return true;
}

bool CreateGamePkgHandler::isFileUsed(const std::string& file) {
	if (strncmp(file.c_str(), "src/", 4) == 0) {
		return true;
	}

	auto iter = m_unusedFiles.find(file);
	if (iter != m_unusedFiles.end()) {
		return false;
	}

	std::vector<std::string> dirs;
	size_t pos = file.find("/");
	while (pos != -1) {
		dirs.push_back(file.substr(0, pos));
		pos = file.find("/", pos + 1);
	}

	if (m_unusedDirs.size() > 0) {
		for (size_t i = 0; i < dirs.size(); ++i) {
			auto iter = m_unusedDirs.find(dirs[i]);
			if (iter != m_unusedDirs.end()) {
				return false;
			}
		}
	}
	
	if (m_usedDirs.size() > 0) {
		for (size_t i = 0; i < dirs.size(); ++i) {
			auto iter = m_usedDirs.find(dirs[i]);
			if (iter != m_usedDirs.end()) {
				return true;
			}
		}
	}

	auto it = m_usedFiles.find(file);
	return it != m_usedFiles.end();
}

bool CreateGamePkgHandler::matchHeroResource(const std::string& file) {
	for (auto iter = m_heroRes.regex_list.begin(); iter != m_heroRes.regex_list.end(); ++iter) {
		std::cmatch cm;
		if (std::regex_match(file.c_str(), cm, *iter)) {
			std::string str = cm[1];
			int id = stoi(str.c_str(), 0, 10);
			auto it = m_heroRes.heros.find(id);
			if (it != m_heroRes.heros.end()) {
				return true;
			}
		}
	}
	return false;
}

bool CreateGamePkgHandler::matchStageResource(const std::string& file) {
	for (auto iter = m_stageRes.regex_list.begin(); iter != m_stageRes.regex_list.end(); ++iter) {
		std::cmatch cm;
		if (std::regex_match(file.c_str(), cm, *iter)) {
			std::string str1 = cm[1];
			std::string str2 = cm[2];
			int chapter = stoi(str1.c_str());
			int stage = stoi(str2.c_str());
			if (chapter > m_stageRes.chapterId || (chapter == m_stageRes.chapterId && stage > m_stageRes.stageId)) {
				return true;
			}
		}
	}
	return false;
}

bool CreateGamePkgHandler::matchSystemResource(const std::string& file) {
	for (auto iter = m_sysRes.regex_list.begin(); iter != m_sysRes.regex_list.end(); ++iter) {
		std::cmatch cm;
		if (std::regex_match(file.c_str(), cm, *iter)) {
			std::string str = cm[1];
			auto it = m_sysRes.pkdIds.find(str);
			if (it != m_sysRes.pkdIds.end()) {
				return true;
			}
		}
	}
	return false;
}

bool CreateGamePkgHandler::matchOtherResource(const std::string& file) {
	auto iter = m_usedFiles.find(file);
	return iter != m_usedFiles.end();
}

void CreateGamePkgHandler::addPatchItemConfig(rapidjson::Document& doc, rapidjson::Value& value, std::vector<PatchInfo*>& patchs) {
	for (auto iter = patchs.begin(); iter != patchs.end(); ++iter) {
		rapidjson::Value v(rapidjson::kObjectType);
		v.AddMember("name", (*iter)->name, doc.GetAllocator());
		v.AddMember("md5", (*iter)->md5, doc.GetAllocator());
		v.AddMember("size", (*iter)->size, doc.GetAllocator());
		value.PushBack(v, doc.GetAllocator());
	}
}

bool CreateGamePkgHandler::outputDocument(rapidjson::Document& doc, const std::string& filepath) {
	rapidjson::StringBuffer buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	FILE* file = fopen(filepath.c_str(), "w");
	if (!file) {
		printf("open file : %s failed", filepath.c_str());
		return false;
	}
	fwrite(buffer.GetString(), 1, buffer.GetSize(), file);
	fclose(file);
	printf("create %s secceed\n", filepath.c_str());
	return true;
}

void CreateGamePkgHandler::traverseEnd() {
	rapidjson::Document doc(rapidjson::kObjectType);
	if (m_flag == "all" || m_flag == "patch") {
		std::string versionPatchCfgFile = m_versionPatchDir + "/version.json";
		doc.AddMember("mainPkgVersion", m_version, doc.GetAllocator());
		rapidjson::Value mainFilesValue(rapidjson::kArrayType);
		addPatchItemConfig(doc, mainFilesValue, m_mainPkgPatchs);
		doc.AddMember("mainPkgFiles", mainFilesValue, doc.GetAllocator());
		doc.AddMember("subPkgVersion", m_subVersion, doc.GetAllocator());
		rapidjson::Value subFilesValue(rapidjson::kArrayType);
		addPatchItemConfig(doc, subFilesValue, m_subPkgPatchs);
		doc.AddMember("subPkgFiles", subFilesValue, doc.GetAllocator());
		
		rapidjson::Value langValue(rapidjson::kObjectType);
		for (auto iter = m_langPatchs.begin(); iter != m_langPatchs.end(); ++iter) {
			rapidjson::Value v(rapidjson::kArrayType);
			addPatchItemConfig(doc, v, iter->second);
			std::string key = iter->first;
			langValue.AddMember(rapidjson::Value(key.c_str(), key.size(), doc.GetAllocator()), v, doc.GetAllocator());
		}
		doc.AddMember("lang",langValue,doc.GetAllocator());
		outputDocument(doc, versionPatchCfgFile);
	}

	if (m_flag == "all" || m_flag == "main") {
		doc.RemoveAllMembers();
		doc.SetObject();
		std::string versionMainCfgFile = m_versionMainDir + "/res/version.json";
		doc.AddMember("mainPkgVersion", m_version, doc.GetAllocator());
		rapidjson::Value mainFileValue(rapidjson::kArrayType);
		addPatchItemConfig(doc, mainFileValue, m_mainPkgPatchs);
		doc.AddMember("mainPkgFiles", mainFileValue, doc.GetAllocator());
		if (m_subPkgPatchs.size() <= 0) {
			doc.AddMember("subPkgVersion", m_subVersion, doc.GetAllocator());
		}
		else {
			doc.AddMember("subPkgVersion", "1.0.0", doc.GetAllocator());
		}
		
		rapidjson::Value subPkgFiles(rapidjson::kArrayType);
		doc.AddMember("subPkgFiles", subPkgFiles, doc.GetAllocator());

		rapidjson::Value langValue(rapidjson::kObjectType);
		for (auto iter = m_langPatchs.begin(); iter != m_langPatchs.end(); ++iter) {
			if (iter->first == m_lang) {
				rapidjson::Value value(rapidjson::kArrayType);
				addPatchItemConfig(doc, value, iter->second);
				langValue.AddMember(rapidjson::Value(m_lang.c_str(), m_lang.size(), doc.GetAllocator()), value, doc.GetAllocator());
			}
		}
		doc.AddMember("lang", langValue, doc.GetAllocator());
		outputDocument(doc, versionMainCfgFile);
	}
}

CheckDBFileNameHandler::CheckDBFileNameHandler(const std::string& dir)
:TraverseDir(dir){

}

CheckDBFileNameHandler::~CheckDBFileNameHandler() {

}

bool CheckDBFileNameHandler::doTraverse(const std::string& root, const std::string& relateDir) {
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	char DirSpec[MAX_PATH + 1];

	memset(DirSpec, 0, sizeof(DirSpec));
	strncpy(DirSpec, root.c_str(), root.length());
	strncat(DirSpec, "/*", 2);
	hFind = FindFirstFile(DirSpec, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		printf("Invalid file handle. Error is %u \n", GetLastError());
		return false;
	}

	do
	{
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (strcmp(FindFileData.cFileName, ".") != 0 && strcmp(FindFileData.cFileName, "..") != 0) {
				std::string str = std::string(FindFileData.cFileName) + "/";
				std::string path = std::string(root) + "/" + FindFileData.cFileName;
				std::string releate = relateDir.empty() ? FindFileData.cFileName : relateDir + "/" + FindFileData.cFileName;
				if (!handleDirectory(path, m_dir, releate)) {
					printf("handle dir:%s failed\n", path.c_str());
					return false;
				}
				if (!doTraverse(path, releate)) {
					FindClose(hFind);
					return false;
				}
			}
		}
		else if (relateDir != "") {
			std::string file = std::string(root) + "/" + FindFileData.cFileName;
			std::string releate = relateDir.empty() ? FindFileData.cFileName : relateDir + "/" + FindFileData.cFileName;
			if (!handleFile(file, m_dir, releate)) {
				printf("handle file :%s failed\n", file.c_str());
				FindClose(hFind);
				return false;
			}
		}
	} while (FindNextFile(hFind, &FindFileData) != 0);
	FindClose(hFind);
	return true;
}

bool CheckDBFileNameHandler::handleDirectory(const std::string& dir, const std::string& root, const std::string& releate) {
	size_t pos = releate.find_last_of("/");
	if (pos == std::string::npos) {
		pos = releate.find_last_of("\\");
	}
	if (pos != std::string::npos) {
		m_lastDir = releate.substr(pos+1);
	}
	else {
		m_lastDir = releate;
	}
	return true;
}

bool CheckDBFileNameHandler::handleFile(const std::string& file, const std::string& root, const std::string& releate) {
	std::vector<std::regex> regs{
		std::regex("(\\w+)_ske.json"),
		std::regex("(\\w+)_tex.json"),
		std::regex("(\\w+)_tex.png")
	};
	size_t pos = releate.find_last_of("/");
	std::string filename = releate.substr(pos + 1);

	bool ret = false;
	bool isAllNotMatch = false;
	for (size_t i = 0; i < regs.size(); ++i) {
		std::cmatch cm;
		if (std::regex_match(filename.c_str(), cm, regs[i])) {
			std::string t = cm[1].str();
			if (t != m_lastDir) {
				ret = true;
				break;
			}
			isAllNotMatch = true;
		}
	}
	if (!isAllNotMatch || ret ) {
		m_files.push_back(releate);
	}
	return true;
}

void CheckDBFileNameHandler::traverseEnd() {
	if (m_files.size() > 0) {
		printf("the file is wrong:\n");
		for (size_t i = 0; i < m_files.size(); ++i) {
			printf("%s\n",m_files[i].c_str());
		}
	}
	else {
		printf("can not find the wrong file name\n");
	}
}

ObbFileCreator::ObbFileCreator(const std::string& dir, const std::string& fileName) 
:TraverseDir(dir)
, m_fileName(fileName){

}

ObbFileCreator::~ObbFileCreator() {

}

bool ObbFileCreator::traverse() {
	if (!init()) {
		printf("traverse dir init failed\n");
		return false;
	}
	m_zipFile = zipOpen64(m_fileName.c_str(), APPEND_STATUS_CREATE);
	if (!m_zipFile) {
		printf("create zip file failed,%s\n", m_fileName.c_str());
		return false;
	}

	if (doTraverse(m_dir, "")) {
		traverseEnd();
		return true;
	}
	else {
		zipClose(m_zipFile,NULL);
		DeleteFile(m_fileName.c_str());
		return false;
	}
	return false;
}

bool ObbFileCreator::handleFile(const std::string& file, const std::string& root, const std::string& releate) {
	std::string filepath = root + "/" + releate;
	printf("zip file:%s\n",releate.c_str());
	FILE* fp = fopen(filepath.c_str(), "rb");
	if (!fp) {
		printf("open the file failed,file:%s",releate.c_str());
		return false;
	}
	fseek(fp, 0,SEEK_END);
	size_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	char* data = new char[size + 1];
	memset(data, 0, size + 1);
	fread(data, 1, size, fp);
	fclose(fp);

	zip_fileinfo zFileInfo = { 0 };
	int ret = zipOpenNewFileInZip(m_zipFile, releate.c_str(), &zFileInfo, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
	if (ret != ZIP_OK) {
		printf("open new file failed,file=%s\n", releate.c_str());
		delete[] data;
		return false;
	}

	ret = zipWriteInFileInZip(m_zipFile, data, size);
	if (ret != ZIP_OK) {
		printf("write new file failed,file=%s", releate.c_str());
		zipCloseFileInZip(m_zipFile);
		delete[] data;
		return false;
	}
	zipCloseFileInZip(m_zipFile);
	delete[] data;
	return true;
}

void ObbFileCreator::traverseEnd() {
	printf("create zip file secceed!\n");
	zipClose(m_zipFile,NULL);
}

CompressPng::CompressPng(const std::string& dir, const std::string& config) 
:TraverseDir(dir)
,m_config(config){

}

CompressPng::~CompressPng() {

}

bool CompressPng::traverse() {
	rapidjson::Document doc(rapidjson::kObjectType);
	size_t size = 0;
	std::shared_ptr<char> p = getFileData(m_config.c_str(), size);
	if (p == NULL) {
		return false;
	}
	doc.Parse<0>(p.get(), size);
	if (doc.HasParseError()) {
		printf("parse file failed, %s\n",m_config.c_str());
		return false;
	}
	if (!doc.HasMember("files")) {
		printf("config has no 'files' field\n");
		return false;
	}
	rapidjson::Value& files = doc["files"];
	for (auto iter = files.MemberBegin(); iter != files.MemberEnd(); ++iter) {
		m_qualityMap[iter->name.GetString()] = iter->value.GetInt();
	}

	m_quality = 80;
	if (doc.HasMember("quality")) {
		m_quality = doc["quality"].GetInt();
	}

	m_speed = 1;
	if (doc.HasMember("speed")) {
		m_speed = doc["speed"].GetInt();
	}
	
	return TraverseDir::traverse();
}

bool CompressPng::handleFile(const std::string& file, const std::string& root, const std::string& releate) {
	std::string s = root + "/" + releate;
	size_t pos = releate.find_last_of(".");
	if (pos != -1 && releate.substr(pos + 1) == "png") {
		char text[256];
		std::string path = root + "/" + releate;
		auto iter = m_qualityMap.find(releate);
		if (iter != m_qualityMap.end()) {
			printf("compress : %s\n", path.c_str());
			int quality = iter->second;
			sprintf(text, "pngquant.exe --force --verbose --ext .png %s", path.c_str());
			system(text);
		}
		else if(m_quality > 0) {
			sprintf(text, "pngquant.exe --force --verbose --ext .png --quality=30-%d --speed=%d %s",m_quality, m_speed,path.c_str());
			system(text);
		}
	}
	return true;
}

CompressFiles::CompressFiles(const std::string& dir, const std::string& filter)
:TraverseDir(dir){
	size_t pre = 0;
	size_t pos = filter.find("+");
	while (pos != -1) {
		m_filters.push_back(filter.substr(pre, pos - pre));
		pre = pos;
		pos = filter.find("+", pos+1);
	}
	m_filters.push_back(filter.substr(pre + 1));
}

CompressFiles::~CompressFiles() {

}

bool CompressFiles::isInFilter(const std::string& releate) {
	size_t pos = releate.rfind(".");
	std::string sub = releate.substr(pos + 1);
	for (size_t i = 0; i < m_filters.size(); ++i) {
		if (m_filters[i] == sub) {
			return true;
		}
	}
	return false;
}

bool CompressFiles::handleFile(const std::string& file, const std::string& root, const std::string& releate) {
	if (!isInFilter(releate)) {
		return true;
	}
	
	std::string path = root + "/" + releate;
	size_t size = 0;
	std::shared_ptr<char> p = getFileData(path.c_str(), size);
	if (p == NULL) {
		return false;
	}
	const unsigned char flag[] = { 0xF1, 0xE4, 0x13, 0x78 };

	if (strncmp(p.get(), (const char*)flag, sizeof(flag)) == 0) {
		return true;
	}

	printf("zip file:%s\n",releate.c_str());
	 
	uLong newSize = compressBound(size);
	std::shared_ptr<char> cp(new char[newSize]);
	compress((unsigned char*)cp.get(), &newSize, (unsigned char*)p.get(), size);
	FILE* fp = fopen(path.c_str(), "wb");
	if (!fp) {
		printf("open file failed,%s\n",releate.c_str());
		return false;
	}
	
	fwrite(flag, 1, sizeof(flag), fp);
	fwrite((char*)&size, 1, sizeof(size), fp);
	fwrite(cp.get(), 1, newSize, fp);
	fclose(fp);
	return true;
}

CompareVersion::CompareVersion(const std::string& input1, const std::string& input2, const std::string& field)
:TraverseDir("")
,m_input1(input1)
,m_input2(input2)
,m_field(field){

}

CompareVersion::~CompareVersion() {

}



void CompareVersion::compare(const char* data, size_t size, std::unordered_map<std::string, PatchInfo* >& patch) {
	rapidjson::Document doc(rapidjson::kObjectType);
	doc.Parse<0>(data, size);
	if (doc.HasParseError()) {
		printf("parse data failed\n");
		return;
	}
	if (doc.HasMember(m_field.c_str())) {
		const rapidjson::Value& value = doc[m_field.c_str()];
		for (auto iter = value.Begin(); iter != value.End(); ++iter) {
			const auto& fileValue = *iter;
			PatchInfo* info = new PatchInfo;
			info->md5 = fileValue["md5"].GetString();
			info->name = fileValue["name"].GetString();
			info->size = fileValue["size"].GetInt();
			patch[info->md5] = info;
		}
	}
}

bool CompareVersion::traverse() {
	size_t size = 0;
	std::unordered_map<std::string, PatchInfo*> patch1,patch2;
	
	auto ptr1 = getFileData(m_input1.c_str(),size);
	compare(ptr1.get(), size, patch1);

	auto ptr2 = getFileData(m_input2.c_str(), size);
	compare(ptr2.get(), size, patch2);

	int count = 0;
	for (auto iter = patch1.begin(); iter != patch1.end(); ++iter) {
		auto it = patch2.find(iter->first);
		if (it == patch2.end()) {
			printf("diff:%s\n",iter->first.c_str());
			count++;
		}
	}
	printf("diff count : %d", count);
	return true;
}

ResPathHandler::ResPathHandler(const std::string& input, const std::string& output, const std::string& charMap, const std::string& filterFile, bool isCreateRedundant)
:TraverseDir(input)
,m_output(output)
,m_charMap(charMap)
, m_filterConfig(filterFile)
,m_isCreateRedundant(isCreateRedundant)
, m_logFile(NULL){

}

ResPathHandler::~ResPathHandler() {

}

bool ResPathHandler::traverse() {
	if (m_charMap.length() != 62) {
		printf("the char map length is not 62\n");
		return false;
	}
	std::unordered_map<char, char> charMap;
	static char A[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456879";
	for (size_t i = 0; i < m_charMap.size(); ++i) {
		char c = m_charMap[i];
		if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) {
			printf("the char is A-Z a-z 0-9\n");
			return false;
		}
		if (charMap.find(c) != charMap.end()) {
			printf("the char map is not right\n");
			return false;
		}
		charMap[c] = A[i];
	}
	
	size_t size = 0;
	auto data = getFileData(m_filterConfig.c_str(), size);
	if (data && size > 0) {
		rapidjson::Document doc(rapidjson::kObjectType);
		doc.Parse<0>(data.get(), size);
		if (doc.HasParseError()) {
			Log("parse filter config file failed,%s",m_filterConfig.c_str());
			return false;
		}
		if (doc.HasMember("dir")) {
			auto& filterDir = doc["dir"];
			for (auto iter = filterDir.Begin(); iter != filterDir.End(); ++iter) {
				m_filterDir.push_back(iter->GetString());
			}
		}
		if (doc.HasMember("file")) {
			auto& filterFile = doc["file"];
			for (auto iter = filterFile.Begin(); iter != filterFile.End(); ++iter) {
				m_filterFile.push_back(iter->GetString());
			}
		}
	}
	if (isExistDirectory(m_output)) {
		removeDirectory(m_output);	
	}
	if (!CreateDirectory(m_output.c_str(), NULL)) {
		Log("create dir failed,%s", m_output.c_str());
		return true;
	}
	srand(time(NULL));
	m_logFile = fopen("./pathmap.txt", "wb");
	if (!m_logFile) {
		Log("open pathmap.txt failed");
		return false;
	}
	return TraverseDir::traverse();
}

std::string ResPathHandler::getFilterDir(const std::string& path) {
	for (size_t i = 0; i < m_filterDir.size(); ++i) {
		size_t pos = path.find(m_filterDir[i].c_str());
		if (pos != -1) {
			return m_filterDir[i];
		}
	}
	return "";
}

std::string ResPathHandler::getFilterFile(const std::string& path) {
	std::string ret = getFilterDir(path);
	if (ret != "") {
		return ret;
	}

	for (size_t i = 0; i < m_filterFile.size(); ++i) {
		if (m_filterFile[i] == path) {
			return m_filterFile[i];
		}
	}
	return "";
}

bool ResPathHandler::handleDirectory(const std::string& dir, const std::string& root, const std::string& releate) {
	std::string filterDir = getFilterDir(releate);
	std::string	tmp = getReplaceReleatePath(releate,filterDir);
	std::string outputDir = m_output + "/" + tmp;

	char text[256];
	int n = snprintf(text, sizeof(text), "%s->%s\n", releate.c_str(), tmp.c_str());
	printf(text);
	if (m_logFile) {
		fwrite(text, 1, n, m_logFile);
	}

	if (!isExistDirectory(outputDir)) {
		if (!CreateDirectory(outputDir.c_str(), NULL)) {
			printf("create directory failed, %s\n",outputDir.c_str());
			return false;
		}
	}
	return true;
}

void ResPathHandler::handleDirectoryEnd(const std::string& dir, const std::string& root, const std::string& releate) {
	if (m_isCreateRedundant) {
		std::string filterDir = getFilterDir(releate);
		std::string	tmp = getReplaceReleatePath(releate, filterDir);
		std::string outputDir = m_output + "/" + tmp;
		createRedundantDir(outputDir);
	}
}

bool ResPathHandler::handleFile(const std::string& file, const std::string& root, const std::string& releate) {
	std::string filterFile = getFilterFile(releate);
	std::string	tmp = getReplaceReleatePath(releate,filterFile);

	char text[256];
	int n = snprintf(text, sizeof(text), "%s->%s\n", releate.c_str(), tmp.c_str());

	printf(text);
	if (m_logFile) {
		fwrite(text, 1, n, m_logFile);
	}

	size_t size = 0;
	std::shared_ptr<char> dataPtr = getFileData(file.c_str(), size);
	std::string outputDir = m_output + "/" + tmp;
	writeFileData(outputDir.c_str(), (unsigned char*)dataPtr.get(), size);
	return true;
}

std::string ResPathHandler::getReplaceReleatePath(const std::string& releate, const std::string& filterPath) {
	std::string tmp = releate;
	size_t endIndex = filterPath.empty() ? releate.size() : filterPath.rfind("/");
	for (size_t i = 0; i < endIndex; ++i) {
		char c = releate[i];
		if (c >= 'A' && c <= 'Z') {
			tmp[i] = m_charMap[size_t(c - 'A')];
		}
		else if (c >= 'a' && c <= 'z') {
			tmp[i] = m_charMap[26 + size_t(c - 'a')];
		}
		else if (c >= '0' && c <= '9') {
			tmp[i] = m_charMap[52 + size_t(c - '0')];
		}
	}
	return tmp;
}

void ResPathHandler::createRedundantDir(std::string path) {
	int count =  rand() % 5 + 2;
	for (size_t i = 0; i < count; ++i) {
		if (rand() % 3 == 0) {
			std::string dir = path + "/" + randomString();
			if (!isExistDirectory(dir)) {
				CreateDirectory(dir.c_str(),NULL);
				int filecount = rand() % 5 + 2;
				for (int j = 0; j < filecount; ++j) {
					createRedundantFile(dir);
				}
			}
		}
		else {
			createRedundantFile(path);
		}
	}
}

void ResPathHandler::createRedundantFile(std::string path) {
	std::string filepath = path + "/" + randomString();
	if (!isExistDirectory(filepath)) {
		FILE* file = fopen(filepath.c_str(), "wb");
		if (file) {
			int size = rand() % 100 + 10;
			std::string data(size, 'a');
			for (int j = 0; j < size; ++j) {
				data[j] = rand() % 256;
			}
			fwrite(data.c_str(), 1, data.size(), file);
			fclose(file);
		}
	}
}

std::string ResPathHandler::randomString() {
	size_t count = rand() % 10 + 3;
	std::string s(count,'a');
	for (size_t i = 0; i < count; ++i) {
		int index = rand() % m_charMap.size();
		s[i] = m_charMap[index];
	}
	return s;
}

void ResPathHandler::traverseEnd() {
	if (m_logFile) {
		fclose(m_logFile);
		m_logFile = NULL;
	}

	TraverseDir::traverseEnd();
}

KeyWorldHandler::KeyWorldHandler(const std::string& input, const std::string& output, const std::string& mapFile, bool bReserveSuffix, bool addRedundant)
:TraverseDir(input)
,m_output(output)
, m_bReserveSuffix(bReserveSuffix)
,m_mapFile(mapFile)
, m_isAddRedundant(addRedundant){

}

KeyWorldHandler::~KeyWorldHandler() {

}

bool KeyWorldHandler::traverse() {
	size_t size = 0;
	auto ptr = getFileData(m_mapFile.c_str(), size);
	if (!ptr) {
		Log("read map file data failed");
		return false;
	}
	rapidjson::Document doc(rapidjson::kObjectType);
	doc.Parse<0>(ptr.get(), size);
	if (doc.HasParseError()) {
		rapidjson::ParseErrorCode code = doc.GetParseError();
		size_t offset = doc.GetErrorOffset();
		Log("parse the map file failed, %d, offset:%d", code,offset);
		return false;
	}

	if (!doc.HasMember("dirwords")) {
		Log("can not find the field of 'dirwords'");
		return false;
	}

	if (!doc.HasMember("filewords")) {
		Log("can not find the field of 'filewords'");
		return false;
	}

	for (auto iter = doc["dirwords"].Begin(); iter != doc["dirwords"].End(); ++iter) {
		m_dirWords.push_back(iter->GetString());
	}

	for (auto iter = doc["filewords"].Begin(); iter != doc["filewords"].End(); ++iter) {
		m_fileWords.push_back(iter->GetString());
	}

	return TraverseDir::traverse();
}

bool KeyWorldHandler::handleDirectory(const std::string& dir, const std::string& root, const std::string& releate) {
	auto iter = m_dirMap.find(releate);
	if (iter == m_dirMap.end()) {
		std::string newdir;
		std::string fullNewdir;

		size_t pos = releate.rfind("/");
		if (pos != -1) {
			std::string predir = releate.substr(0,pos);
			auto it = m_dirMap.find(predir);
			if (it == m_dirMap.end()) {
				Log("can not find the dir:%s", predir.c_str());
				return false;
			}
			predir = it->second;

			newdir = predir + "/" + m_dirWords[rand() % m_dirWords.size()];
			fullNewdir = m_output + "/" + newdir;

			while (isExistDirectory(fullNewdir)) {
				newdir = predir + "/" + m_dirWords[rand() % m_dirWords.size()];
				fullNewdir = m_output + "/" + newdir;
			}
		}
		else {
			newdir = m_dirWords[rand() % m_dirWords.size()];
			fullNewdir = m_output + "/" + newdir;
			while (isExistDirectory(fullNewdir)) {
				newdir =  m_dirWords[rand() % m_dirWords.size()];
				fullNewdir = m_output + "/" + newdir;
			}
		}

		if (!CreateDirectory(fullNewdir.c_str(), NULL)) {
			Log("create dir failed, %s", fullNewdir.c_str());
			return false;
		}
		m_dirMap[releate] = newdir;

		if (m_isAddRedundant) {
			addRedundantFiles(m_output + "/" + newdir);
			int newdirCount = rand() % 3 + 1;
			for (int i = 0; i < newdirCount; ++i) {
				std::string subDir = m_output + "/" + newdir + "/" + m_dirWords[rand() % m_dirWords.size()];
				if (!isExistDirectory(subDir) && CreateDirectory(subDir.c_str(), NULL)) {
					addRedundantFiles(subDir);
				}
			}
		}
	}
	return true;
}

bool KeyWorldHandler::handleFile(const std::string& file, const std::string& root, const std::string& releate) {
	std::string newfile = createFileWithRandom(root, releate);
	if (newfile.empty()) {
		Log("handle file failed:%s",releate.c_str());
		return false;
	}
	m_fileMap[releate] = newfile;
	Log("%s->%s",releate.c_str(), newfile.c_str());
	return true;
}

std::string KeyWorldHandler::createFileWithRandom(const std::string& root, const std::string& releate) {
	std::string newpath;
	std::string suffix = getSuffix(releate);
	size_t pos = releate.rfind("/");
	if (pos != -1) {
		newpath = releate.substr(0, pos);
		auto iter = m_dirMap.find(newpath);
		if (iter == m_dirMap.end()) {
			Log("not find the dir: %s", newpath.c_str());
			return "";
		}
		newpath = iter->second + "/";
	}

	std::string newname = createFileName();
	std::string fullNewName = m_output + "/" + newpath + newname;
	if (m_bReserveSuffix) {
		fullNewName += "." + suffix;
	}
	while (isExistFile(fullNewName)) {
		newname = createFileName();
		fullNewName = m_output + "/" + newpath + newname;
		if (m_bReserveSuffix) {
			fullNewName += "." + suffix;
		}
	}
	
	size_t size = 0;
	std::string file = root + "/" + releate;
	auto dataptr = getFileData(file.c_str(), size);
	if (!dataptr) {
		return "";
	}

	writeFileData(fullNewName, (unsigned char*) dataptr.get(), size);
	std::string key = newpath + newname;
	if (m_bReserveSuffix) {
		key += "." + suffix;
	}
	return key;
}

std::string KeyWorldHandler::createFileName() {
	std::string name;
	int len = 0;
	bool is_ = rand() % 1000 < 500;
	int count = rand() % 3 + 1;
	int num = 0;

	while (len <= 5 || num < count) {
		int index = rand() % m_fileWords.size();
		std::string word = m_fileWords[index];
		if (len > 0 && is_) {
			name += "_" + word;
			len += name.size() + 1;
		}
		else {
			word[0] = toupper(word[0]);
			name += word;
			len += name.size();
		}
		num++;
	}
	return name;
}

void KeyWorldHandler::addRedundantFiles(const std::string& path) {
	int count = rand() % 10 + 1;
	for (int i = 0; i < count; ++i) {
		std::string name = createFileName();

		std::string filepath = path + "/" + name + ".lua";
		FILE* file = fopen(filepath.c_str(), "rb");
		if (file) {
			Log("file %s is exist", filepath.c_str());
			fclose(file);
			continue;
		}

		file = fopen(filepath.c_str(), "wb");
		if (file) {
			int size = rand() % 1000 + 100;
			std::string data(size, 'a');
			for (int j = 0; j < size; ++j) {
				data[j] = rand() % 256;
			}
			fwrite(data.c_str(), 1, data.size(), file);
			fclose(file);
		}
	}
}

void KeyWorldHandler::traverseEnd() {
	std::string mapFile = m_output + "/" + "berlma";
	FILE* file = fopen(mapFile.c_str(), "w");
	if (!file) {
		Log("open the file failed, %s", mapFile.c_str());
		return;
	}

	rapidjson::Document doc(rapidjson::kObjectType);

	for (auto iter = m_fileMap.begin(); iter != m_fileMap.end(); ++iter) {
		doc.AddMember(rapidjson::Value(iter->first.c_str(),iter->first.length()), iter->second, doc.GetAllocator());
	}

	rapidjson::StringBuffer buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	fwrite(buffer.GetString(), 1, buffer.GetSize(), file);

	fclose(file);
}

ModifyMd5Handler::ModifyMd5Handler(const std::string& input, const std::string& filter)
:TraverseDir(input){
	split(filter, '@', m_filters);
}

bool ModifyMd5Handler::isNeedModifyBySuffix(const std::string& suffix) {
	for (size_t i = 0; i < m_filters.size(); ++i) {
		if (m_filters[i] == suffix) {
			return true;
		}
	}
	return false;
}

bool ModifyMd5Handler::handleFile(const std::string& file, const std::string& root, const std::string& releate) {
	std::string suffix = getSuffix(releate);
	if (!isNeedModifyBySuffix(suffix)) {
		return true;
	}
	if (suffix == "png") {
		modifyPngMd5(file,releate);
	}
	if (suffix == "jpg") {
		modifyJpegMd5(file, releate);
	}
	if (suffix == "lua") {
		modifyLuaMd5(file, releate);
	}
	return true;
}

void ModifyMd5Handler::modifyPngMd5(const std::string& file, const std::string& releate) {
	std::string md51 = getMd5(file);

	FILE* fp = fopen(file.c_str(), "rb+");
	if (!fp) {
		return;
	}

	int len = 0, crc = rand() % RAND_MAX;
	fseek(fp, 8, SEEK_CUR);
	fread(&len, 4, 1, fp);
	len = htonl(len);

	fseek(fp, 16 + len, SEEK_CUR);
	fwrite(&crc, 4, 1, fp);

	fseek(fp, 0, SEEK_SET);
	fclose(fp);

	std::string md52 = getMd5(file);

	Log("%s:MD5:%s->%s", releate.c_str(), md51.c_str(), md52.c_str());

	return;
}

void ModifyMd5Handler::modifyLuaMd5(const std::string& file, const std::string& releate) {
	std::string md51 = getMd5(file);
	size_t size = 0;
	auto ptr = getFileData(file.c_str(), size);
	if (size <= 0) {
		Log("read the file failed,file:%s",releate.c_str());
		return;
	}
	unsigned char* data = (unsigned char*)ptr.get();
	unsigned char flag[5] = { 0x1B, 0x4C, 0x4A, 0x02, 0x0A, };
	if (strncmp((const char*)data, (const char*)flag, 5) == 0) {
		Log("the file is luac file,%s",releate.c_str());
		return;
	}

	FILE* fp = fopen(file.c_str(), "wb");
	if (!fp) {
		Log("open file failed, %s", file.c_str());
		return;
	}

	char text[128];
	sprintf(text, "local __%s = %d\n", randomVariableName(1, 10).c_str(), randomNumber(1, 1000000));
	fwrite(text, strlen(text), 1, fp);
	fwrite(ptr.get(), size, 1, fp);

	fclose(fp);

	std::string md52 = getMd5(file);
	Log("%s:MD5:%s->%s", releate.c_str(), md51.c_str(), md52.c_str());
	return ;
}

void ModifyMd5Handler::modifyJpegMd5(const std::string& file, const std::string& releate) {
	std::string md51 = getMd5(file);

	size_t size = 0;
	auto ptr = getFileData(file.c_str(), size);

	if (size <= 0) {
		return;
	}

	unsigned char* data = (unsigned char*)ptr.get();
	unsigned short APP0LEN = (unsigned short)htons(*((unsigned short*) (data + 4)));

	FILE* fp = fopen(file.c_str(), "wb");
	if (!fp) {
		return;
	}

	int APP1OFF = 4 + APP0LEN;
	if (*(data + APP1OFF) == 0xFF && *(data + APP1OFF + 1) >= 0xE1 && *(data + APP1OFF + 1) <= 0xEF && *(data + APP1OFF)) {
		unsigned short len = htons(*(data + APP1OFF + 2));
		if (len > 0) {
			*(data + APP1OFF + 4) = rand() % 255;
			fwrite(data, size, 1, fp);
		}
		else {
			*(unsigned short*)(data + APP1OFF + 2) = htons(1);
			char c = rand() % 255;
			fwrite(data, APP1OFF + 4, 1, fp);
			fwrite(&c, 1, 1, fp);
			fwrite(data + APP1OFF + 4, size - APP1OFF - 4, 1, fp);
		}
	}
	else {
		unsigned char flag[2] = { 0xFF,0xE1 };
		unsigned short len = htons(0);
		unsigned char c = rand() % RAND_MAX;
		fwrite(data, APP1OFF, 1, fp);
		fwrite(flag, 2, 1, fp);
		fwrite(&len, 2, 1, fp);
		fwrite(&c, 1, 1, fp);
		fwrite(data + APP1OFF, size - APP1OFF, 1, fp);
	}

	fclose(fp);

	std::string md52 = getMd5(file);
	Log("%s:MD5:%s->%s", releate.c_str(), md51.c_str(), md52.c_str());

	return;
}


RepClassNameHandler::RepClassNameHandler(const std::string& input, const std::string& cfgfile, const std::string& wordsfile)
:TraverseDir(input)
,m_cfgfile(cfgfile)
, m_wordsfile(wordsfile){

}

RepClassNameHandler::~RepClassNameHandler() {

}

bool RepClassNameHandler::traverse() {
	size_t size = 0;
	auto dataptr = getFileData(m_wordsfile.c_str(), size);
	if (!dataptr) {
		Log("get file data failed, %s", m_wordsfile.c_str());
		return false;
	}
	rapidjson::Document doc(rapidjson::kObjectType);
	doc.Parse<0>(dataptr.get(), size);
	if (doc.HasParseError()) {
		Log("parse file failed, offset:%d",doc.GetErrorOffset());
		return false;
	}
	if (doc.HasMember("dirwords")) {
		auto& dirwords = doc["dirwords"];
		for (auto iter = dirwords.Begin(); iter != dirwords.End(); ++iter) {
			m_worlds.push_back(iter->GetString());
		}
	}
	if (doc.HasMember("filewords")) {
		auto& filewords = doc["filewords"];
		for (auto iter = filewords.Begin(); iter != filewords.End(); ++iter) {
			m_worlds.push_back(iter->GetString());
		}
	}
	if (m_worlds.empty()) {
		Log("the words is empty");
		return false;
	}

	srand(time(NULL));

	dataptr = getFileData(m_cfgfile.c_str(), size);

	rapidjson::Document doc1(rapidjson::kObjectType);
	doc1.Parse<0>(dataptr.get(), size);
	if (doc1.HasParseError()) {
		Log("cfgfile get data failed, offset:%d", doc1.GetErrorOffset());
		return false;
	}
	if (!doc1.HasMember("classname")) {
		Log("cfgfile has no field:'classname'");
		return false;
	}

	auto& classNames = doc1["classname"];
	for (auto iter = classNames.Begin(); iter != classNames.End(); ++iter) {
		m_classNames.push_back(iter->GetString());
	}

	if (!doc1.HasMember("funcname")) {
		Log("cfgfile has not filed:funcname");
		return false;
	}
	auto& funcNames = doc1["funcname"];
	for (auto iter = funcNames.Begin(); iter != funcNames.End(); ++iter) {
		m_funcNames.push_back(iter->GetString());
	}

	if (!doc1.HasMember("luacfunc")) {
		Log("cfgfile has not field:luacfunc");
		return false;
	}
	auto& luacfunc = doc1["luacfunc"];
	for (auto iter = luacfunc.Begin(); iter != luacfunc.End(); ++iter) {
		LuaCFuncRegexInfo info;
		if (iter->HasMember("file") && iter->HasMember("regex")) {
			info.file = (*iter)["file"].GetString();
			info.regex = (*iter)["regex"].GetString();
			m_luacfunc.push_back(info);
		}
		if (iter->HasMember("filter")) {
			auto& filters = (*iter)["filter"];
			for (size_t i = 0; i < filters.Size(); ++i) {
				m_luacFilters.insert(filters[i].GetString());
			}
		}
	}
	
	m_repType = kClassName;
	for (int i = 0; i < m_classNames.size(); ++i) {
		m_curClassName = m_classNames[i];

		m_curRepstr = createClassName();
		while (m_sets.find(m_curRepstr) != m_sets.end()) {
			m_curRepstr = createClassName();
		}
		m_sets.insert(m_curRepstr);
		m_result[m_curClassName] = m_curRepstr;
		Log("%s->%s",m_curClassName.c_str(), m_curRepstr.c_str());
		if (!TraverseDir::traverse()) {
			return false;
		}
	}

	m_repType = kFuncName;
	for (int i = 0; i < m_funcNames.size(); ++i) {
		m_curFuncName = m_funcNames[i];
		m_curRepstr = createFuncName();
		while (m_sets.find(m_curRepstr) != m_sets.end()) {
			m_curRepstr = createFuncName();
		}
		m_sets.insert(m_curRepstr);
		m_result[m_curFuncName] = m_curRepstr;
		Log("%s->%s",m_curFuncName.c_str(), m_curRepstr.c_str());
		if (!TraverseDir::traverse()) {
			return false;
		}
	}

	m_repType = kLuaCFunc;
	for (size_t i = 0; i < m_luacfunc.size(); i++) {
		size_t size = 0;
		auto dataptr = getFileData(m_luacfunc[i].file.c_str(), size);
		if (size > 0) {
			std::string data(dataptr.get(), size);
			replaceLuaCFunName(data, m_luacfunc[i].regex);
			writeFileData(m_luacfunc[i].file,(unsigned char*) data.c_str(), data.size());
		}
	}
	traverseEnd();

	return true;
}

bool RepClassNameHandler::handleFile(const std::string& file, const std::string& root, const std::string& releate) {
	size_t size = 0;
	std::string suffix = getSuffix(releate);
	if (suffix != "cpp" && suffix != "h" && suffix != "c" && suffix != "mm" && suffix != "hpp") {
		return true;
	}
	Log("handler:%s",releate.c_str());

	auto datapre = getFileData(file.c_str(), size);
	if (!datapre) {
		Log("get file data failed,'%s'",releate.c_str());
		return false;
	}

	std::string data(datapre.get(), size);
	if (m_repType == kClassName) {
		replaceClassName(data, m_curClassName, m_curRepstr);
	}
	if (m_repType == kFuncName) {
		replaceFuncName(data, m_curFuncName, m_curRepstr);
	}

	writeFileData(file, (unsigned char*) data.c_str(), data.size());	
	return true;
}

bool checkPrefix(const std::string& data, int index) {
	if (index == 0) {
		return true;
	}
	char c = data[index - 1];
	if (c == '_' || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
		return false;
	}
	if (c == '"') {
		return false;
	}
	return true;
}

bool checkSuffix(const std::string& data, int index) {
	if (index >= data.size() - 1) {
		return true;
	}
	char c = data[index + 1];
	if (c == '_' || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
		return false;
	}
	if (c == '"') {
		return false;
	}
	if (strncmp(data.c_str() + index + 1, ".h", 2) == 0) {
		return false;
	}
	return true;
}

void RepClassNameHandler::replaceClassName(std::string& data, const std::string& classname, const std::string& repstr) {
	static char text[256];
	size_t pos = 0, pre = 0;
	pos = data.find(classname.c_str(), pre);
	while (pos != -1) {
		if (checkPrefix(data, pos) && checkSuffix(data, pos + classname.size() - 1)) {
			data.replace(pos, classname.size(), repstr);
			pre = pos + repstr.size();
		}
		else {
			pre = pos + classname.size();
		}
		pos = data.find(classname, pre);
	}
}

bool checkFunNamePrefix(std::string& data, int index) {
	if (index == 0) {
		return true;
	}
	char c = data[index - 1];
	if (c == '_' || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
		return false;
	}
	if (c == '"') {
		return false;
	}
	return true;
}

bool checkFunNameSubffix(std::string& data, int index) {
	if (index >= data.size() - 1) {
		return true;
	}
	char c = data[index + 1];
	if (c == '_' || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
		return false;
	}
	if (c == '"') {
		return false;
	}
	if (c == '.') {
		return false;
	}
	return true;
}

void RepClassNameHandler::replaceFuncName(std::string& data, const std::string& funcname, const std::string& repstr) {
	size_t pos = 0, pre = 0;
	pos = data.find(funcname.c_str(), pre);
	while (pos != -1) {
		if (checkFunNamePrefix(data, pos) && checkFunNameSubffix(data, pos + funcname.size() - 1)) {
			data.replace(pos, funcname.size(), repstr);
			pre = pos + repstr.size();
		}
		else {
			pre = pos + funcname.size();
		}
		pos = data.find(funcname, pre);
	}
}

std::string RepClassNameHandler::createClassName() {
	int len = 0;
	int num = 0;
	int count = rand() % 3 + 1;
	std::string newname;
	while (len <= 5 || num < count) {
		std::string word = m_worlds[rand() % m_worlds.size()];
		word[0] = std::toupper(word[0]);
		newname += word;
		len+=word.size();
		num++;
	}
	return newname;
}

std::string RepClassNameHandler::createFuncName() {
	int len = 0;
	int num = 0;
	int count = rand() % 4 + 2;
	std::string newname;
	while (len < 10 || num < count) {
		std::string word = m_worlds[rand() % m_worlds.size()];
		word[0] = std::toupper(word[0]);
		newname += word;
		len += word.size();
		num++;
	}
	newname[0] = std::tolower(newname[0]);
	return newname;
}

std::string RepClassNameHandler::createLuaCFuncName() {
	int num = 0;
	int count = rand() % 2 + 4;
	std::string newname;
	while (num < count) {
		std::string word = m_worlds[rand() % m_worlds.size()];
		word[0] = std::toupper(word[0]);
		newname += word;
		num++;
	}
	newname[0] = std::tolower(newname[0]);
	return newname;
}

void RepClassNameHandler::replaceLuaCFunName(std::string& data, const std::string& regexstr) {
	std::function<std::string(std::smatch const& match, size_t pos)> callback = [=](std::smatch const& match, size_t pos)->std::string {
		std::string name;
		for (size_t i = 2; i < match.size() - 1; ++i) {
			name += (i < match.size() - 2) ? (match[i].str() + "_") : match[i].str();
		}
		if (m_luacFilters.find(name) != m_luacFilters.end()) {
			return match[0].str();
		}

		std::string str;
		if (m_result.find(name) != m_result.end()) {
			str = m_result[name];
		}
		else {
			str = createLuaCFuncName();
			while (m_sets.find(str) != m_sets.end()) {
				str = createLuaCFuncName();
			}
			m_sets.insert(str);
			m_result[name] = str;
			Log("%s->%s", name.c_str(), str.c_str());
		}
		return match[1].str() + str + match[match.size() - 1].str();
	};
	std::regex reg(regexstr);
	data = regex_replace(data, reg, callback);
}

void RepClassNameHandler::traverseEnd() {
	FILE* file = fopen("classnamemap.txt", "wb");
	if (!file) {
		Log("open file classnamemap.txt failed");
		return;
	}
	char text[256];
	for (auto iter = m_result.begin(); iter != m_result.end(); ++iter) {
		sprintf(text, "%s->%s\n", iter->first.c_str(), iter->second.c_str());
		fwrite(text, 1, strlen(text), file);
	}
	fclose(file);
}

AddGarbageCode::AddGarbageCode(const std::string& cfgfile, const std::string& wordsfole) 
:TraverseDir("")
,m_cfgFile(cfgfile)
,m_wordFile(wordsfole){
}

AddGarbageCode::~AddGarbageCode() {

}

bool AddGarbageCode::traverse() {
	size_t size = 0;
	auto dataptr = getFileData(m_wordFile.c_str(), size);
	if (!dataptr) {
		Log("get file data failed, %s", m_wordFile.c_str());
		return false;
	}
	rapidjson::Document doc(rapidjson::kObjectType);
	doc.Parse<0>(dataptr.get(), size);
	if (doc.HasParseError()) {
		Log("parse file failed, offset:%d", doc.GetErrorOffset());
		return false;
	}
	if (doc.HasMember("dirwords")) {
		auto& dirwords = doc["dirwords"];
		for (auto iter = dirwords.Begin(); iter != dirwords.End(); ++iter) {
			m_words.push_back(iter->GetString());
		}
	}
	if (doc.HasMember("filewords")) {
		auto& filewords = doc["filewords"];
		for (auto iter = filewords.Begin(); iter != filewords.End(); ++iter) {
			m_words.push_back(iter->GetString());
		}
	}
	if (m_words.empty()) {
		Log("the words is empty");
		return false;
	}

	dataptr = getFileData(m_cfgFile.c_str(), size);
	if (!dataptr) {
		Log("get cfgfile data failed, %s",m_cfgFile.c_str());
		return false;
	}

	rapidjson::Document doc1(rapidjson::kObjectType);
	doc1.Parse<0>(dataptr.get(), size);
	if (doc1.HasParseError()) {
		Log("parse file failed, offset:%d", doc1.GetErrorOffset());
		return false;
	}
	if (!doc1.HasMember("dir") || !doc1.HasMember("file")) {
		Log("cfgfile has not field:dir and file");
		return false;
	}

	auto& dir = doc1["dir"];
	for (auto iter = dir.Begin(); iter != dir.End(); ++iter) {
		m_inputDirs.push_back(iter->GetString());
	}
	auto& file = doc1["file"];
	for (auto iter = file.Begin(); iter != file.End(); ++iter) {
		m_inputFiles.push_back(iter->GetString());
	}

	srand(time(NULL));

	for (size_t i = 0; i < m_inputDirs.size(); ++i) {
		setInputDir(m_inputDirs[i]);
		if (!TraverseDir::traverse()) {
			return false;
		}
	}

	for (size_t i = 0; i < m_inputFiles.size(); ++i) {
		if (!handleCppFile(m_inputFiles[i])) {
			return false;
		}
	}

	return true;
}

bool AddGarbageCode::handleFile(const std::string& file, const std::string& root, const std::string& releate) {
	std::string suffix = getSuffix(releate);
	if (suffix != "cpp" && suffix != "c") {
		return true;
	}
	Log("handler:%s", releate.c_str());
	return handleCppFile(file);
}

bool AddGarbageCode::handleCppFile(const std::string& file) {
	size_t size = 0;
	auto datapre = getFileData(file.c_str(), size);
	if (!datapre) {
		Log("get file data failed,'%s'", file.c_str());
		return false;
	}
	std::string data(datapre.get(), size);
	addGarbageCodeToCPP(data);
	writeFileData(file, (unsigned char*)data.c_str(), data.size());
	return true;
}

std::string AddGarbageCode::createGarbageCode() {
	char text[512] = { 0 };
	int type = rand() % 10;
	if (type == 0) {
		std::string arg1 = "__" + randomVariableName(5, 8);
		std::string arg2 = "__" + randomVariableName(5, 8);
		std::string arg3 = "__" + randomVariableName(5, 8);
		int n = sprintf(text, "\n\t\tint %s = %d;\n", arg1.c_str(), rand() % RAND_MAX);
		n += sprintf(text + n, "\t\tint %s = %d;\n", arg2.c_str(), (rand() % 2 == 0) ? (rand() % RAND_MAX) : (-rand() % RAND_MAX));
		n += sprintf(text + n, "\t\tint %s = %s + %s;\n", arg3.c_str(), arg1.c_str(), arg2.c_str());
		n += sprintf(text + n, "\t\t%s++;\n", arg3.c_str());
		n += sprintf(text + n, "\t\t%s = (%s > %s) ? %s + %s : %s - %s;\n", arg2.c_str(), arg3.c_str(), arg1.c_str(), arg3.c_str(), arg1.c_str(), arg3.c_str(), arg1.c_str());
	}
	else if (type == 1) {
		std::string arg1 = "__" + randomVariableName(4, 9);
		std::string arg2 = "__" + randomVariableName(4, 9);
		std::string arg3 = "__" + randomVariableName(4, 9);
		int n = sprintf(text, "\n\t\tint %s = %d;\n", arg1.c_str(), (rand() % 2 == 0) ? (rand() % RAND_MAX) : (-rand() % RAND_MAX));
		n += sprintf(text + n, "\t\tint %s = %d;\n", arg2.c_str(), (rand() % 2 == 0) ? (rand() % RAND_MAX) : (-rand() % RAND_MAX));
		n += sprintf(text + n, "\t\tint %s = %s * %s;\n", arg3.c_str(), arg1.c_str(), arg2.c_str());
		n += sprintf(text + n, "\t\t%s = (%s > %s) ? %s * %s : %s / %s;\n", arg2.c_str(), arg3.c_str(), arg1.c_str(), arg3.c_str(), arg1.c_str(), arg3.c_str(), arg1.c_str());
	}
	else if (type == 2) {
		std::string arg1 = "__" + randomVariableName(4, 9);
		std::string arg2 = "__" + randomVariableName(4, 9);
		std::string arg3 = "__" + randomVariableName(4, 9);
		int n = sprintf(text, "\n\t\tchar %s[] = \"%s\";\n", arg1.c_str(), m_words[rand() % m_words.size()].c_str());
		n += sprintf(text + n, "\t\tchar %s[] = \"%s\";\n", arg2.c_str(), m_words[rand() % m_words.size()].c_str());
		n += sprintf(text + n, "\t\tint %s = sizeof(%s) + sizeof(%s);\n", arg3.c_str(), arg1.c_str(), arg2.c_str());
		n += sprintf(text + n, "\t\t--%s;\n", arg3.c_str());
	}
	else if (type == 3) {
		std::string arg1 = "__" + randomVariableName(4, 9);
		std::string arg2 = "__" + randomVariableName(4, 9);
		int len = rand() % 10 + 8;
		const char* t = "\n\t\tchar %s[%d];\n"
			"\t\tfor(int i = 0; i < sizeof(%s); ++i){\n"
			"\t\t	%s[i] = i;\n"
			"\t\t}\n"
			"\t\tint * %s = (int*)(%s + %d);\n"
			"\t\t(*%s)++;\n";
		sprintf(text, t, arg1.c_str(), len, arg1.c_str(), arg1.c_str(), arg2.c_str(), arg1.c_str(), len - 8, arg2.c_str());
	}
	else if (type == 4) {
		std::string maparg = "__" + randomVariableName(4, 9);
		std::string arg1 = "__" + randomVariableName(4, 9);
		std::string arg2 = "__" + randomVariableName(4, 9);
		std::string arg3 = "__" + randomVariableName(4, 9);
		const char* t = "\n\t\tconst char* %s = \"%s\";\n"
			"\t\tconst char* %s = \"%s\";\n"
			"\t\tint %s = 0;\n"
			"\t\t%s += %s[0];\n"
			"\t\t%s -= %s[0];\n";
		sprintf(text, t,
			arg1.c_str(), m_words[rand() % m_words.size()].c_str(),
			arg2.c_str(), m_words[rand() % m_words.size()].c_str(),
			arg3.c_str(),
			arg3.c_str(), arg1.c_str(),
			arg3.c_str(), arg2.c_str());
	}
	else if (type == 5) {
		std::string arg1 = "__" + randomVariableName(4, 9);
		std::string arg2 = "__" + randomVariableName(4, 9);
		int len = rand() % 10 + 1;
		const char* t = "\n\t\tint %s[%d];\n"
			"\t\tint %s = 100;\n"
			"\t\tfor(int i = 0; i < %d; ++i){\n"
			"\t\t	%s[i] = i + 1;\n"
			"\t\t	if(%s[i] > %s){\n"
			"\t\t		%s = %s[i];\n"
			"\t\t	}\n"
			"\t\t}\n"
			"\t\t%s += %s[%d];\n";
		sprintf(text, t, arg1.c_str(), len, arg2.c_str(), len, arg1.c_str(), arg1.c_str(), arg2.c_str(), arg2.c_str(), arg1.c_str(), arg2.c_str(), arg1.c_str(), rand() % len);
	}
	else if (type == 6) {
		std::string arg1 = "__" + randomVariableName(4, 9);
		const char* t = "\n\t\tint %s = %d;\n"
			"\t\t%s *= %d;\n"
			"\t\t--%s;\n";
		sprintf(text, t, arg1.c_str(), rand() % RAND_MAX, arg1.c_str(), rand() % 1000, arg1.c_str());
	}
	else if (type == 7) {
		std::string arg1 = "__" + randomVariableName(4, 9);
		std::string arg2 = "__" + randomVariableName(4, 9);
		const char* t = "\n\t\tfloat %s = %.2ff;\n"
			"\t\tfloat %s = %.2ff;\n"
			"\t\t%s *= (%s > %s) ? %.2ff : %.2ff;\n";
		sprintf(text, t, arg1.c_str(), rand() / 100.0f, arg2.c_str(), rand() / 100.0f, arg1.c_str(), arg1.c_str(), arg2.c_str(), rand() / 100.0f, rand() / 100.0f);
	}
	else if (type == 8) {
		std::string arg1 = "__" + randomVariableName(4, 9);
		const char* t = "\n\t\tint %s = 0;\n"
			"\t\tfor(int i = 0; i < %d; ++i){\n"
			"\t\t	%s += i;\n"
			"\t\t}\n";
		sprintf(text, t, arg1.c_str(), rand() % 10 + 1, arg1.c_str());
	}
	else if (type == 9) {
		std::string arg1 = "__" + randomVariableName(4, 9);
		std::string arg2 = "__" + randomVariableName(4, 9);
		int l = (rand() % RAND_MAX) * (rand() % 10000);
		const char* t = "\n\t\tint %s = %d;\n"
			"\t\tint %s = 0;\n"
			"\t\twhile(%s){%s = %s >> 1; ++%s;}\n";
		sprintf(text,t,arg1.c_str(),l,arg2.c_str(),arg1.c_str(),arg1.c_str(),arg1.c_str(),arg2.c_str());
	}
	return text;
}

std::string AddGarbageCode::getPreContent(const std::string& data, int p) {
	std::string str;
	size_t pos = p - 1, pre = p;
	pos = data.rfind('\n', pre - 1);
	if (pos == -1) {
		str = data.substr(0, pre);
		trim(str);
	}

	while (pos != -1) {
		str = data.substr(pos + 1, pre - pos - 1);
		trim(str);
		pre = pos;
		if (!str.empty()) {
			break;
		}
		pos = data.rfind('\n', pre - 1);
	}
	return str;
}

bool AddGarbageCode::checkPreLine(const std::string& str) {
	if (str.empty()) {
		return true;
	}
	if (str[str.size() - 1] == '=') {
		return false;
	}
	if (str.find("extern \"C\"") != -1) {
		return false;
	}
	if (str.find("union") != -1) {
		return false;
	}
	if (str.find("//") != -1) {
		return false;
	}
	if (str.find("namespace") != -1) {
		return false;
	}
	if (str.find("struct") != -1) {
		return false;
	}
	if (str.find("switch") != -1) {
		return false;
	}
	if (str.find("for") != -1) {
		return false;
	}
	return true;
}

void AddGarbageCode::addGarbageCodeToCPP(std::string& data) {
	std::regex reg("\\{(\[^{}\]*)\\}");
	std::function<std::string(std::smatch const& match, size_t pos)> callback = [=](std::smatch const& match, size_t pos)->std::string {
		std::string content = match[1].str();
		if (content.find("\\") != -1) {
			return match[0].str();
		}

		std::string preline = getPreContent(data, pos);
		if (checkPreLine(preline)) {
			std::string s = createGarbageCode();
			s = "{\n\t" + s + match[1].str() + "}\n\t";
			Log("%s", s.c_str());
			return s;
		}

		return match[0].str();
	};
	data = regex_replace(data, reg, callback);
}
