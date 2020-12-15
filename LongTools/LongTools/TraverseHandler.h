#pragma once

#include <vector>
#include <functional>
#include <unordered_map>
#include <map>
#include <set>
#include <regex>
#include "json/rapidjson.h"
#include "json/document.h"
#include "zip.h"

using namespace std;

typedef std::function<void(const std::string&)> TraverseCallback;
struct PatchInfo {
	std::string name;
	std::string md5;
	int size;
};

class TraverseDir {
public:
	TraverseDir(const std::string& dir);
	virtual ~TraverseDir();
	virtual bool init() { return true; }
	virtual bool traverse();
	void setInputDir(const std::string& dir) { m_dir = dir; }
	std::string getCurrentDirectory();
	bool isExistDirectory(const std::string& path);
	bool isExistFile(const std::string& path);
	void removeDirectory(const std::string& dir);
	PatchInfo* createPatchInfo(const std::string& path, const std::string& name);
	virtual bool handleDirectory(const std::string& dir, const std::string& root, const std::string& relateDir);
	virtual void handleDirectoryEnd(const std::string& dir, const std::string& root, const std::string& relateDir) {};
	virtual bool handleFile(const std::string& file, const std::string& root, const std::string& relateDir);
	virtual void traverseEnd() {};
	void checkDirWithFileName(const std::string& file, const std::string& root, const std::string& releate);
	std::shared_ptr<char> getFileData(const char* filepath, size_t& size);
	void writeFileData(const std::string& filepath, const unsigned char* data, size_t size);
	std::string getSuffix(const std::string& name);
	std::string getMd5(const std::string& file);
protected:
	virtual bool doTraverse(const std::string& root, const std::string& relateDir);
protected:
	std::string m_dir;
};

class EncryptHandler : public TraverseDir {
public:
	EncryptHandler(const std::string& dir, const std::string& type);
	~EncryptHandler();
	virtual bool traverse();
	virtual bool handleFile(const std::string& file, const std::string& root, const std::string& releate);
	void skipBOM(const char*& chunk, int& chunkSize);
	void encryptFile(const char* path);
	void decryptFile(const char* path);
	std::string getKeyByRandom(int len);
	bool isLuaFile(const char* filename);
	bool isNeedEncrypt(const char* filename);
private:
	std::vector<std::string> m_types;
};

class CreatePatchHandler : public TraverseDir {
public:
	CreatePatchHandler(const std::string& dir,const std::string& version,const std::string& lang);
	~CreatePatchHandler();
	virtual bool handleDirectory(const std::string& dir, const std::string& root, const std::string& releate);
	virtual bool handleFile(const std::string& file, const std::string& root, const std::string& releate);
	bool createPatchFile();
	bool createLangConfigFile();
	virtual bool traverse();
private:
	std::string getFileLang(const std::string& name);
private:
	std::string m_versionDir;
	std::string m_version;
	std::string m_curLang;
	std::vector<PatchInfo*> m_allPatchs;
	std::vector<PatchInfo*> m_patchs;
	std::unordered_map<std::string, std::vector<PatchInfo*>> m_langMap;
};

struct HeroResource {
	std::set<int> heros;
	std::vector<std::string> files;
	std::vector<std::regex> regex_list;
};

struct StageResource {
	int chapterId;
	int stageId;
	std::vector<std::string> files;
	std::vector<std::regex> regex_list;
};

struct SystemResource {
	std::vector<std::string> files;
	std::set<std::string> pkdIds;
	std::vector<std::regex> regex_list;
};

