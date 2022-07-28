#include <iostream>
#include <clang-c/Index.h>

#include "Reflection.hpp"
#include <string>
#include <unordered_map>

std::string getCursorSpelling(CXCursor cursor)
{
	std::string str = (char*)clang_getCursorSpelling(cursor).data;
	return str;
}

std::string getCursorKindSpelling(CXCursor cursor)
{
	std::string str = (char*)clang_getCursorKindSpelling(clang_getCursorKind(cursor)).data;
	return str;
}

std::string getCursorTypeSpelling(CXCursor cursor)
{
	std::string str = (char*)clang_getTypeSpelling(clang_getCursorType(cursor)).data;
	return str;
}

auto visitor0 = [](CXCursor cursor, CXCursor parent, CXClientData clientData) {
	if (clang_isAttribute(clang_getCursorKind(cursor)))
	{
		*reinterpret_cast<std::string*>(clientData) = getCursorSpelling(cursor);
		return CXChildVisit_Break;
	}

	return CXChildVisit_Continue;
};

std::string getCursorAnnotateAttrSpelling(CXCursor cursor)
{
	std::string str;
	if (clang_Cursor_hasAttrs(cursor) && !clang_isAttribute(clang_getCursorKind(cursor)))
	{
		clang_visitChildren(cursor, visitor0, (void*)str.data());
	}
	return str;
}

using Property = std::pair<std::string, std::string>; //<变量名, 类型>
using ParseMap = std::unordered_map<std::string, std::vector<Property>>; //类或结构体及其成员
ParseMap g_ParseMap;

//找到成员变量
auto visitor1 = [](CXCursor cursor, CXCursor parent, CXClientData clientData) {
	CXCursorKind kind = clang_getCursorKind(cursor);
	if (kind != CXCursor_FieldDecl)
		return CXChildVisit_Continue;
	auto strParentAttr = getCursorAnnotateAttrSpelling(parent);
	auto strSelfAttr = getCursorAnnotateAttrSpelling(cursor);
	bool bShouldExtract = false;

	if ((strParentAttr == "WhiteList") && (strSelfAttr != "Disable"))
		bShouldExtract = true;
	if ((strParentAttr == "BlackList") && (strSelfAttr == "Enable"))
		bShouldExtract = true;

	if (bShouldExtract)
	{
		auto pVecProperty = &g_ParseMap[getCursorSpelling(parent)];
		pVecProperty->push_back(std::make_pair(getCursorSpelling(cursor), getCursorTypeSpelling(cursor)));
	}
	
	return CXChildVisit_Continue;
};

//找到class和struct
auto visitor2 = [](CXCursor cursor, CXCursor parent, CXClientData clientData) {
	if (clang_Cursor_hasAttrs(cursor))
	{
		//获取cursor的类型
		CXCursorKind kind = clang_getCursorKind(cursor);
		if ((kind == CXCursor_ClassDecl) || (kind == CXCursor_StructDecl))
		{
			auto strCursorName = getCursorSpelling(cursor);
			if (g_ParseMap.find(strCursorName) == g_ParseMap.end())
			{
				std::vector<Property> vecProperties;
				g_ParseMap[strCursorName] = vecProperties;
			}
			clang_visitChildren(cursor, visitor1, nullptr);
		}
	}
	return CXChildVisit_Continue;
};

int main()
{
	CXIndex index = clang_createIndex(0, 0);
	//解析文件 human.hpp
	CXTranslationUnit unit = clang_parseTranslationUnit(index, "human.hpp",
		nullptr, 0, nullptr, 0, CXTranslationUnit_None);

	if (!unit)
	{
		std::cout << "Parse Translation Unit Failed" << std::endl;
		return EXIT_FAILURE;
	}
	//获取AST的入口
	CXCursor entryPoint = clang_getTranslationUnitCursor(unit);
	clang_visitChildren(entryPoint, visitor2, nullptr);

	for (auto it = g_ParseMap.begin(); it != g_ParseMap.end(); ++it)
	{
		std::cout << it->first << ":" << std::endl;
		for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		{
			std::cout << "\t" << it2->first << "|" << it2->second << std::endl;
		}
	}

	clang_disposeTranslationUnit(unit);
	clang_disposeIndex(index);

	return EXIT_SUCCESS;
}

/************* output ***************
*  Human:
*       Age|int
*       Name|char *
*       Tel|char *
* Puppy:
*       Color|int
*       Dom|Human
**************************************/