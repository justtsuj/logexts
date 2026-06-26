#include "pch.h"
#include "Manifest.h"

using namespace ApiManifest;

Manifest* ManifestType::p_manifest = nullptr;

bool Flag::Read(std::ifstream& reader){
    unknown = ReadINT(reader);
    GetString(reader, name);
    return true;
}

bool ApiManifest::Flag::Write(std::ofstream& writer)
{
    WriteINT(writer, unknown);
    SetString(writer, name);
    return false;
}

bool Value::Read(std::ifstream& reader) {
    unknown1 = ReadINT(reader);
    unknown2 = ReadINT(reader);
    GetString(reader, name);
    return true;
}

bool ApiManifest::Value::Write(std::ofstream& writer)
{
    WriteINT(writer, unknown1);
    WriteINT(writer, unknown2);
    SetString(writer, name);
    return false;
}

bool Declaration::Read(std::ifstream& reader) {
    isOutParam = ReadINT(reader);
    GetString(reader, name);
    pointerRank = ReadINT(reader);
    number = ReadINT(reader);
    typeIndex = ReadINT(reader);
    return true;
}

bool ApiManifest::Declaration::Write(std::ofstream& writer)
{
    WriteINT(writer, isOutParam);
    SetString(writer, name);
    WriteINT(writer, pointerRank);
    WriteINT(writer, number);
    WriteINT(writer, typeIndex);
    return true;
}

Type* Declaration::GetType() {
    if (typeIndex == -1) return nullptr;
    return p_manifest->types[typeIndex];
}

int Declaration::GetSizeInMemory() {
    if (pointerRank) return POINTERSIZE;
    return GetType()->size;
}

int Declaration::GetSizeOnStack() {
    int size = GetSizeInMemory();
    if (size % POINTERSIZE) size += POINTERSIZE - (size % POINTERSIZE);
    return size;
}

Struct::~Struct()
{
    for (Declaration* declaration : declarations) delete declaration;
}

bool Struct::Read(std::ifstream& reader) {
    ReadArray(declarations, reader);
    return true;
}

bool ApiManifest::Struct::Write(std::ofstream& writer)
{
    WriteArray(declarations, writer);
    return false;
}

Type::~Type()
{
    for (Flag* flag : flags) delete flag;
    for (Value* value : values) delete value;
}

bool Type::Read(std::ifstream&reader){
    typeIndex = ReadINT(reader);
    parentTypeIndex = ReadINT(reader);
    GetString(reader, name);
    isPointer = ReadINT(reader);
    INT32 numOfFlags = ReadINT(reader);
    for (INT32 i = 0; i < numOfFlags; ++i) {
        Flag *flag = new Flag();
        flags.push_back(flag);
        flag->Read(reader);
    }
    INT32 numOfValues = ReadINT(reader);
    for (INT32 i = 0; i < numOfValues; ++i) {
        Value *value = new Value();
        values.push_back(value);
        value->Read(reader);
    }
    structIndex = ReadINT(reader);
    IIDIndex = ReadINT(reader);
    INT32 numOfMethod = ReadINT(reader);
    for(INT32 i = 0; i < numOfMethod; ++i){
        methodsIndex.push_back(ReadINT(reader));
    }
    unknown = ReadINT(reader);
    return true;
}

bool ApiManifest::Type::Write(std::ofstream& writer)
{
    WriteINT(writer, typeIndex);
    WriteINT(writer, parentTypeIndex);
    SetString(writer, name);
    WriteINT(writer, isPointer);
    WriteINT(writer, flags.size());
    for (Flag* flag : flags) {
        flag->Write(writer);
    }
    WriteINT(writer, values.size());
    for (Value* value : values) {
        value->Write(writer);
    }
    WriteINT(writer, structIndex);
    WriteINT(writer, IIDIndex);
    WriteINT(writer, methodsIndex.size());
    for (INT32 i : methodsIndex) {
        WriteINT(writer, i);
    }
    WriteINT(writer, unknown);
    return false;
}

