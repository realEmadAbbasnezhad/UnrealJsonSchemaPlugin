using System.IO;
using UnrealBuildTool;

public class JsonSchema : ModuleRules
{
	public JsonSchema(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.NoPCHs;
		bEnableExceptions = true;
		CppStandard = CppStandardVersion.Cpp23;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		PublicDependencyModuleNames.AddRange(["Core", "CoreUObject", "Engine", "Json", "RapidJSON"]);
		PublicDefinitions.Add("VALIJSON_USE_EXCEPTIONS=0");

		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "../../ThirdParty/valijson/include")); // Valijson
	}
}
