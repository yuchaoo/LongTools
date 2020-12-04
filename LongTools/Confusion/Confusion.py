# -*- coding=utf-8 -*-
import os;
import re;
import chardet;

def createNewFileName(fileName,reword):
    tmp = os.path.splitext(fileName);    
    name = tmp[0]
    suffix = tmp[1]

    def matched(result):
        group = result.group(1)
        return reword + group
    result = re.sub("([A-Z][a-z0-9]*)",matched, name)
    print(result)
    return result + suffix

def replaceFileInclude(filepath,header,replace):
    print("替换头文件:%s" % header)
    print("查找文件:%s" % filepath)

    header = os.path.splitext(header)[0]
    replace = os.path.splitext(replace)[0]

    file = open(filepath,"rb")
    data = file.read()
    file.close()

    result = chardet.detect(data)
    data = data.decode(result["encoding"])

    def matched(result):
       group = result.group(2) 
       return replace + group

    format = r"(%s)(\.h\"\s*)" % header
    data = re.sub(format,matched,data)

    data = data.encode('utf-8')
    file = open(filepath,"wb")
    file.write(data)
    file.close()

def replaceFileClassName(filepath,className,replace):
    file = open(filepath,"rb")
    data = file.read()
    file.close()

    if len(data) <= 0:
        return

    print("类名:%s->%s" % (className,replace))
    print("查找文件:%s" % filepath)

    result = chardet.detect(data)
    data = data.decode(result["encoding"])

    def matched(result):
        return result.group(1) + replace + result.group(3)
    

    for i in range(0,2):
        format = "([^\w\"/])(%s)([^\w\.])" % className
        data = re.sub(format,matched,data);

    data = data.encode('utf-8')
    file = open(filepath,"wb")
    file.write(data)
    file.close()


def replaceDirInclude(dirpath,header,replace):
    for root, dirs, files in os.walk(dirpath):
        for file in files:
            filepath = os.path.join(root,file)
            replaceFileInclude(filepath,header,replace)
        for dir in dirs:
            tmp = os.path.join(root,dir)
            replaceDirInclude(tmp,header,replace)

def replaceAllDirInclude(dirpath,headers,headerMap):
    for header in headers:
        replaceDirInclude(dirpath,header, headerMap[header])

def replaceDirClassName(dirpath,className,replace):
     for root, dirs, files in os.walk(dirpath):
        for file in files:
            filepath = os.path.join(root,file)
            replaceFileClassName(filepath,className,replace)
        for dir in dirs:
            tmp = os.path.join(root,dir)
            replaceDirClassName(tmp,className,replace)

def replaceAllClassName(dirpatch,classMap):
    keys = classMap.keys()
    for key in keys:
        replaceDirClassName(dirpatch,key,classMap[key])

def replaceFileName(fileDir, nameMap, headers):
    for root , dirs, files in os.walk(fileDir):
        for file in files:
            oldfilepath = os.path.join(root,file)
            if file in nameMap.keys():
                newfilepath = os.path.join(root,nameMap[file])
                os.rename(oldfilepath,newfilepath)
                suffix = os.path.splitext(file)[1]
                if suffix == ".h":
                    headers.append(file)
        for dir in dirs:
            dirpath = os.path.join(root,dir)
            replaceFileName(dirpath,nameMap,headers)

def main():
    fileMap = {
        "AndroidSdkBridge.cpp":"AndroidPlatformSdkBridge.cpp",
        "AndroidSdkBridge.h" : "AndroidPlatformSdkBridge.h",
        "AppUtils.cpp":"ApplicationUtils.cpp",
        "AppUtils.h":"ApplicationUtils.h",
        "AssetsUpdater.cpp":"ResuouceUpdater.cpp",
        "AssetsUpdater.h":"ResuouceUpdater.h",
        "BuglyLuaAgent.cpp":"BgyLuaAgenter.cpp",
        "BuglyLuaAgent.h":"BgyLuaAgenter.h",
        "Configure.cpp":"PatchConfigure.cpp",
        "Configure.h":"PatchConfigure.h",
        "CrashReport.cpp":"CrashReporter.cpp",
        "CrashReport.h":"CrashReporter.h",
        "DownloadMultiTask.cpp":"DldMultiTask.cpp",
        "DownloadMultiTask.h":"DldMultiTask.h",
        "DownLoadTask.cpp":"DldTask.cpp",
        "DownLoadTask.h":"DldTask.h",
        "DownloadThread.cpp":"DldThread.cpp",
        "DownloadThread.h":"DldThread.h",
        "IOSSdk.cpp":"IOSPlatformSdk.cpp",
        "IOSSdk.h":"IOSPlatformSdk.h",
        "IOSSdk.mm":"IOSPlatformSdk.mm",
        "lua_app_register.h":"lua_all_app_register.h",
        "PatchEvent.cpp":"DownloadTaskEvent.cpp",
        "PatchEvent.h":"DownloadTaskEvent.h",
        "register_game_module.cpp":"register_all_game_module.cpp",
        "SdkBridge.cpp":"SKKBridge.cpp",
        "SdkBridge.h":"SKKBridge.h",
        "UpdateBase.cpp":"UpdateTaskBase.cpp",
        "UpdateBase.h":"UpdateTaskBase.h",
        "UpdateLang.cpp":"UpdateLangTask.cpp",
        "UpdateLang.h":"UpdateLangTask.h",
        "UpdatePatch.cpp":"UpdatePatchTask.cpp",
        "UpdatePatch.h":"UpdatePatchTask.h",
        "BaseSocket.cpp":"BaseSkt.cpp",
        "BaseSocket.h":"BaseSkt.h",
        "LuaSocket.cpp":"LuaSkt.cpp",
        "LuaSocket.h":"LuaSkt.h"
    }

    '''
    fileMap = {
        "AssetsUpdater.h":"ResuouceUpdater.h"
    }'''

    classMap = {
        "BaseSocket":"BaseSkt",
        "GamePackage":"GamePkg",
        "LuaSocket":"LuaSkt",
        "AppUtils":"GameUtils",
        "BuglyLuaAgent":"BglLuaAgenter",
        "CrashReport":"CrsReporter",
        "DownloadMultiTask":"DldMultiTask",
        "DownloadTask":"DldTask",
        "DownloadThread":"DldThread",
        "PatchEvent":"GamePthEvent",
        "FileOperateEvent":"FileOptEvent",
        "DownloadEvent":"DldEvent",
        "ProgressEvent":"PgsEvent",
        "ThreadQuitEvent":"ThdQuitEvent",
        "EventHandler":"EvtHandler",
        "Configure":"PatchConfigure",
        "VersionConfig":"VrsConfig",
        "AssetsUpdater":"ResourceUpdater",
        "SdkBridge":"SKKBridge",
        "UpdateLang":"UpdateLangTask",
        "PatchConfig":"PthConfig",
        "UpdatePatch":"UpdatePatchTask",
        "UpdateBase":"UpdateTaskBase"
    }

    headers = []
    dirpath = "E:\\project\\k1_engine\\trunk\\frameworks\\runtime-src\\Classes"
    replaceFileName(dirpath,fileMap,headers)
    replaceAllDirInclude(dirpath,headers,fileMap)
    replaceAllClassName(dirpath,classMap)
if __name__ == "__main__":
    main()





