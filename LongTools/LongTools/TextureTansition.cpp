#include "TextureTansition.h"
#include "PVRTexLib.hpp"
#include "utils.h"
#include "png.h"
#include "tga.h"

TextureTansition::TextureTansition(const std::string& input, const std::string& output, const std::string& totype, const std::string& filter)
:TraverseDir(input)
, _output(output)
, _totype(totype){
	splitFilter(filter, _filters, '@');
}

TextureTansition::~TextureTansition() {

}

bool TextureTansition::handleDirectory(const std::string& dir, const std::string& root, const std::string& relateDir) {
	std::string newdir = _output + "/" + relateDir;
	if (!isExistDirectory(newdir)) {
		CreateDirectory(newdir.c_str(), NULL);
	}
	return true;
}

void TextureTansition::handleDirectoryEnd(const std::string& dir, const std::string& root, const std::string& relateDir) {
	for (auto iter = _tomodify.begin(); iter != _tomodify.end(); ++iter) {
		std::string orgin = root + "/" + iter->first;
		std::string newfile = root + "/" + iter->second;
		MoveFile(orgin.c_str(), newfile.c_str());
		transitFile(root, iter->second);
	}
	_tomodify.clear();
}

bool TextureTansition::handleFile(const std::string& file, const std::string& root, const std::string& relate) {
	size_t pos = relate.find_last_of('.');
	std::string suffix = relate.substr(pos + 1);
	if (!_filters.empty() && _filters.find(suffix) == _filters.end()) {
		return true;
	}
	
	std::string newsuffix = checkSuffix(file);
	if (newsuffix != suffix) {
		_tomodify[relate] = relate.substr(0, pos) + "." + newsuffix;
		return true;
	}
	return transitFile(root, relate);
}

bool TextureTansition::transitFile(const std::string& root, const std::string& relate) {
	std::string file = root + "/" + relate;
	PVRTexture texture(file);
	if (!texture.GetTextureDataSize()) {
		Log("load the file failed, %s", file.c_str());
		return false;
	}

	if (!(texture.GetTexturePixelFormat() & PVRTEX_PFHIGHMASK) || texture.TextureHasPackedChannelData()) {
		const PVRTuint64 RGBA8888 = PVRTGENPIXELID4('r', 'g', 'b', 'a', 8, 8, 8, 8);
		if (!texture.Transcode(RGBA8888,
			PVRTexLibVariableType::PVRTLVT_UnsignedByteNorm,
			PVRTexLibColourSpace::PVRTLCS_Linear)) {
			Log("pvr texture transcode failed");
			return false;
		}
	}
	size_t pos = relate.find_last_of('.');
	std::string tmp = _output + "/" + relate.substr(0, pos);
	std::string suffix = relate.substr(0, pos);

	if (_totype == string("png")) {
		Log("%s to png", file.c_str());
		return saveToPng(&texture, tmp + ".png");
	}
	else if (_totype == string("ktx")) {
		Log("%s to ktx", file.c_str());
		return saveToKtx(&texture, tmp + ".ktx");
	}
	else if (_totype == string("astc")) {
		Log("%s to astc", file.c_str());
		return saveToAstc(&texture, tmp + ".astc");
	}
	else {
		Log("%s to default", file.c_str());
		std::string ktxfile = tmp + ".ktx";
		if (isExistFile(ktxfile)) {
			std::string newsuffix = checkSuffix(ktxfile);
			std::string newfile = ktxfile;
			if (newsuffix != "ktx") {
				newfile = tmp + "." + newsuffix;
				MoveFile(ktxfile.c_str(), newfile.c_str());
			}
			if (saveToKtx(&texture, newfile)) {
				if (newfile != ktxfile) {
					DeleteFile(ktxfile.c_str());
				}
				return true;
			}
			return false;
		}

		std::string astcfile = tmp + ".astc";
		if(isExistFile(astcfile)){
			std::string newsuffix = checkSuffix(astcfile);
			std::string newfile = astcfile;
			if (newsuffix != "astc") {
				newfile = tmp + "." + newsuffix;
				MoveFile(astcfile.c_str(), newfile.c_str());
			}
			if (saveToAstc(&texture, newfile)) {
				if (newfile != astcfile) {
					DeleteFile(astcfile.c_str());
				}
				return true;
			}
			return false;
		}

		std::string tgafile = tmp + ".tga";
		if (isExistFile(tgafile)) {
			return saveToTga(&texture, tgafile);
		}
		Log("can not find the corresponding file of %s, but will save to default file ktx", relate.c_str());
		return saveToKtx(&texture, ktxfile);
	}
}

