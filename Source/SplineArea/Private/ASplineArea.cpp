// Robin Smekens 2021 

#include "ASplineArea.h"
#include "ProceduralMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Materials/MaterialInterface.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ASplineArea::ASplineArea()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    pAreaMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("AreaMesh"));
    pSpline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
    pAreaOutline = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("AreaOutline"));

    pAreaMesh->SetupAttachment(RootComponent);
    pSpline->SetupAttachment(RootComponent);
    pAreaOutline->SetupAttachment(RootComponent);

    pAreaMesh->bUseAsyncCooking = true;
    ClearSpline();
    pAreaOutline->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (pAreaOutline == nullptr)
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> foundMesh(
            TEXT("StaticMesh'/SplineArea/Models/SM_Unit_Plane.SM_Unit_Plane'"));
        if (foundMesh.Succeeded())
            pAreaOutline->SetStaticMesh(foundMesh.Object);
    }

    if (pAreaMeshMaterial == nullptr)
    {
        static ConstructorHelpers::FObjectFinder<UMaterialInterface> foundMaterial(
            TEXT("Material'/SplineArea/Materials/M_SplineArea.M_SplineArea'"));
        if (foundMaterial.Succeeded())
            pAreaMeshMaterial = foundMaterial.Object;
    }

    if (pAreaOutlineMaterial == nullptr)
    {
        static ConstructorHelpers::FObjectFinder<UMaterialInterface> foundMaterial(
            TEXT("Material'/SplineArea/Materials/M_SplineArea_Outline.M_SplineArea_Outline'"));
        if (foundMaterial.Succeeded())
        {
            pAreaOutlineMaterial = foundMaterial.Object;
            pAreaOutline->SetMaterial(0, pAreaOutlineMaterial);
        }
    }
}

// Called when the game starts or when spawned
void ASplineArea::BeginPlay()
{
    Super::BeginPlay();
}

// Called every frame
void ASplineArea::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ASplineArea::OnConstruction(const FTransform& Transform)
{
    CreateTeleportationArea();

    for (int i = 0; i < pSpline->GetNumberOfSplinePoints(); i++)
    {
        pSpline->SetSplinePointType(i, ESplinePointType::Linear, false);
    }
}

void ASplineArea::CreateTeleportationArea()
{
    VisualizationTriangles.Reset(0);
    TArray<MeshTriangle> meshTriangles;

    TriangulateSpline();

    CreateAreaMesh();
    CreateAreaOutline();
}

void ASplineArea::ClearSpline() const
{
    pSpline->ClearSplinePoints(true);

    TArray<FSplinePoint> splinePoints;
    splinePoints.Add(FSplinePoint(0, FVector(-StandardSize, -StandardSize, 0.f)));
    splinePoints.Add(FSplinePoint(1, FVector(StandardSize, -StandardSize, 0.f)));
    splinePoints.Add(FSplinePoint(2, FVector(StandardSize, StandardSize, 0.f)));
    splinePoints.Add(FSplinePoint(3, FVector(-StandardSize, StandardSize, 0.f)));

    pSpline->AddPoints(splinePoints, false);
    for (int i = 0; i < pSpline->GetNumberOfSplinePoints(); i++)
    {
        pSpline->SetSplinePointType(i, ESplinePointType::Linear, false);
    }

    pSpline->SetClosedLoop(true);
}

/// <summary>
/// If the index is larger then length it will loop around
/// </summary>
/// <param name="index"> index to transform </param>
/// <param name="length"> maximum value -> length of the array </param>
inline int CircularIndex(const int index, const int length)
{
    int temp = index % length;
    int temp1 = temp + length;
    if (temp < 0)
        return temp1;

    return temp;
}

/// <summary>
/// Checks if this is a valid point
/// </summary>
/// <param name="t1"> first point </param>
/// <param name="t2"> second point </param>
/// <param name="t3"> third point </param>
inline bool PointIsConvex(FVector prevPoint, FVector curPoint, FVector nextPoint)
{
    FVector temp1 = prevPoint - curPoint;
    FVector temp2 = nextPoint - curPoint;
    temp1.Normalize();
    temp2.Normalize();
    FVector temp3 = FVector::CrossProduct(temp1, temp2);

    return temp3.Z < 0;
}

/// <summary>
/// Checks if this is a valid point
/// </summary>
/// <param name="t1"> first point </param>
/// <param name="t2"> second point </param>
/// <param name="t3"> third point </param>
inline bool IsPointInTriangle(FVector t1, FVector t2, FVector t3, FVector p)
{
    //Get two edges of the triangle and the line to the point
    const FVector v1 = t3 - t1;
    const FVector v2 = t2 - t1;
    const FVector v3 = p - t1;

    //Convert the triangle edges to a Barycentric Coordinate System
    const float daa = FVector::DotProduct(v1, v1);
    const float dab = FVector::DotProduct(v1, v2);
    const float dac = FVector::DotProduct(v1, v3);
    const float dbb = FVector::DotProduct(v2, v2);
    const float dbc = FVector::DotProduct(v2, v3);

    float u = (((dbb * dac) - (dab * dbc)) / ((daa * dbb) - (dab * dab)));
    float v = (((daa * dbc) - (dab * dac)) / ((daa * dbb) - (dab * dab)));

    //If the position of the point in the barycentric coord sys is greater than 1 is is not inside the triangle
    return (v >= 0.f) && (u >= 0.f) && ((u + v) < 1.f);
}

