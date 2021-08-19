#pragma once
#include <string>
#include <set>
using namespace std;
int preNUm(unsigned char byte);
bool isGBK(const unsigned char* data, int len);
bool isUtf8(const unsigned char* data, int len);

void splitFilter(const std::string& s, std::set<std::string>& result, char c);
std::string wstr2str(const std::wstring& str);
std::wstring str2wstr(const std::string& str);
string GbkToUtf8(const std::string& strGBK);
string Utf8ToGbk(const std::string& strUTF8);
void trim(string& s);
std::string getFileIdentifier(const std::string& path, size_t len);
