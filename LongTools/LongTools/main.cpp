#include <windows.h>
#include <iostream>
#include <unordered_map>

#include "xxtea.h"
#include "TraverseHandler.h"
#include "cmdline.h"
#include "ExcelParser.h"
using namespace std;

int parseCommondline(int argc, char** args, std::unordered_map<string,string>& cmd) {
	int i = 1;
	while(i < argc){
		const char* str = args[i++];
		if (str[0] == '-') {
			string key(str + 1);
			if (i < argc && args[i][0] != '-') {
				string v = args[i++];
				cmd[key] = v;
			}
			else {
				cmd[key] = "";
			}
		}
		else {
			string key(str);
			cmd[key] = "";
		}
	}
	return 0;
}

template<typename... Args>
void print(Args... args){
	printf("args num is : %d", sizeof...args);
}

int main(int argc, char* args[]) {
	size_t startIndex = 0;

	cmdline::parser mainCmd;
	mainCmd.add<string>("type", '\0', "cmd type:encrypt,createpatch,subpkg,obb,zip,pathmap,repword,modifymd5,repclass,addcodes,xltolua", true, "");
	if (!mainCmd.parse(min(3,argc), args + startIndex)) {
		printf("%s\n", mainCmd.usage().c_str());
		return 0;
	}

	startIndex += 2;
	string type = mainCmd.get<string>("type");
	if (type == "encrypt") {
		cmdline::parser subCmd;
		subCmd.add<string>("input", 'i', "input file", true, "");
		subCmd.add<string>("filetype", 't', "encrypt file type, such as 'lua@json'", false, "");
		if (!subCmd.parse(argc - startIndex, args + startIndex)) {
			printf("%s\n", subCmd.usage().c_str());
			return 0;
		}

		std::string input = subCmd.get<string>("input");
		std::string filetype = subCmd.get<string>("filetype");
		
		EncryptHandler hander(input, filetype);
		hander.traverse();
	}
	else if (type == "createpatch") {
		cmdline::parser subCmd;
		subCmd.add<string>("input", 'i', "input file", true, "");
		subCmd.add<string>("version", 'v', "the patch version", true, "");
		subCmd.add<string>("lang", 'l', "the patch language", false, "");
		if (!subCmd.parse(argc - startIndex, args + startIndex)) {
			printf("%s\n", subCmd.usage().c_str());
			return 0;
		}

		std::string input = subCmd.get<string>("input");
		std::string version = subCmd.get<string>("version");
		std::string lang = subCmd.get<string>("lang");

		CreatePatchHandler handler(input, version, lang);
		handler.traverse();
	}
	else if (type == "subpkg") {
		cmdline::parser subCmd;
		subCmd.add<string>("input", 'i', "input dir", true, "");
		subCmd.add<string>("version", 'v', "the subpkg version", true, "");
		subCmd.add<string>("lang", 'l', "the subpkg version", true, "");
		subCmd.add<string>("config", 'c', "the config file", false, "");
		subCmd.add<string>("subver", 's', "the sub pkg version", true, "");
		subCmd.add<string>("flag", 'f', "all,patch,main", false, "all");
		if (!subCmd.parse(argc - startIndex, args + startIndex)) {
			printf("%s\n", subCmd.usage().c_str());
			return 0;
		}

		std::string input = subCmd.get<string>("input");
		std::string version = subCmd.get<string>("version");
		std::string lang = subCmd.get<string>("lang");
		std::string config = subCmd.get<string>("config");
		std::string subver = subCmd.get<string>("subver");
		std::string flag = subCmd.get<string>("flag");

		CreateGamePkgHandler handler(input, version, config, lang, subver, flag);
		handler.traverse();
	}
	else if (type == "obb") {
		cmdline::parser subCmd;
		subCmd.add<string>("input", 'i', "the input dir", true, "");
		subCmd.add<string>("outputfile",'o',"the outputfile name is not dir",true,"");
		if (!subCmd.parse(argc - startIndex, args + startIndex)) {
			printf("%s\n", subCmd.usage().c_str());
			return 0;
		}

		std::string input = subCmd.get<string>("input");
		std::string outfile = subCmd.get<string>("outputfile");

		ObbFileCreator creator(input, outfile);
		creator.traverse();
	}
	else if (type == "zip") {
		cmdline::parser subCmd;
		subCmd.add<string>("input", 'i', "the input dir", true, "");
		subCmd.add<string>("filter", 'f', "the file type for zip, such as 'lua|png'",true,"");
		if (!subCmd.parse(argc - startIndex, args + startIndex)) {
			printf("%s\n", subCmd.usage().c_str());
			return 0;
		}

		std::string input = subCmd.get<string>("input");
		std::string filter = subCmd.get<string>("filter");

		CompressFiles compressFiles(input, filter);
		compressFiles.traverse();
	}
	else if (type == "pathmap") {
		cmdline::parser subCmd;
		subCmd.add<string>("input", 'i', "the input dir", true, "");
		subCmd.add<string>("output", 'o', "output dir", true, "");
		subCmd.add<string>("charmap", 'c', "the replace char map",true,"");
		subCmd.add<bool>("redundant",'r',"add the redundant files",false,true);
		subCmd.add<string>("filterfile", 'f', "the filter config file", false, "");
		if (!subCmd.parse(argc - startIndex, args + startIndex)) {
			printf("%s\n", subCmd.usage().c_str());
			return 0;
		}

		std::string input = subCmd.get<string>("input");
		std::string output = subCmd.get<string>("output");
		std::string charmap = subCmd.get<string>("charmap");
		bool redundant = subCmd.get<bool>("redundant");
		std::string filterFile = subCmd.get<string>("filterfile");

		ResPathHandler handler(input, output, charmap, filterFile, redundant);
		handler.traverse();
	}
	else if (type == "repword") {
		cmdline::parser subCmd;
		subCmd.add<string>("input",'i',"the input dir",true,"");
		subCmd.add<string>("output",'o',"the output dir",true,"");
		subCmd.add<string>("mapcfg",'m',"the key words config",true,"");
		subCmd.add<bool>("ressuffix", 'r', "keep suffix", false, false);
		subCmd.add<bool>("addredundant", 'a', "add redundant file flag", false, true);
		if (!subCmd.parse(argc - startIndex, args + startIndex)) {
			printf("%s\n", subCmd.usage().c_str());
			return 0;
		}

		std::string input = subCmd.get<string>("input");
		std::string output = subCmd.get<string>("output");
		std::string mapcfg = subCmd.get<string>("mapcfg");
		bool bReserveSuffix = subCmd.get<bool>("ressuffix");
		bool addRed = subCmd.get<bool>("addredundant");
		KeyWorldHandler handler(input,output,mapcfg, bReserveSuffix,addRed);
		handler.traverse();
	}
	else if (type == "modifymd5") {
		cmdline::parser subCmd;
		subCmd.add<string>("input", 'i', "the input dir", true, "");
		subCmd.add<string>("filters",'f',"filter files, such as png@jpeg","");
		if (!subCmd.parse(argc - startIndex, args + startIndex)) {
			printf("%s\n", subCmd.usage().c_str());
			return 0;
		}
		std::string input = subCmd.get<string>("input");
		std::string filters = subCmd.get<string>("filters");

		ModifyMd5Handler handler(input, filters);
		handler.traverse();	
	}
	else if (type == "repclass") {
		cmdline::parser subCmd;
		subCmd.add<string>("input", 'i', "the input dir", true, "");
		subCmd.add<string>("wordcfg",'w', "the word config file",true,"");
		subCmd.add<string>("classcfg",'c', "the class name config",true,"");
		if (!subCmd.parse(argc - startIndex, args + startIndex)) {
			printf("%s\n", subCmd.usage().c_str());
			return 0;
		}

		std::string input = subCmd.get<string>("input");
		std::string wordcfg = subCmd.get<string>("wordcfg");
		std::string classcfg = subCmd.get<string>("classcfg");

		RepClassNameHandler handler(input, classcfg, wordcfg);
		handler.traverse();
	}
	else if (type == "addcodes") {
		cmdline::parser subCmd;
		subCmd.add<string>("cfgfile",'c',"the config file",true,"");
		subCmd.add<string>("wordcfg", 'w', "the words config", true, "");
		if (!subCmd.parse(argc - startIndex, args + startIndex)) {
			printf("%s\n", subCmd.usage().c_str());
			return 0;
		}
		std::string cfgfile = subCmd.get<string>("cfgfile");
		std::string wordcfg = subCmd.get<string>("wordcfg");

		AddGarbageCode addGarHandler(cfgfile, wordcfg);
		addGarHandler.traverse();
	}
	else if (type == "xltolua") {
		cmdline::parser subCmd;
		subCmd.add<string>("input", 'i', "the excel dir", true, "");
		subCmd.add<string>("clientoutput", 'c', "the client output dir", true, "");
		subCmd.add<string>("serveroutput",'s',"the server output dir",true, "");
		subCmd.add<string>("exportflag", 'f', "the export flag 'client' 'server' 'all'",true,"");

		if (!subCmd.parse(argc - startIndex, args + startIndex)) {
			printf("%s\n", subCmd.usage().c_str());
			return 0;
		}
		std::string input = subCmd.get<string>("input");
		std::string clientoutput = subCmd.get<string>("clientoutput");
		std::string serveroutput = subCmd.get<string>("serveroutput");
		std::string exportflag = subCmd.get<string>("exportflag");
		ExcelParser parser(input, clientoutput, serveroutput, exportflag);
		parser.traverse();
	}
	else {
		printf("%s\n", mainCmd.usage().c_str());
	}
	system("pause");
	return 0;
}