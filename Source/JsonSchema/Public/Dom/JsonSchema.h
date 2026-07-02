#pragma once
#include "CoreMinimal.h"
#include "valijson/schema.hpp"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "valijson/adapters/rapidjson_adapter.hpp"
#include "rapidjson/document.h"


struct FValijsonSchema
{
	valijson::Schema Value;
};

struct JSONSCHEMA_API FJsonSchema
{
	friend class FJsonSchemaConverter;

private:
	TSharedPtr<FValijsonSchema> Schema = nullptr;
	bool InternalValidation(const rapidjson::Document& DataDoc, FString* FaliureReason = nullptr) const;

public:
	bool IsValid() const;
	explicit operator bool() const;

	bool Validate(const FString& DataJson, FString* FaliureReason = nullptr) const;
	bool Validate(const TSharedPtr<FJsonObject>& JsonObject, FString* FaliureReason = nullptr) const;
};


struct FRapidJsonConvertor
{
private:
	static void ConvertObject(
		const TSharedPtr<FJsonObject>& Obj, rapidjson::Value& Out, rapidjson::Document::AllocatorType& Alloc);
	static void Convert(
		const TSharedPtr<FJsonValue>& In, rapidjson::Value& Out, rapidjson::Document::AllocatorType& Alloc);

public:
	static void JsonObjectToRapidJson(const TSharedPtr<FJsonObject>& Obj, rapidjson::Document& Doc);
	static void JsonValueToRapidJson(const TSharedPtr<FJsonValue>& Val, rapidjson::Document& Doc);
	static bool JsonStringToRapidJson(const FString& String, rapidjson::Document& Doc, FString* FaliureReason);
};

class JSONSCHEMA_API FJsonSchemaConverter
{
public:
	static FJsonSchema JsonObjectToJsonSchema(const TSharedPtr<FJsonObject>& JsonObject);
	static FJsonSchema JsonObjectStringToJsonSchema(const FString& JsonObjectString, FString* FaliureReason);
};
