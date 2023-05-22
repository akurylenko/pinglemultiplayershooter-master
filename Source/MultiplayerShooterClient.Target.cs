// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class MultiplayerShooterClientTarget : TargetRules
{
	public MultiplayerShooterClientTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Client;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.AddRange(new string[] { "MultiplayerShooter" });
		MultiplayerShooterTarget.ApplySharedMultiplayerShooterTargetSettings(this);
		//bUsesSteam = true;
	}
}
