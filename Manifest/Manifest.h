#pragma once

#ifdef _WIN64
#define POINTERSIZE 8
#else
#define POINTERSIZE 4
#endif

namespace ApiManifest
{
	class Manifest;
	class ManifestType
	{
	public:
		static Manifest* p_manifest;
		std::string name;
	};

	class Flag : public ManifestType
	{
	public:
		INT32 unknown = 0;
		bool Read(std::ifstream& reader);
		bool Write(std::ofstream& writer);
	};

	class Value : public ManifestType
	{
	public:
		INT32 unknown1 = 0;
		INT32 unknown2 = 0;
		bool Read(std::ifstream& reader);
		bool Write(std::ofstream& writer);
	};

	class Struct;
	class Type : public ManifestType
	{
	public:
		// 这个变量指的是当前类型基类的索引，通过CalculateTypeSizeInMemory函数指示其所占用的空间
		INT32 typeIndex = 0;
		INT32 parentTypeIndex = 0;	//关联的类型*type -> type或者type alias -> type，目前就想到这两种情况
		INT32 isPointer = 0;
		std::vector<Flag*> flags;
		std::vector<Value*> values;
		INT32 structIndex = 0;
		INT32 IIDIndex = 0;
		std::vector<INT32> methodsIndex;
		INT32 unknown = 0;
		INT32 size = 0;

		~Type();
		bool Read(std::ifstream& reader);
		bool Write(std::ofstream& writer);

		Type* GetParentType();
		INT32 GetBaseType(bool isBase);
		INT32 CalculateSizeInMemory();
		Struct* GetStruct();
		INT32 CalculateTypeSizeInMemory();
	};

	class Declaration : public ManifestType
	{
	public:
		INT32 isOutParam = 0;
		INT32 typeIndex = 0;
		INT32 pointerRank = 0;
		INT32 number = 0;

		bool Read(std::ifstream& reader);
		bool Write(std::ofstream& writer);

		Type* GetType();
		INT32 GetSizeInMemory();
		INT32 GetSizeOnStack();
	};

	class Struct : public ManifestType
	{
	public:
		std::vector<Declaration*> declarations;

		~Struct();
		bool Read(std::ifstream& reader);
		bool Write(std::ofstream& writer);
	};

	class Category : public ManifestType
	{
	public:
		bool Read(std::ifstream& reader);
		bool Write(std::ofstream& writer);
	};
	
	class UUID : public ManifestType
	{
	public:
		CHAR uuid[16];
		INT32 unknown;

		UUID();
		bool Read(std::ifstream& reader);
		bool Write(std::ofstream& writer);
	};
	
	class Function : public ManifestType
	{
	public:
		INT32 index = 0;
		std::string moduleName;
		Declaration* returnDeclaration = nullptr;
		std::vector<Declaration*> parametersDeclarations;
		INT32 unknown = 0;
		INT32 categoryIndex = 0;
		INT32 typeIndex = 0;
		INT32 parametersSize = 0;

		~Function();
		bool Read(std::ifstream& reader);
		bool Write(std::ofstream& writer);
		//bool to_json(nlohmann::json &j);

		bool SetReturnDeclaration(Declaration *declaration);
		bool IsDeprecated();
		Type* GetComInterface();
	};

	class Manifest
	{
		public:
			CHAR fileHeader[0x42c];
			std::vector<Type*> types;
			std::vector<Category*> categories;
			std::vector<UUID*> uuids;
			std::vector<Struct*> structs;
			std::vector<Function*> functions;
			std::vector<Type*> iidTypes;
			std::map<std::string, Function*> functionNameTable;

			Manifest();
			~Manifest();

			bool Read(std::ifstream& reader);
			bool Write(std::ofstream& writer);
			bool RebuildLinks();

			Function* FindFunctionByName(LPCSTR functionName);
			Function* FindFunctionByName(const std::string& functionName);
			Function* AddFunction(std::string& decla);
			Declaration* AddDeclaration(std::string& decla);
			INT32 FindTypeByName(const std::string& typeName);
			INT32 FindCategoryByName(const std::string& typeName);
	};

	template <typename T> void ReadArray(std::vector<T*>& list, std::ifstream& reader);
	template <typename T> void WriteArray(std::vector<T*>& list, std::ofstream& writer);
	void GetString(std::ifstream& reader, std::string& s);
	void SetString(std::ofstream& writer, const std::string& s);
	INT32 ReadINT(std::ifstream& reader);
	void WriteINT(std::ofstream& writer, INT32 i);
}
