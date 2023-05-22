// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class MultiplayerShooterServerTarget : TargetRules
{
	public MultiplayerShooterServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.AddRange(new string[] { "MultiplayerShooter" });
		MultiplayerShooterTarget.ApplySharedMultiplayerShooterTargetSettings(this);

		//bUsesSteam = true;

		bUseChecksInShipping = true;
	}
}