bool TextureTansition::isKtxFile(const std::string& file) {
	static const char ktxId[] = { 0xAB,0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A };
	std::string id = getFileIdentifier(file, 12);
	return strncmp(id.c_str(), ktxId, 12) == 0;
}

bool TextureTansition::isAstcFile(const std::string& file) {
	static const char ktxId[] = { 0x13, 0xAB, 0xA1, 0x5C };
	std::string id = getFileIdentifier(file, 4);
	return strncmp(id.c_str(), ktxId, 4) == 0;
}

std::string TextureTansition::checkSuffix(const std::string& file) {
	size_t pos = file.find_last_of('.');
	std::string suffix = file.substr(pos + 1);
	std::string tmp = file.substr(0, pos);
	if (suffix == "ktx") {
		if (!isKtxFile(file) && isAstcFile(file)) {
			return "astc";
		}
	}
	else if (suffix == "astc") {
		if (!isAstcFile(file) && isKtxFile(file)) {
			return "ktx";
		}
	}
	return suffix;
}

bool TextureTansition::saveToPng(PVRTexture* texture, const std::string& file) {
	PVRTuint32 size = texture->GetTextureDataSize();
	unsigned char* data = (unsigned char*) texture->GetTextureDataPointer();
	unsigned int width = texture->GetTextureWidth(0);
	unsigned int height = texture->GetTextureHeight(0);
	unsigned int bit_depth = texture->GetTextureBitsPerPixel();

	FILE* fp = NULL;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	bool ret = true;

	fp = fopen(file.c_str(), "wb");
	if (!fp) {
		Log("open the file failed, %s", file.c_str());
		return false;
	}

	do {
		png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!png_ptr) {
			Log("png lib init failed");
			ret = false;
			break;
		}

		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr) {
			Log("png create png info failed");
			ret = false;
			break;
		}

		if (setjmp(png_jmpbuf(png_ptr))) {
			Log("Error during init_io");
			ret = false;
			break;
		}

		png_init_io(png_ptr, fp);

		unsigned int colorType = PNG_COLOR_TYPE_RGB_ALPHA;
		if (bit_depth == 24) {
			colorType = PNG_COLOR_TYPE_RGB;
		}

		png_set_IHDR(
			png_ptr,
			info_ptr,
			width,
			height,
			8,
			colorType,
			PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE,
			PNG_FILTER_TYPE_BASE);

		png_write_info(png_ptr, info_ptr);
		png_set_packing(png_ptr);

		if (setjmp(png_jmpbuf(png_ptr))) {
			Log("[write_png_file] Error during writing bytes");
			ret = false;
			break;
		}

		png_bytepp row_pointers = (png_bytepp) malloc(height * sizeof(png_bytep));
		if (!row_pointers) {
			ret = false;
			break;
		}
		int bytenum = bit_depth / 8;
		for (size_t i = 0; i < height; ++i) {
			row_pointers[i] = (png_bytep)data + (i * width * bytenum);
		}
		png_write_image(png_ptr, row_pointers);
		free(row_pointers);

		if (setjmp(png_jmpbuf(png_ptr))) {
			Log("[write_png_file] Error during end of write");
			ret = false;
			break;
		}
		png_write_end(png_ptr, info_ptr);
	} while (0);

	if (png_ptr) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
	}
	if (fp) {
		fclose(fp);
	}
	return ret;
}

