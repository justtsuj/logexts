#include<tchar.h>
#include<Windows.h>
#include<fstream>
#include<vector>
#include<map>
#include<iostream>
#include <nlohmann/json.hpp>

#include "Manifest/Manifest.h"

namespace ApiManifest {

	void to_json(nlohmann::json& j, const Flag& flag) {
	    j["name"] = flag.name;
	}
	void from_json(const nlohmann::json& j, Flag& flag) {
		j.at("name").get_to(flag.name);
	}
	void to_json(nlohmann::json& j, const Value& value) {
		j["name"] = value.name;
	}
	void from_json(const nlohmann::json& j, Value& value) {
		j.at("name").get_to(value.name);
	}
	void to_json(nlohmann::json& j, const UUID& uuid) {
		j["name"] = uuid.name;
		j["uuid"] = uuid.uuid;
	}
	void from_json(const nlohmann::json& j, UUID& uuid) {
		j.at("name").get_to(uuid.name);
		j.at("uuid").get_to(uuid.uuid);
	}
	void to_json(nlohmann::json& j, const Struct& structt) {
		j["declarations"] = nlohmann::json::array();
		for (Declaration* declaration : structt.declarations) {
			j["declarations"].emplace_back(*declaration);
		}
	}
	void from_json(const nlohmann::json& j, Struct& structt) {
		for (const nlohmann::json& element : j.at("declarations")) {
			Declaration* declaration = new Declaration();
			element.get_to(*declaration);
			structt.declarations.push_back(declaration);
		}
	}
	void to_json(nlohmann::json& j, const Type& type) {
		j["typeIndex"] = type.typeIndex;
		j["parentIndex"] = type.parentTypeIndex;
		j["name"] = type.name;
		j["flags"] = nlohmann::json::array();
		for (Flag* flag : type.flags) {
			j["flags"].emplace_back(*flag);
		}
		j["values"] = nlohmann::json::array();
		for (Value* value : type.values) {
			j["values"].emplace_back(*value);
		}
		j["structIndex"] = type.structIndex;
		j["IIDIndex"] = type.IIDIndex;
		j["methodsIndex"] = type.methodsIndex;
	}
	void from_json(const nlohmann::json& j, Type& type) {
		j.at("typeIndex").get_to(type.typeIndex);
		j.at("parentIndex").get_to(type.parentTypeIndex);
		j.at("name").get_to(type.name);
		for (const nlohmann::json& element : j.at("flags")) {
			Flag* flag = new Flag();
			element.get_to(*flag);
			type.flags.push_back(flag);
		}
		for (const nlohmann::json& element : j.at("values")) {
			Value* value = new Value();
			element.get_to(*value);
			type.values.push_back(value);
		}
		j.at("structIndex").get_to(type.structIndex);
		j.at("IIDIndex").get_to(type.IIDIndex);
		j.at("methodsIndex").get_to(type.methodsIndex);
	}
	void to_json(nlohmann::json& j, const Category& category) {
		j["name"] = category.name;
	}
	void from_json(const nlohmann::json& j, Category& category) {
		j.at("name").get_to(category.name);
	}
	void to_json(nlohmann::json& j, const Declaration& declaration) {
		j["isOutParam"] = declaration.isOutParam;
		j["name"] = declaration.name;
		j["pointRank"] = declaration.pointerRank;
		j["number"] = declaration.number;
		j["typeIndex"] = declaration.typeIndex;
	}
	void from_json(const nlohmann::json& j, Declaration& declaration) {
		j.at("isOutParam").get_to(declaration.isOutParam);
		j.at("name").get_to(declaration.name);
		j.at("pointRank").get_to(declaration.pointerRank);
		j.at("number").get_to(declaration.number);
		j.at("typeIndex").get_to(declaration.typeIndex);
	}
	void to_json(nlohmann::json& j, const Function& function) {
		j["moduleName"] = function.moduleName;
		j["name"] = function.name;
		j["returnDeclaration"] = *function.returnDeclaration;
		j["parameterDeclarations"] = nlohmann::json::array();
		for (Declaration* declaration : function.parametersDeclarations) {
			j["parameterDeclarations"].emplace_back(*declaration);
		}
		j["typeIndex"] = function.typeIndex;
	}
	void from_json(const nlohmann::json& j, Function& function) {
		j.at("moduleName").get_to(function.moduleName);
		j.at("name").get_to(function.name);
		function.returnDeclaration = new Declaration();
		j.at("returnDeclaration").get_to(*function.returnDeclaration);
		for (const nlohmann::json& element : j.at("parameterDeclarations")) {
			Declaration* declaration = new Declaration();
			element.get_to(*declaration);
			function.parametersDeclarations.push_back(declaration);
		}
		j.at("typeIndex").get_to(function.typeIndex);
	}
	void to_json(nlohmann::json& j, const Manifest& manifest) {
		j["categories"] = nlohmann::json::array();
		for (Category* category : manifest.categories) {
			j["categories"].emplace_back(*category);
		}
		j["types"] = nlohmann::json::array();
		for (Type* type : manifest.types) {
			j["types"].emplace_back(*type);
		}
		j["uuids"] = nlohmann::json::array();
		for (UUID* uuid : manifest.uuids) {
			j["uuids"].emplace_back(*uuid);
		}
		j["structs"] = nlohmann::json::array();
		for (Struct* structt : manifest.structs) {
			j["structs"].emplace_back(*structt);
		}
		j["functions"] = nlohmann::json::array();
		for (Function* function : manifest.functions) {
			j["functions"].emplace_back(*function);
		}
	}
	void from_json(const nlohmann::json& j, Manifest& manifest) {
		for (const nlohmann::json& element : j.at("categories")) {
			Category* category = new Category();
			element.get_to(*category);
			manifest.categories.push_back(category);
		}
		for (const nlohmann::json& element : j.at("types")) {
			Type* type = new Type();
			element.get_to(*type);
			manifest.types.push_back(type);
		}
		for (const nlohmann::json& element : j.at("uuids")) {
			UUID* uuid = new UUID();
			element.get_to(*uuid);
			manifest.uuids.push_back(uuid);
		}
		for (const nlohmann::json& element : j.at("structs")) {
			Struct* structt = new Struct();
			element.get_to(*structt);
			manifest.structs.push_back(structt);
		}
		for (const nlohmann::json& element : j.at("functions")) {
			Function* function = new Function();
			element.get_to(*function);
			manifest.functions.push_back(function);
		}
	}
}