class CreateGamePkgHandler : public TraverseDir{
public:
	CreateGamePkgHandler(const std::string& dir, std::string& version, const std::string& config, const std::string& lang, const std::string& subVersion, const std::string& flag = "all");
	~CreateGamePkgHandler();
	virtual bool init();
	virtual bool handleDirectory(const std::string& dir, const std::string& root, const std::string& releate);
	virtual bool handleFile(const std::string& file, const std::string& root, const std::string& releate);
	virtual void traverseEnd();
	bool matchHeroResource(const std::string& file);
	bool matchStageResource(const std::string& file);
	bool matchSystemResource(const std::string& file);
	bool matchOtherResource(const std::string& file);
	bool isFileUsed(const std::string& file);
	void addPatchItemConfig(rapidjson::Document& doc, rapidjson::Value& value, std::vector<PatchInfo*>& patchs);
	bool outputDocument(rapidjson::Document& doc,const std::string& file);
	
private:
	std::string m_config;
	std::string m_version;
	std::string m_subVersion;
	std::string m_flag;
	std::string m_versionPatchDir;
	std::string m_versionSubDir;
	std::string m_lang;
	std::string m_versionMainDir;
	std::set<std::string> m_usedFiles;
	std::set<std::string> m_usedDirs;
	std::set<std::string> m_unusedDirs;
	std::set<std::string> m_unusedFiles;
	HeroResource m_heroRes;
	StageResource m_stageRes;
	SystemResource m_sysRes;
	std::vector<PatchInfo*> m_allPatchs; //所有包
	std::unordered_map<std::string, std::vector<PatchInfo*>> m_langPatchs;
	std::vector<PatchInfo*> m_mainPkgPatchs; //主包
	std::vector<PatchInfo*> m_subPkgPatchs;  //子包
	bool m_isAllInMain;
};

class CheckDBFileNameHandler : public TraverseDir {
public:
	CheckDBFileNameHandler(const std::string& dir);
	~CheckDBFileNameHandler();
	virtual bool doTraverse(const std::string& root, const std::string& relateDir);
	virtual bool handleDirectory(const std::string& dir, const std::string& root, const std::string& releate);
	virtual bool handleFile(const std::string& file, const std::string& root, const std::string& releate);
	virtual void traverseEnd();
private:
	std::vector<std::string> m_files;
	std::string m_lastDir;
};

class ObbFileCreator : public TraverseDir {
public:
	ObbFileCreator(const std::string& dir, const std::string& fileName);
	~ObbFileCreator();
	virtual bool traverse();
	virtual bool handleFile(const std::string& file, const std::string& root, const std::string& releate);
	virtual void traverseEnd();
private:
	std::string m_fileName;
	zipFile m_zipFile;
};

class CompressPng : TraverseDir {
public:
	CompressPng(const std::string& dir,const std::string& config);
	~CompressPng();
	virtual bool traverse();
	virtual bool handleFile(const std::string& file, const std::string& root, const std::string& releate);
private:
	std::string m_config;
	std::unordered_map<std::string, int> m_qualityMap;
	int m_quality;
	int m_speed;
};

class CompressFiles : public TraverseDir {
public:
	CompressFiles(const std::string& dir, const std::string& filter);
	~CompressFiles();
	bool isInFilter(const std::string& releate);
	virtual bool handleFile(const std::string& file, const std::string& root, const std::string& releate);
private:
	std::vector<std::string> m_filters;
};

class CompareVersion : public TraverseDir {
public:
	CompareVersion(const std::string& input1, const std::string& input2, const std::string& field);
	~CompareVersion();
	void compare(const char* data, size_t size,std::unordered_map<std::string, PatchInfo* >& patch);
	virtual bool traverse();
private:
	std::string m_input1;
	std::string m_input2;
	std::string m_field;
};

class ResPathHandler : public TraverseDir {
public:
	ResPathHandler(const std::string& input, const std::string& output, const std::string& charMap, const std::string& filterFile, bool isCreateRedundant);
	~ResPathHandler();
	virtual bool traverse();
	virtual bool handleDirectory(const std::string& dir, const std::string& root, const std::string& releate);
	virtual void handleDirectoryEnd(const std::string& dir, const std::string& root, const std::string& releate);
	virtual bool handleFile(const std::string& file, const std::string& root, const std::string& releate);
	virtual void traverseEnd();
private:
	std::string getReplaceReleatePath(const std::string& path,const std::string& filterPath);
	void createRedundantDir(std::string path);
	void createRedundantFile(std::string path);
	std::string randomString();
	std::string getFilterDir(const std::string& path);
	std::string getFilterFile(const std::string& path);
private:
	std::string m_output;
	std::string m_charMap;
	std::unordered_map<std::string, std::string> m_requirePathMap;
	std::unordered_map<std::string, std::string> m_resPathMap;
	std::string m_filterConfig;
	std::vector<std::string> m_filterDir;
	std::vector<std::string> m_filterFile;
	FILE* m_logFile;
	bool m_isCreateRedundant;
};

