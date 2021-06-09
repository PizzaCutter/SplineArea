// Copyright 2021 Robin Smekens

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSplineAreaModule : public IModuleInterface
{
public:

    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