int Type::CalculateTypeSizeInMemory() {
    int result;
    switch (typeIndex)
    {
    case 2:
    case 9:
        result = 1;
        break;
    case 3:
    case 0xA:
        result = 2;
        break;
    case 4:
    case 5:
    case 6:
    case 7:
    case 0xB:
    case 0xC:
    case 0xF:
    case 0x10:
    case 0x12:
        result = 4;
        break;
    case 8:
        result = 8;
        break;
    case 0xE:
        result = 16;
        break;
    default:
        result = 0;
        break;
    }
    return result;
}

Type* Type::GetParentType(){
    if(parentTypeIndex == -1) return nullptr;
    return p_manifest->types[parentTypeIndex];
}

int Type::GetBaseType(bool isBase) {
    // 这个isBase很奇怪，不是通过成员来判断它是否是base类型
    if (!isBase) return typeIndex;
    Type* type = this;
    while (type->parentTypeIndex != -1) {
        type = p_manifest->types[type->parentTypeIndex];
    }
    return type->typeIndex;
}

int Type::CalculateSizeInMemory(){
    Type* type = this;
    while (type->parentTypeIndex != -1) {
        type = p_manifest->types[type->parentTypeIndex];
    }
    return type->CalculateTypeSizeInMemory();
}

Struct* Type::GetStruct(){
    if(structIndex == -1) return nullptr;
    return p_manifest->structs[structIndex];
}

bool Category::Read(std::ifstream& reader){
    GetString(reader, name);
    return true;
}

bool ApiManifest::Category::Write(std::ofstream& writer)
{
    SetString(writer, name);
    return false;
}

UUID::UUID() {
    memset(uuid, 0, 16);
}


bool UUID::Read(std::ifstream& reader){
    GetString(reader, name);
    UINT16 data_len = 0;
    reader.read(reinterpret_cast<char*>(&data_len), 2);
    data_len = data_len >> 6;
    if (data_len != 16) throw "read uuid fail";
    reader.read(uuid, 16);
    unknown = ReadINT(reader);
    return true;
}

bool ApiManifest::UUID::Write(std::ofstream& writer)
{
    SetString(writer, name);
    UINT16 data_len = 16;
    data_len = data_len << 6;
    writer.write(reinterpret_cast<const char*>(&data_len), 2);
    writer.write(uuid, 16);
    WriteINT(writer, unknown);
    return true;
}

Function::~Function()
{
    for (Declaration* declaration : parametersDeclarations) delete declaration;
    delete returnDeclaration; returnDeclaration = nullptr;
}

bool Function::Read(std::ifstream& reader){
    GetString(reader, moduleName);
    if(!moduleName.empty() && moduleName.starts_with("API_MS_")){
        moduleName[3] = '-';
        moduleName[6] = '-';
    }
    GetString(reader, name);
    Declaration *declaration = new Declaration();
    SetReturnDeclaration(declaration);
    declaration->Read(reader);
    ReadArray(parametersDeclarations, reader);
    unknown = ReadINT(reader);
    categoryIndex = ReadINT(reader);
    typeIndex = ReadINT(reader);
    return true;
}

bool ApiManifest::Function::Write(std::ofstream& writer)
{
    SetString(writer, moduleName);
    SetString(writer, name);
    returnDeclaration->Write(writer);
    WriteArray(parametersDeclarations, writer);
    WriteINT(writer, unknown);
    WriteINT(writer, categoryIndex);
    WriteINT(writer, typeIndex);
    return true;
}

bool Function::SetReturnDeclaration(Declaration *declaration){
    if(returnDeclaration){
        delete returnDeclaration;
        returnDeclaration = nullptr;
    }
    returnDeclaration = declaration;
    return true;
}

Type* Function::GetComInterface() {
    if (typeIndex == -1) return nullptr;
    return p_manifest->types[typeIndex];
}