bool CheckTypeIndex(nlohmann::json& element, ApiManifest::Manifest& manifest) {
	if (element.is_string()) {
		INT32 index = manifest.FindTypeByName(element);
		if (index == -1) { std::cout << "Can not find type " << element << std::endl; return false; }
		element = index;
	}
	else if (element.is_number_integer()) {
		if (element >= manifest.types.size()) { std::cout << "Type index out of range " << element << std::endl; return false; }
	}
	else return false;
	return true;
}

int _tmain(int argc, _TCHAR* argv[]) {
	
	/*
		manifest落地
	*/
	//read json
	std::ifstream f("C:\\Users\\Administrator\\Desktop\\wininet.json");
	if (!f.good()) {
		OutputDebugString(TEXT("Error opening json file\r\n"));
		return -1;
	}
	nlohmann::json data = nlohmann::json::parse(f);
	f.close();
	//manifes构建
	ApiManifest::Manifest manifest;
	std::ifstream reader("C:\\Users\\Administrator\\Desktop\\logexts\\LogManifest.lgm", std::ifstream::binary);
	if (!reader.good()) {
		OutputDebugString(TEXT("Error opening old manifest file\r\n"));
		return -1;
	}
	manifest.Read(reader);
	reader.close();
	//json to bin
	//实例化类型，添加到manifest
	if (data.contains("categories")) {
		for (const nlohmann::json& element : data.at("categories")) {
			if (manifest.FindCategoryByName(element["name"]) == -1) {
				ApiManifest::Category* category = new ApiManifest::Category();
				category->name = element["name"];
				manifest.categories.push_back(category);
			}
			else {
				std::cout << element["name"] << " already exist";
			}
		}
	}
	if (data.contains("types")) {
		//检查节点合法性
		for (nlohmann::json& typeElement : data.at("types")) {
			if (manifest.FindTypeByName(typeElement["name"]) != -1) {
				std::cout << typeElement["name"] << " already exist" << std::endl;
				typeElement["isValid"] = false;
				continue;
			}
			if (!CheckTypeIndex(typeElement.at("parentIndex"), manifest)) {
				std::cout << "type index invalid" << std::endl;
				typeElement["isValid"] = false;
				continue;
			}
			typeElement["isValid"] = true;
		}
		for (const nlohmann::json& typeElement : data.at("types")) {
			if (typeElement["isValid"]) {
				ApiManifest::Type* type = new ApiManifest::Type();
				typeElement.get_to(*type);
				manifest.types.push_back(type);
			}
		}
	}
	if (data.contains("functions")) {
		//检查节点合法性
		for (nlohmann::json& functionElement : data.at("functions")) {
			if (manifest.FindFunctionByName(functionElement["name"]) != nullptr) {
				std::cout << functionElement["name"] << " already exist" << std::endl;
				functionElement["isValid"] = false;
				continue;
			}
			if (!CheckTypeIndex(functionElement.at("/returnDeclaration/typeIndex"_json_pointer), manifest)) {
				std::cout << "return declaration type index invalid" << std::endl;
				functionElement["isValid"] = false;
				continue;
			}
			for (nlohmann::json& parameterElement : functionElement.at("parameterDeclarations")) {
				if (!CheckTypeIndex(parameterElement.at("typeIndex"), manifest)) {
					std::cout << "parameter declaration type index invalid" << std::endl;
					functionElement["isValid"] = false;
					break;
				}
			}
			if(functionElement.find("isValid") == functionElement.end()) functionElement["isValid"] = true;
		}
		for (nlohmann::json& functionElement : data.at("functions")) {
			if (functionElement["isValid"]) {
				ApiManifest::Function* function = new ApiManifest::Function();
				functionElement.get_to(*function);
				manifest.functions.push_back(function);
			}
		}
	}
	//manifest落地
	std::ofstream writer("C:\\Users\\Administrator\\Desktop\\logexts\\LogManifest_wininet.lgm", std::ifstream::binary);
	if (!writer.good()) {
		OutputDebugString(TEXT("Error opening new manifest file\r\n"));
		return -1;
	}
	manifest.Write(writer);
	writer.close();
	return 0;
}