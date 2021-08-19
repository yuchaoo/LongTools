#include "utils.h"
#include <codecvt>
#include <Windows.h>

bool isGBK(const unsigned char* data, int len) {
    int i = 0;
    bool isAsc = true;
    while (i < len) {
        if (data[i] <= 0x7f) {
            i++;
            continue;
        }
        else {
            if (data[i] >= 0x81 &&
                data[i] <= 0xfe && i + 1 < len &&
                data[i + 1] >= 0x40 &&
                data[i + 1] <= 0xfe &&
                data[i + 1] != 0x7f) {
                i += 2;
                isAsc = false;
                continue;
            }
            else {
                return false;
            }
        }
    }
    return !isAsc;
}

int preNUm(unsigned char byte) {
    unsigned char mask = 0x80;
    if (byte & mask) {
        if ((byte & 0b11110000) == 0b11110000) return 4;
        if ((byte & 0b11100000) == 0b11100000) return 3;
        if ((byte & 0b11000000) == 0b11000000) return 2;
    }
    return 1;
}


bool isUtf8(const unsigned char* data, int len) {
    int num = 0;
    int i = 0;
    while (i < len) {
        int n = preNUm(data[i]);
        if (n == 1 && (data[i] & 0b10000000) != 0) {
            return false;
        }
        if (n == 2 && (i + 1 >= len
            || (data[i] & 0b11000000) != 0b11000000
            || (data[i + 1] & 0b10000000) != 0b10000000)) {
            return false;
        }
        if (n == 3 && (i + 2 >= len
            || (data[i] & 0b11100000) != 0b11100000
            || (data[i + 1] & 0b10000000) != 0b10000000
            || (data[i + 2] & 0b10000000) != 0b10000000)){
            return false;
        }
        if (n == 4 && (i + 3 >= len
            || (data[i] & 0b11110000) != 0b11110000
            || (data[i + 1] & 0b10000000) != 0b10000000
            || (data[i + 2] & 0b10000000) != 0b10000000
            || (data[i + 3] & 0b10000000) != 0b10000000)) {
            return false;
        }
        i += n;
    }
    return true;
}

void splitFilter(const std::string& s, std::set<std::string>& result, char c) {
    size_t pos = s.find(c);
    size_t pre = 0;
    while (pos != -1) {
        result.insert(s.substr(pre, pos - pre));
        pre = pos + 1;
        pos = s.find(c, pre);
    }
    if (pre < s.size()) {
        result.insert(s.substr(pre));
    }
}

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

    WCHAR* str1;
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

    WCHAR* str1;
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
    while (i >= 0) {
        if (!(str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r')) {
            break;
        }
        --i;
    }
    if (i < str.size() - 1) {
        str.resize(i + 1);
    }
}

std::string getFileIdentifier(const std::string& path, size_t len) {
    static char text[128];
    FILE* file = fopen(path.c_str(), "rb");
    if (!file) {
        return "";
    }
    size_t size = std::min<size_t>(len, sizeof(text));
    fread(text, 1, size, file);
    fclose(file);
    return string(text, size);
}