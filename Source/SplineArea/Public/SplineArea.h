﻿// Robin Smekens 2021 

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