struct WordSlice {
	bool isPlaced = false;
	std::string slice;
	int index = 0;
};

class KeyWorldHandler : public TraverseDir {
public:
	KeyWorldHandler(const std::string& input, const std::string& output, const std::string& mapFile, bool bReserveSuffix, bool addRedundant);
	~KeyWorldHandler();
	virtual bool traverse();
	virtual bool handleDirectory(const std::string& dir, const std::string& root, const std::string& releate);
	virtual bool handleFile(const std::string& file, const std::string& root, const std::string& releate);
	virtual void traverseEnd();
	std::string createFileWithRandom(const std::string& root, const std::string& releate);
	void addRedundantFiles(const std::string& dir);
	std::string createFileName();
private:
	std::string m_output;
	std::string m_mapFile;
	std::string m_outputFile;
	std::vector<std::string> m_dirWords;
	std::vector<std::string> m_fileWords;
	std::map<std::string, std::string> m_fileMap;
	std::unordered_map < std::string, std::string> m_dirMap;
	bool m_isAddRedundant;
	bool m_bReserveSuffix;
};

class ModifyMd5Handler : public TraverseDir {
public:
	ModifyMd5Handler(const std::string& input, const std::string& filter);
	bool isNeedModifyBySuffix(const std::string& suffix);
	virtual bool handleFile(const std::string& file, const std::string& root, const std::string& releate);
	void modifyPngMd5(const std::string& file, const std::string& releate);
	void modifyLuaMd5(const std::string& file, const std::string& releate);
	void modifyJpegMd5(const std::string& file, const std::string& releate);
private:
	std::vector<std::string> m_filters;
};

struct RegexString {
	std::string regstr;
	int totalGroup;
	std::vector<int> repindexs;
};

enum RepType {
	kClassName,
	kFuncName,
	kAddCodes,
	kLuaCFunc,	
};

struct LuaCFuncRegexInfo {
	std::string file;
	std::string regex;
};

class RepClassNameHandler : public TraverseDir {
public:
	RepClassNameHandler(const std::string& input, const std::string& cfgfile, const std::string& wordsfile);
	~RepClassNameHandler();
	virtual bool traverse();
	virtual bool handleFile(const std::string& file, const std::string& root, const std::string& releate);
	std::string createClassName();
	std::string createFuncName();
	std::string createLuaCFuncName();
	void replaceClassName(std::string& data, const std::string& classname, const std::string& repstr);
	void replaceFuncName(std::string& data, const std::string& funcname, const std::string& repstr);
	void replaceLuaCFunName(std::string& data, const std::string& regex);
	virtual void traverseEnd();
private:
	std::string m_cfgfile;
	std::string m_wordsfile;
	std::string m_curClassName;
	std::string m_curRepstr;
	std::string m_curFuncName;
	int m_repType = kClassName;
	std::vector<std::string> m_worlds;
	std::vector<std::string> m_classNames;
	std::vector<std::string> m_funcNames;
	std::set<std::string> m_sets;
	std::unordered_map<std::string,std::string> m_result;
	std::vector<LuaCFuncRegexInfo> m_luacfunc;
	std::set<std::string> m_luacFilters;
};

class AddGarbageCode : public TraverseDir {
public:
	AddGarbageCode(const std::string& cfgfile, const std::string& wordsfole);
	~AddGarbageCode();
	virtual bool traverse();
	virtual bool handleFile(const std::string& file, const std::string& root, const std::string& releate);
	bool handleCppFile(const std::string& file);
	void addGarbageCodeToCPP(std::string& data);
	std::string createGarbageCode();
	std::string getPreContent(const std::string& data, int p);
	bool checkPreLine(const std::string& str);
private:
	std::string m_cfgFile;
	std::string m_wordFile;
	std::vector<std::string> m_words;
	std::vector<std::string> m_inputDirs;
	std::vector<std::string> m_inputFiles;
};