#include "Dom/JsonSchema.h"

#include "Dom/JsonValue.h"
THIRD_PARTY_INCLUDES_START
#include "valijson/schema_parser.hpp"
#include "valijson/validator.hpp"
#include "rapidjson/error/en.h"
THIRD_PARTY_INCLUDES_END


void FRapidJsonConvertor::ConvertObject(
	const TSharedPtr<FJsonObject>& Obj, rapidjson::Value& Out, rapidjson::Document::AllocatorType& Alloc)
{
	Out.SetObject();
	if (!Obj.IsValid()) return;

	for (const auto& Pair : Obj->Values)
	{
		FTCHARToUTF8 KeyUtf8(*Pair.Key);
		rapidjson::Value Key(
			KeyUtf8.Get(),
			static_cast<rapidjson::SizeType>(KeyUtf8.Length()),
			Alloc);

		rapidjson::Value Val;
		Convert(Pair.Value, Val, Alloc);
		Out.AddMember(Key, Val, Alloc);
	}
}

void FRapidJsonConvertor::Convert(
	const TSharedPtr<FJsonValue>& In, rapidjson::Value& Out, rapidjson::Document::AllocatorType& Alloc)
{
	if (!In.IsValid())
	{
		Out.SetNull();
		return;
	}

	switch (In->Type)
	{
	case EJson::None:
	case EJson::Null:
		Out.SetNull();
		break;

	case EJson::Boolean:
		Out.SetBool(In->AsBool());
		break;

	case EJson::Number:
		{
			// UE stores all numbers as double.
			// Promote whole numbers to int64 so valijson's integer check passes.
			const double D = In->AsNumber();
			if (const int64 I = static_cast<int64>(D); static_cast<double>(I) == D)
				Out.SetInt64(I);
			else
				Out.SetDouble(D);
			break;
		}

	case EJson::String:
		{
			const FTCHARToUTF8 Utf8(*In->AsString());
			Out.SetString(
				Utf8.Get(),
				static_cast<rapidjson::SizeType>(Utf8.Length()),
				Alloc);
			break;
		}

	case EJson::Array:
		{
			Out.SetArray();
			for (const TSharedPtr<FJsonValue>& Elem : In->AsArray())
			{
				rapidjson::Value ElemRj;
				Convert(Elem, ElemRj, Alloc);
				Out.PushBack(ElemRj, Alloc);
			}
			break;
		}

	case EJson::Object:
		ConvertObject(In->AsObject(), Out, Alloc);
		break;
	}
}

void FRapidJsonConvertor::JsonObjectToRapidJson(const TSharedPtr<FJsonObject>& Obj, rapidjson::Document& Doc)
{
	ConvertObject(Obj, static_cast<rapidjson::Value&>(Doc), Doc.GetAllocator());
}

void FRapidJsonConvertor::JsonValueToRapidJson(const TSharedPtr<FJsonValue>& Val, rapidjson::Document& Doc)
{
	Convert(Val, Doc, Doc.GetAllocator());
}

bool FRapidJsonConvertor::JsonStringToRapidJson(const FString& String, rapidjson::Document& Doc, FString* FaliureReason)
{
	Doc.Parse(TCHAR_TO_UTF8(*String));
	if (Doc.HasParseError() && FaliureReason)
	{
		*FaliureReason = FString::Printf(
			TEXT("Malformed JSON: %s (offset %zu)"),
			UTF8_TO_TCHAR(rapidjson::GetParseError_En(Doc.GetParseError())), Doc.GetErrorOffset());
		return false;
	}
	return true;
}

bool FJsonSchema::InternalValidation(const rapidjson::Document& DataDoc, FString* FaliureReason) const
{
	valijson::ValidationResults ValResults;
	const valijson::adapters::RapidJsonAdapter Adapter(DataDoc);

	if (valijson::Validator Validator; Validator.validate(Schema->Value, Adapter, &ValResults))
	{
		if (FaliureReason)
		{
			*FaliureReason = "";
			valijson::ValidationResults::Error ErrorItr;
			while (ValResults.popError(ErrorItr))
			{
				FString Path;
				for (const std::string& Crumb : ErrorItr.context) Path += UTF8_TO_TCHAR(Crumb.c_str());
				*FaliureReason +=
					FString::Printf(TEXT("[%s] %s, "), *Path, UTF8_TO_TCHAR(ErrorItr.description.c_str()));
			}
		}
		return false;
	}
	return true;
}

bool FJsonSchema::IsValid() const
{
	return Schema.IsValid();
}

FJsonSchema::operator bool() const
{
	return IsValid();
}

bool FJsonSchema::Validate(const FString& DataJson, FString* FaliureReason) const
{
	if (!Schema.IsValid())
	{
		if (FaliureReason) *FaliureReason = "Schema not initialized";
		return false;
	}

	rapidjson::Document Doc;
	if (!FRapidJsonConvertor::JsonStringToRapidJson(DataJson, Doc, FaliureReason)) return false;
	return InternalValidation(Doc, FaliureReason);
}

bool FJsonSchema::Validate(const TSharedPtr<FJsonObject>& JsonObject, FString* FaliureReason) const
{
	if (!Schema.IsValid())
	{
		if (FaliureReason) *FaliureReason = "Schema not initialized";
		return false;
	}
	if (!JsonObject.IsValid())
	{
		if (FaliureReason) *FaliureReason = "Null FJsonObject";
		return false;
	}

	rapidjson::Document Doc;
	FRapidJsonConvertor::JsonObjectToRapidJson(JsonObject, Doc);
	return InternalValidation(Doc, FaliureReason);
}

FJsonSchema FJsonSchemaConverter::JsonObjectToJsonSchema(const TSharedPtr<FJsonObject>& JsonObject)
{
	FJsonSchema RetVal;

	rapidjson::Document SchemaDoc;
	FRapidJsonConvertor::JsonObjectToRapidJson(JsonObject, SchemaDoc);

	RetVal.Schema = MakeShared<FValijsonSchema>();
	valijson::SchemaParser Parser;
	const valijson::adapters::RapidJsonAdapter Adapter(SchemaDoc);
	Parser.populateSchema(Adapter, RetVal.Schema->Value);
	return RetVal;
}

FJsonSchema FJsonSchemaConverter::JsonObjectStringToJsonSchema(const FString& JsonObjectString, FString* FaliureReason)
{
	FJsonSchema RetVal;

	rapidjson::Document SchemaDoc;
	if (!FRapidJsonConvertor::JsonStringToRapidJson(JsonObjectString, SchemaDoc, FaliureReason)) return {};

	try
	{
		valijson::SchemaParser Parser;
		const valijson::adapters::RapidJsonAdapter Adapter(SchemaDoc);
		Parser.populateSchema(Adapter, RetVal.Schema->Value);
		return RetVal;
	}
	catch (const std::exception& Ex)
	{ if (FaliureReason) *FaliureReason = UTF8_TO_TCHAR(Ex.what()); }
	catch (...) { if (FaliureReason) *FaliureReason = TEXT("Unknown exception in populateSchema()"); }
	return RetVal;
}
