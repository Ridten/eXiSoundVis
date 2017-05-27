using UnrealBuildTool;
using System.IO;

public class eXiSoundVis : ModuleRules
{
	public eXiSoundVis(ReadOnlyTargetRules Target) : base (Target)
	{
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(new string[] { "eXiSoundVis/Public" });
        PrivateIncludePaths.AddRange(new string[] {	"eXiSoundVis/Private" });
		
		
		PublicDependencyModuleNames.AddRange(new string[] { "Engine", "Core", "CoreUObject", "InputCore", "RHI", "Kiss_FFT" });


        AddEngineThirdPartyPrivateStaticDependencies(Target, "Kiss_FFT");
        

    }
}