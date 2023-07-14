// Copyright Epic Games, Inc. All Rights Reserved.

#include "AntPlusPlugin.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "ue5_lib.h"

#define LOCTEXT_NAMESPACE "FAntPlusPluginModule"

void FAntPlusPluginModule::StartupModule()
{
    // This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
    //ue5_lib__startupAntPlusLib();
}

void FAntPlusPluginModule::ShutdownModule()
{
    // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
    // we call this function before unloading the module.
    //ue5_lib__endAntPlusLib();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAntPlusPluginModule, AntPlusPlugin)
