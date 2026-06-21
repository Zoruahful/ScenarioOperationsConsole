using UnrealBuildTool;

public class ScenarioOperationsConsole : ModuleRules
{
	public ScenarioOperationsConsole(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Json",
			"MissionScenarioRuntime"
		});
	}
}
