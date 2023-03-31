// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

class FAIntPHelpStyle
{
public:

	static void Initialize();

	static void Shutdown();

	/** reloads textures used by slate renderer */
	static void ReloadTextures();

	static const FSlateBrush * GetBrush(FName PropertyName, const ANSICHAR* Specifier = NULL)
	{
		return StyleInstance->GetBrush(PropertyName, Specifier);
	};

	/** @return The Slate style set for the Actor Interaction Plugin Help Button */
	static const ISlateStyle& Get();

	static FName GetStyleSetName();

private:

	static TSharedRef< class FSlateStyleSet > Create();

private:

	static TSharedPtr< class FSlateStyleSet > StyleInstance;
};