/// <summary>
/// Check if any reflex points are inside the triangle created by the 3 points if so it is an ear
/// </summary>
/// <param name="curPointIndex"> Point index that you want to check </param>
/// <param name="reflexIndices"> Array that contains all the known reflex indices </param>
/// <param name="splinePoints"> Array that contains all the spline points positional data </param>
inline bool IsPointAnEar(const int curPointIndex, const TArray<int>& reflexIndices, const TArray<FVector>& splinePoints)
{
    bool IsEar = true;

    const int prevIndex = CircularIndex(curPointIndex - 1, splinePoints.Num());
    const int nextIndex = CircularIndex(curPointIndex + 1, splinePoints.Num());

    const FVector curPoint = splinePoints[curPointIndex];
    const FVector prevPoint = splinePoints[prevIndex];
    const FVector nextPoint = splinePoints[nextIndex];

    for (auto i = 0; i < reflexIndices.Num(); i++)
    {
        IsEar = !IsPointInTriangle(curPoint, prevPoint, nextPoint, splinePoints[reflexIndices[i]]);
        if (!IsEar)
            return IsEar;
    }
    return IsEar;
}

void ASplineArea::TriangulateSpline()
{
    TArray<FVector> splinePoints = GetSplinePoints();
    TArray<FVector> reflexPoints = TArray<FVector>{};
    TArray<FVector> convexPoints = TArray<FVector>{};
    TArray<FVector> earPoints = TArray<FVector>{};

    TArray<int> reflexIndices = TArray<int>{};
    TArray<int> convexIndices = TArray<int>{};
    TArray<int> earIndices = TArray<int>{};

    GetPolygonComponents(splinePoints, reflexPoints, convexPoints, earPoints, reflexIndices, convexIndices, earIndices);

    reflexPoints.Reset();
    convexPoints.Reset();
    earPoints.Reset();

    TrianglesFromPoints(splinePoints, convexIndices, reflexIndices, earIndices);
}

void ASplineArea::GetPolygonComponents(const TArray<FVector>& splinePoints, TArray<FVector>& reflexPoints,
                                       TArray<FVector>& convexPoints, TArray<FVector>&
                                       earPoints, TArray<int>& reflexIndices, TArray<int>& convexIndices,
                                       TArray<int>& earIndices) const
{
    //Testing if point is convex or reflex
    {
        FVector prevPoint = FVector();
        FVector nextPoint = FVector();
        for (int curIndex = 0; curIndex < splinePoints.Num(); curIndex++)
        {
            FVector curPoint = splinePoints[curIndex];
            const int prevIndex = CircularIndex(curIndex - 1, splinePoints.Num());
            prevPoint = splinePoints[prevIndex];
            const int nextIndex = CircularIndex(curIndex + 1, splinePoints.Num());
            nextPoint = splinePoints[nextIndex];

            if (PointIsConvex(prevPoint, curPoint, nextPoint))
            {
                convexPoints.Add(curPoint);
                convexIndices.Add(curIndex);
            }
            else
            {
                reflexPoints.Add(curPoint);
                reflexIndices.Add(curIndex);
            }
        }
    }

    //Testing if point is an ear
    {
        for (int curIndex = 0; curIndex < convexIndices.Num(); curIndex++)
        {
            int curElement = convexIndices[curIndex];

            if (IsPointAnEar(curElement, reflexIndices, splinePoints))
            {
                earPoints.Add(splinePoints[curElement]);
                earIndices.Add(curElement);
            }
        }
    }
}

void ASplineArea::TrianglesFromPoints(TArray<FVector>& splinePoints, const TArray<int>& inConvexIndices,
                                      const TArray<int>& inReflexIndices, const TArray<
                                          int>& inEarIndices)
{
    int curPoint = 0;
    if (inEarIndices.Num() > 0)
        curPoint = inEarIndices[0];
    const int prevPoint = CircularIndex(curPoint - 1, splinePoints.Num());
    const int nextPoint = CircularIndex(curPoint + 1, splinePoints.Num());

    if (splinePoints.Num() <= 3)
    {
        MeshTriangle triangle;
        triangle.point1 = splinePoints[0];
        triangle.point2 = splinePoints[1];
        triangle.point3 = splinePoints[2];
        VisualizationTriangles.Add(triangle);
    }
    else
    {
        //Make a triangle of the ear point and its adjacent points
        MeshTriangle triangle;
        triangle.point1 = splinePoints[curPoint];
        triangle.point2 = splinePoints[prevPoint];
        triangle.point3 = splinePoints[nextPoint];

        //Remove the ear
        splinePoints.RemoveAt(curPoint);

        //Recalcualte the polygon components with the ear missing for an optimisation a linked list data structure can be used instead of recalculating at every step
        TArray<FVector> reflexPoints = TArray<FVector>{};
        TArray<FVector> convexPoints = TArray<FVector>{};
        TArray<FVector> earPoints = TArray<FVector>{};
        TArray<int> reflexIndices = TArray<int>{};
        TArray<int> convexIndices = TArray<int>{};
        TArray<int> earIndices = TArray<int>{};
        GetPolygonComponents(splinePoints, reflexPoints, convexPoints, earPoints, reflexIndices, convexIndices,
                             earIndices);
        TrianglesFromPoints(splinePoints, convexIndices, reflexIndices, earIndices);
        VisualizationTriangles.Add(triangle);
    }
}

