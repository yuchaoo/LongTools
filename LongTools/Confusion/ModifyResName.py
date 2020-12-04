# -*- coding=utf-8 -*-

import os;
import random;

MaskCode = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890"
MaskKey = "OPQRSTcdeuvwxyUVWXYZabz1234567890ABCDfghijklmnopqrstEFGHIJKLMN"

Filter = {".png",".jpg",".json",".lua"}
Result = set({})

def getCharIndex(c):
    if c >= 'A' and c <= 'Z':
        return ord(c) - ord('A')
    if c >= 'a' and c <= 'z':
        return ord(c) - ord('a') + 26
    if c >= '0' and c <= '9':
        return ord(c) - ord('0') + 52
    return -1

def getNewFileName(fileName):
    newName = ""
    for i in range(0,len(fileName)):
        index = getCharIndex(fileName[i])
        if index >= 0:
            newName += MaskKey[index]
        else:
            newName += fileName[i]
    return newName


def modifyFileName(pathDir):
    print(pathDir)
    files = os.listdir(pathDir)
    for file in files:
        filepath = os.path.join(pathDir,file)
        if os.path.isdir(filepath):
            modifyFileName(filepath)
        else:
            filename = os.path.splitext(file)[0]
            suffix = os.path.splitext(file)[1]
            if suffix in Filter:
                if filename in Result:
                    print(filename)
                tmp = getNewFileName(file)
                newfile = os.path.join(pathDir,tmp)
                print("%s->%s" % (file,tmp))
                Result.add(tmp)
                try:
                    os.rename(filepath,newfile)
                except Exception as e:
                    print("filename:%s rename failed" % filename)

if __name__ == "__main__":
    modifyFileName("./version-1.5.19992-zh-TW")
