// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Shake_TeleportationArea.generated.h"

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
class SHAKEVR_API AShake_TeleportationArea : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AShake_TeleportationArea();

	//VARIABLES
private:
	TArray<MeshTriangle> VisualizationTriangles;
	float StandardSize = 50.f;

	//FUNCTIONS
private:

	//VARIABLES
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
		float OutlineWidth = 2.f;
	//FUCNTIONS
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/// <summary>
	/// Calls all the other functions to generate the area
	/// </summary>
	void CreateTeleportationArea();


	/// <summary>
	/// Creates starting array of reflex, convex, ear vertices. Returns the final mesh triangles
	/// </summary>
	void TriangulateSpline();
	/// <summary>
	/// Create the procedural mesh based on the spline data (data produced by CreateTeleportationArea function)
	/// </summary>
	void CreateAreaMesh();
	/// <summary>
	/// Creates instances of meshes to create an outline effect around the generated area
	/// </summary>
	void CreateAreaOutline();


	/// <summary>
	/// Clears the current spline and adds 4 linear points in the shape of a square based on the StandardSize variable
	/// </summary>
	void ClearSpline() const;
	/// <summary>
	/// Returns a TArray with all the positional data of the spline
	/// </summary>
	TArray<FVector> GetSplinePoints();


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
	/// <param name="convexIndices"> Empty array that will hold the reflex points afterwards </param>
	/// <param name="reflexIndices"> Empty array that will hold the convex points afterwards </param>
	/// <param name="earIndices"> Empty array that will hold the earPoints afterwards </param>
	void TrianglesFromPoints(TArray<FVector>& splinePoints, const TArray<int>& convexIndices, const TArray<int>& reflexIndices, const TArray<
		int>& earIndices);

	/// <summary>
	/// Creates an index list from the triangle points
	/// </summary>
	/// <param name="triangles"> Needs a copy of the positional data of the spline </param>
	/// <param name="vertices"> Empty array that will hold the reflex points afterwards </param>
	/// <param name="indices"> Empty array that will hold the convex points afterwards </param>
	void TrianglesToIndices(const TArray<MeshTriangle>& triangles, TArray<FVector>& vertices, TArray<int>& indices) const;

	//FUNCTIONS
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform & Transform);

	UFUNCTION(BlueprintCallable)
		/// <summary>
		/// Disables/Enables the collision en visibility of the teleportation area based on the boolean
		/// </summary>
		void SetAreaActive(bool newState);
};