bool Function::IsDeprecated() {
    if (name.length() < 12) return false;
    return name.ends_with("_deprecated");
}

Manifest::Manifest() {
    ManifestType::p_manifest = this;
    memset(fileHeader, 0, 0x42c);
}

Manifest::~Manifest() {    
    for (Function* function : functions) delete function;
    for (Struct* structt : structs) delete structt;
    for (UUID* uuid : uuids) delete uuid;
    for (Category* category : categories) delete category;
    for (Type* type : types) delete type;
    ManifestType::p_manifest = nullptr;
}

bool Manifest::Read(std::ifstream &reader){
    try
    {
        reader.read(fileHeader, 0x42c);
        if (memcmp(fileHeader, "\x25\x52\x22\x00", 4)) {
            OutputDebugString(TEXT("Error bad manifest"));
            return false;
        }
        if (memcmp(fileHeader + 0x4, "\x10\x10\x00\x00", 4)) {
            OutputDebugString(TEXT("Error bad manifest version"));
            return false;
        }
        ReadArray(categories, reader);
        ReadArray(types, reader);
        ReadArray(uuids, reader);
        ReadArray(structs, reader);
        ReadArray(functions, reader);
    }
    catch (...)
    {
        return false;
    }
    RebuildLinks();
    return true;
}

bool ApiManifest::Manifest::Write(std::ofstream& writer)
{
    writer.write(fileHeader, 0x42c);
    WriteArray(categories, writer);
    WriteArray(types, writer);
    WriteArray(uuids, writer);
    WriteArray(structs, writer);
    WriteArray(functions, writer);
    return true;
}

bool Manifest::RebuildLinks(){
    for(Type* type : types){
        if(type->IIDIndex != -1){
            iidTypes.push_back(type);
        }
        if(type->isPointer){
            type->size = 4;
        }
        else if(type->GetBaseType(true) == 13){
            int size = 0;
            Struct *structt = type->GetStruct();
            for(Declaration* declaration : structt->declarations){
                size += declaration->GetSizeInMemory();
            }
            type->size = size;
        }
        else{
            type->size = type->CalculateSizeInMemory();
        }
    }
    for(unsigned int i = 0; i < functions.size(); i++){
        Function *function = functions[i];
        function->index = i;
        int size = 0;
        for(Declaration *declaration : function->parametersDeclarations){
            size += declaration->GetSizeOnStack();
        } 
        function->parametersSize = size;
    }
    return true;
}

Function* Manifest::FindFunctionByName(LPCSTR functionName) {
    for (ApiManifest::Function* function : functions) {
        if (!function->name.compare(functionName))
            return function;
    }
    return nullptr;
}

Function* ApiManifest::Manifest::FindFunctionByName(const std::string& functionName)
{
    for (ApiManifest::Function* function : functions) {
        if (function->name == functionName)
            return function;
    }
    return nullptr;
}

Function* ApiManifest::Manifest::AddFunction(std::string& decla)
{
    Function* function = new Function();
    std::string::size_type n;
    //跳过前导的空白字符
    for (n = 0; n < decla.length(); ++n) {
        if (decla[n] != ' ') break;
    }
    std::string returnDeclaration;
    //读取返回值类型直到空白字符
    for (; n < decla.length(); ++n) {
        if (decla[n] == ' ') break;
        returnDeclaration += decla[n];
    }
    if (returnDeclaration.empty()) { delete function; return nullptr; }
    Declaration* declaration = AddDeclaration(returnDeclaration);
    if(!declaration) { delete function; return nullptr; }
    function->returnDeclaration = declaration;
    //跳过返回值声明后的空格
    for (; n < decla.length(); ++n) {
        if (decla[n] != ' ') break;
    }
    std::string functionName;
    //读取函数名直到空格或左括号
    for (; n < decla.length(); ++n) {
        if (decla[n] == ' ' || decla[n] == '(')  break;
        functionName += decla[n];
    }
    if (functionName.empty()) { delete function; return nullptr; }
    function->name = functionName;
    //跳过函数名后的空格
    for (; n < decla.length(); ++n) {
        if (decla[n] != ' ' && decla[n] != '(') break;
    }
    std::string parameters;
    for (; n < decla.length(); ++n) {
        if (decla[n] == ')') break;
        parameters += decla[n];
    }
    if (parameters.empty()) { delete function; return nullptr; }
    n = 0;
    std::vector<std::string>parametersList;
    std::string curParameter = parametersList.emplace_back();
    for (; n < parameters.length(); ++n) {
        if (parameters[n] == ',') {
            curParameter = parametersList.emplace_back();
        }
        else
        {
            curParameter += parameters[n];
        }
    }
    for (std::string& s : parametersList) {
        boost::trim(s);
        declaration = AddDeclaration(s);
        if (!declaration) { delete function; return nullptr; }
        function->parametersDeclarations.push_back(declaration);
    }
    return function;
}