bool TextureTansition::saveToKtx(PVRTexture* texture, const std::string& file) {
	PVRTint64 pixelFormat = PVRTexLibPixelFormat::PVRTLPF_ASTC_4x4;
	PVRTexLibVariableType channelType = PVRTexLibVariableType::PVRTLVT_UnsignedByteNorm;
	PVRTexLibColourSpace colorSpace = PVRTexLibColourSpace::PVRTLCS_Linear;
	if (isExistFile(file)) {
		PVRTexture ktxTexture(file);
		if (ktxTexture.GetTextureDataSize()) {
			pixelFormat = ktxTexture.GetTexturePixelFormat();
			channelType = ktxTexture.GetTextureChannelType();
			colorSpace = ktxTexture.GetColourSpace();
		}
	}
	if (!texture->Transcode(pixelFormat, channelType, colorSpace, PVRTexLibCompressorQuality::PVRTLCQ_ASTCFast)) {
		Log("texture transcode failed, %s", file.c_str());
		return false;
	}

	texture->SaveToFile(file);
	return true;
}

bool TextureTansition::saveToAstc(PVRTexture* texture, const std::string& file) {
	PVRTint64 pixelFormat = PVRTexLibPixelFormat::PVRTLPF_ASTC_4x4;
	PVRTexLibVariableType channelType = PVRTexLibVariableType::PVRTLVT_UnsignedByteNorm;
	PVRTexLibColourSpace colorSpace = PVRTexLibColourSpace::PVRTLCS_Linear;
	if (isExistFile(file)) {
		PVRTexture ktxTexture(file);
		if (ktxTexture.GetTextureDataSize()) {
			pixelFormat = ktxTexture.GetTexturePixelFormat();
			channelType = ktxTexture.GetTextureChannelType();
			colorSpace = ktxTexture.GetColourSpace();
		}
	}
	if (!texture->Transcode(pixelFormat, channelType, colorSpace, PVRTexLibCompressorQuality::PVRTLCQ_ASTCFast)) {
		Log("texture transcode failed, %s", file.c_str());
		return false;
	}

	texture->SaveToFile(file);
	return true;
}

bool TextureTansition::saveToTga(PVRTexture* texture, const std::string& file) {
	PVRTuint32 size = texture->GetTextureDataSize();
	unsigned char* data = (unsigned char*)texture->GetTextureDataPointer();
	unsigned int width = texture->GetTextureWidth(0);
	unsigned int height = texture->GetTextureHeight(0);
	unsigned int bit_depth = texture->GetTextureBitsPerPixel();

	TGA* out = NULL;
	TGAData* tgaData = NULL;
	tgaData = (TGAData*)malloc(sizeof(TGAData));
	out = TGAOpen((char*)file.c_str(), "wb");
	
	out->hdr.alpha = bit_depth > 24 ? 0 : 8;
	out->hdr.depth = bit_depth;
	out->hdr.height = height;
	out->hdr.width = width;
	out->hdr.horz = 0;
	out->hdr.id_len = 0;
	out->hdr.img_t = 2;
	out->hdr.map_entry = bit_depth;
	out->hdr.map_first = 0;
	out->hdr.map_len = 0;
	out->hdr.map_t = 0;
	out->hdr.x = 0;
	out->hdr.y = 0;
	out->hdr.vert = 1;

	tgaData->img_id = NULL;
	tgaData->cmap = NULL;
	tgaData->flags = TGA_IMAGE_ID | TGA_IMAGE_DATA | TGA_RGB;
	tgaData->img_data = data;

	TGAWriteImage(out, tgaData);

	TGAClose(out);
	free(tgaData);

	return true;
}