#pragma once
#include "TraverseHandler.h"
#include "PVRTexLib.hpp"
using namespace pvrtexlib;

class TextureTansition : public TraverseDir {
public:
	TextureTansition(const std::string& input, const std::string& output, const std::string& totype, const std::string& filter);
	~TextureTansition();
	virtual bool handleDirectory(const std::string& dir, const std::string& root, const std::string& relateDir);
	virtual void handleDirectoryEnd(const std::string& dir, const std::string& root, const std::string& relateDir);
	virtual bool handleFile(const std::string& file, const std::string& root, const std::string& relateDir);
private:
	bool saveToPng(PVRTexture* texture, const std::string& file);
	bool saveToKtx(PVRTexture* texture, const std::string& file);
	bool saveToAstc(PVRTexture* texture, const std::string& file);
	bool saveToTga(PVRTexture* texture, const std::string& file);
	bool isKtxFile(const std::string& file);
	bool isAstcFile(const std::string& file);
	std::string checkSuffix(const std::string& file);
	bool transitFile(const std::string& root, const std::string& relate);
private:
	std::string _output;
	std::string _totype;
	std::set<string> _filters;
	std::unordered_map<std::string, std::string> _tomodify;
};