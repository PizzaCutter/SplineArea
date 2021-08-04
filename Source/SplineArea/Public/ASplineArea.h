// Copyright 2021 Robin Smekens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASplineArea.generated.h"

class USplineComponent;
class UProceduralMeshComponent;
class UMaterialInterface;
class UInstancedStaticMeshComponent;

struct MeshTriangle
{
    FVector point1 = FVector();
    FVector point2 = FVector();
    FVector point3 = FVector();
};

UCLASS()
class SPLINEAREA_API ASplineArea : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    ASplineArea();

private:
    TArray<MeshTriangle> VisualizationTriangles;
    float StandardSize = 50.f;

protected:
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = Default)
    USplineComponent* pSpline = nullptr;
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = Default)
    UProceduralMeshComponent* pAreaMesh;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Default)
    UMaterialInterface* pAreaMeshMaterial = nullptr;
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = Default)
    UInstancedStaticMeshComponent* pAreaOutline = nullptr;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Default)
    UMaterialInterface* pAreaOutlineMaterial = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Default)
    bool bEnableOutline = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Default, meta = (EditCondition = "bEnableOutline"))
    float OutlineWidth = 2.f;

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    /// <summary>
    /// Creates starting array of reflex, convex, ear vertices. Returns the final mesh triangles
    /// </summary>
    void TriangulateSpline();
    /// <summary>
    /// Create the procedural mesh based on the spline data (data produced by CreateTeleportationArea function)
    /// </summary>
    void CreateAreaMesh() const;
    /// <summary>
    /// Creates instances of meshes to create an outline effect around the generated area
    /// </summary>
    void CreateAreaOutline() const;

    /// <summary>
    /// Figures out which spline points are convex, reflex, ear. Based on this we create an index table for each type.
    /// </summary>
    /// <param name="splinePoints"> Needs a copy of the positional data of the spline </param>
    /// <param name="reflexPoints"> Empty array that will hold the reflex points afterwards </param>
    /// <param name="convexPoints"> Empty array that will hold the convex points afterwards </param>
    /// <param name="earPoints"> Empty array that will hold the earPoints afterwards </param>
    /// <param name="reflexIndices"> Empty array that will hold the reflex indices </param>
    /// <param name="convexIndices"> Empty array that will hold the convex indices </param>
    /// <param name="earIndices"> Empty array that will hold the ear indices </param>
    void GetPolygonComponents(const TArray<FVector>& splinePoints,
                              TArray<FVector>& reflexPoints, TArray<FVector>& convexPoints, TArray<FVector>& earPoints,
                              TArray<int>& reflexIndices, TArray<int>& convexIndices, TArray<int>& earIndices) const;

    /// <summary>
    /// From the different types of vertices can figure out how the triangles need to laid out for the mesh
    /// </summary>
    /// <param name="splinePoints"> Needs a copy of the positional data of the spline </param>
    /// <param name="inConvexIndices"> Empty array that will hold the reflex points afterwards </param>
    /// <param name="inReflexIndices"> Empty array that will hold the convex points afterwards </param>
    /// <param name="inEarIndices"> Empty array that will hold the earPoints afterwards </param>
    void TrianglesFromPoints(TArray<FVector>& splinePoints, const TArray<int>& inConvexIndices,
                             const TArray<int>& inReflexIndices, const TArray<
                                 int>& inEarIndices);

    /// <summary>
    /// Creates an index list from the triangle points
    /// </summary>
    /// <param name="triangles"> Needs a copy of the positional data of the spline </param>
    /// <param name="vertices"> Empty array that will hold the reflex points afterwards </param>
    /// <param name="indices"> Empty array that will hold the convex points afterwards </param>
    void TrianglesToIndices(const TArray<MeshTriangle>& triangles, TArray<FVector>& vertices,
                            TArray<int>& indices) const;

    //FUNCTIONS
public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;
    virtual void OnConstruction(const FTransform& Transform);

    /// <summary>
    /// Disables/Enables the collision en visibility of the teleportation area based on the boolean
    /// </summary>
    UFUNCTION(BlueprintCallable)
    void SetAreaActive(bool newState) const;
    
    /// <summary>
    /// Calls all the other functions to generate the area
    /// </summary>
    UFUNCTION(BLueprintCallable)
    void CreateTeleportationArea();
    
    /// <summary>
    /// Clears the current spline and adds 4 linear points in the shape of a square based on the StandardSize variable
    /// </summary>
    UFUNCTION(BlueprintCallable)
    void ClearSpline() const;
    
    /// <summary>
    /// Returns a TArray with all the positional data of the spline
    /// </summary>
    UFUNCTION(BlueprintPure)
    TArray<FVector> GetSplinePoints() const;
};