Declaration* ApiManifest::Manifest::AddDeclaration(std::string& decla)
{
    Declaration* declaration = new Declaration();
    std::vector<std::string> splitVec;
    boost::split(splitVec, decla, boost::is_any_of(" "), boost::token_compress_on);
    if (splitVec.size() == 3) {
        if (splitVec[0] == "[out]") declaration->isOutParam = 1;
        INT32 index = FindTypeByName(splitVec[1]);
        if (index == -1) { delete declaration; return nullptr; }
        declaration->typeIndex = index;
        declaration->name = splitVec[2];
        declaration->pointerRank = 1;
        declaration->number = 1;
    }
    return declaration;
}

INT32 ApiManifest::Manifest::FindTypeByName(const std::string& typeName)
{
    UINT32 i = 0;
    for (Type* type : types) {
        if (type->name == typeName) return i;
        ++i;
    }
    return -1;
}

INT32 ApiManifest::Manifest::FindCategoryByName(const std::string& typeName)
{
    UINT32 i = 0;
    for (Category* category : categories) {
        if (category->name == typeName) return i;
        ++i;
    }
    return -1;
}

template <typename T> void ApiManifest::ReadArray(std::vector<T*>& list, std::ifstream &reader){
    INT32 num = ReadINT(reader);
    for(INT32 i = 0; i < num; ++i){
        T* t = new T();
        list.push_back(t);
        t->Read(reader);
    }
}

template <typename T> void ApiManifest::WriteArray(std::vector<T*>& list, std::ofstream& writer) {
    WriteINT(writer, list.size());
    for (T* t : list) {
        t->Write(writer);
    }
}

void ApiManifest::GetString(std::ifstream &reader, std::string& s){
    UINT16 data_len = 0;
    reader.read(reinterpret_cast<char*>(&data_len), 2);
    data_len = data_len >> 6;
    for (UINT16 i = 0; i < data_len; ++i) {
        char c = reader.get();
        if(c != '\0') s += c;
    }
}

void ApiManifest::SetString(std::ofstream& writer, const std::string& s)
{
    UINT16 data_len = s.length() + 1;
    data_len = data_len << 6;
    writer.write(reinterpret_cast<const char*>(&data_len), 2);
    for (char c : s) { writer.put(c); }
    writer.put('\0');
}

INT32 ApiManifest::ReadINT(std::ifstream &reader){
    UINT16 data_len = 0;
    reader.read(reinterpret_cast<char*>(&data_len), 2);
    data_len = data_len >> 6;
    if (data_len != 4) throw "read int fail";
    INT32 result = 0;
    reader.read(reinterpret_cast<char*>(&result), 4);
    return result;
}

void ApiManifest::WriteINT(std::ofstream& writer, INT32 i)
{
    UINT16 data_len = 4;
    data_len = data_len << 6;
    writer.write(reinterpret_cast<const char*>(&data_len), 2);
    writer.write(reinterpret_cast<const char*>(&i), 4);
}