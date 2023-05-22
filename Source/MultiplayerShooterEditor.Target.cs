// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class MultiplayerShooterEditorTarget : TargetRules
{
	public MultiplayerShooterEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.AddRange( new string[] { "MultiplayerShooter" } );
		MultiplayerShooterTarget.ApplySharedMultiplayerShooterTargetSettings(this);



		EnablePlugins.Add("RemoteSession");
	}
}