TArray<FVector> ASplineArea::GetSplinePoints() const
{
    TArray<FVector> splinePoints;
    for (int i = 0; i < pSpline->GetNumberOfSplinePoints(); i++)
    {
        splinePoints.Add(pSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local));
    }
    return splinePoints;
}

void ASplineArea::TrianglesToIndices(const TArray<MeshTriangle>& triangles, TArray<FVector>& vertices,
                                     TArray<int>& indices) const
{
    for (int i = 0; i < triangles.Num(); i++)
    {
        int newIndex = vertices.AddUnique(triangles[i].point1);
        if (newIndex < 0)
            indices.Add(newIndex);
        else
            indices.Add(vertices.Find(triangles[i].point1));

        vertices.AddUnique(triangles[i].point2);
        if (newIndex < 0)
            indices.Add(newIndex);
        else
            indices.Add(vertices.Find(triangles[i].point2));

        vertices.AddUnique(triangles[i].point3);
        if (newIndex < 0)
            indices.Add(newIndex);
        else
            indices.Add(vertices.Find(triangles[i].point3));
    }
}

void ASplineArea::CreateAreaMesh() const
{
    TArray<FVector> vertices;
    TArray<int> indices;
    TrianglesToIndices(VisualizationTriangles, vertices, indices);

    TArray<FVector> normals;
    for (int i = 0; i < vertices.Num(); i++)
    {
        normals.Add(FVector(0, 0, 1));
    }
    TArray<FVector2D> uv;
    for (int i = 0; i < vertices.Num(); i++)
    {
        uv.Add(FVector2D(0, 0));
    }
    TArray<FProcMeshTangent> tangents;
    for (int i = 0; i < vertices.Num(); i++)
    {
        tangents.Add(FProcMeshTangent(0, 0, 1));
    }
    TArray<FColor> vertexColors;
    for (int i = 0; i < vertices.Num(); i++)
    {
        vertexColors.Add(FColor(0.75, 0.75, 0.75, 1.0));
    }

    pAreaMesh->ClearMeshSection(0);
    pAreaMesh->CreateMeshSection(0, vertices, indices, normals, uv, vertexColors, tangents, true);
    pAreaMesh->SetMaterial(0, pAreaMeshMaterial);
}

void ASplineArea::CreateAreaOutline() const
{
    if (bEnableOutline == false)
    {
        pAreaOutline->ClearInstances();
        pAreaOutline->SetVisibility(false);
        return;
    }

    pAreaOutline->ClearInstances();
    pAreaOutline->SetVisibility(true);

    const int instanceCount = pSpline->GetNumberOfSplinePoints();
    for (int i = 0; i < instanceCount; i++)
    {
        FVector firstPoint = pSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
        FVector secondPoint = pSpline->GetLocationAtSplinePoint(
            CircularIndex(i + 1, pSpline->GetNumberOfSplinePoints()), ESplineCoordinateSpace::World);
        const FRotator rotationToPoint = UKismetMathLibrary::FindLookAtRotation(firstPoint, secondPoint);
        const float length = FVector::Dist(firstPoint, secondPoint);

        FTransform newTransform;
        newTransform.SetLocation(FMath::Lerp(firstPoint, secondPoint, 0.5f));
        newTransform.SetRotation(FQuat(FRotator(rotationToPoint.Pitch, rotationToPoint.Yaw, 1.f)));
        newTransform.SetScale3D(FVector(length * 0.005f, OutlineWidth, 1.f));

        pAreaOutline->AddInstanceWorldSpace(newTransform);
    }
}

void ASplineArea::SetAreaActive(bool newState) const
{
    if (pAreaMesh == nullptr)
        return;
    if (pAreaOutline == nullptr)
        return;

    if (newState)
    {
        pAreaMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        pAreaMesh->SetVisibility(true);
        pAreaOutline->SetVisibility(true);
    }
    else
    {
        pAreaMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        pAreaOutline->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        pAreaMesh->SetVisibility(false);
        pAreaOutline->SetVisibility(false);
    }
}
