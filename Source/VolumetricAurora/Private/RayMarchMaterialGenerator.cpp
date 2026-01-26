// Copyright (c) 2026 R&B. All rights reserved.

// RayMarchMaterialGenerator.cpp

#include "RayMarchMaterialGenerator.h"

#include "AssetRegistry/AssetRegistryModule.h"

// headers below are only exist in editor build
#if WITH_EDITOR
// UMaterial Class
#include "Materials/Material.h"
// Custom Node Class
#include "Materials/MaterialExpressionCustom.h"
// UV Coordinate Node Class
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
// Texture Sample Parameter Node
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionAppendVector.h"

// CameraPositionWS 노드
#include "Materials/MaterialExpressionCameraPositionWS.h"
// WorldPosition 노드
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionObjectPositionWS.h"
#include "Materials/MaterialExpressionObjectBounds.h"

#include "Factories/MaterialFactoryNew.h"
// Asset Registration System
#include "AssetRegistry/AssetRegistryModule.h"
// Save Asset
#include "UObject/Package.h"
// Save Asset
#include "UObject/SavePackage.h"
#endif

UMaterial* URayMarchMaterialGenerator::GenerateRayMarchMaterial()
{
#if WITH_EDITOR
	// 1. link package path
	// virtual full path = /VolumetricAurora/Materials/M_RayMarchVolume
	// real full path = Plugins/VolumetricAurora/Content/Materials/M_RayMarchVolume.uasset
	FString PackagePath = TEXT("/VolumetricAurora/Materials/");
	FString MaterialName = TEXT("M_RayMarchVolume");
	FString FullPath = PackagePath + MaterialName;

	// 2. Create Package
	UPackage* Package = CreatePackage(*FullPath);
	Package->FullyLoad();

	// 3. Create Material
	UMaterial* Material = NewObject<UMaterial>(
		Package,						// Outer : Package that will have the Material
		*MaterialName,					// Name : Asset Name
		RF_Public | RF_Standalone		// Flags : Can be referenced from other package / Savable
	);

	// 4. Set Material Basic Option
	Material->MaterialDomain = EMaterialDomain::MD_Surface;
	Material->BlendMode =EBlendMode::BLEND_Translucent;
	Material->SetShadingModel(EMaterialShadingModel::MSM_Unlit);

	// 5. Node Location
	int32 NodePosX = -400;
	int32 NodePosY = 0;

	// 6. Texture Object Parameter (Custom Node에서 직접 샘플링용)
	UMaterialExpressionTextureObjectParameter* TextureNode =
		NewObject<UMaterialExpressionTextureObjectParameter>(Material);
	TextureNode->ParameterName = TEXT("VolumeTexture");
	TextureNode->MaterialExpressionEditorX = NodePosX - 600;
	TextureNode->MaterialExpressionEditorY = NodePosY;
	Material->GetExpressionCollection().AddExpression(TextureNode);

	// 7. Camera Position 노드
	UMaterialExpressionCameraPositionWS* CameraPosNode =
		NewObject<UMaterialExpressionCameraPositionWS>(Material);
	CameraPosNode->MaterialExpressionEditorX = NodePosX - 600;
	CameraPosNode->MaterialExpressionEditorY = NodePosY + 300;
	Material->GetExpressionCollection().AddExpression(CameraPosNode);

	// 8. World Position 노드
	UMaterialExpressionWorldPosition* WorldPosNode =
		NewObject<UMaterialExpressionWorldPosition>(Material);
	WorldPosNode->MaterialExpressionEditorX = NodePosX - 600;
	WorldPosNode->MaterialExpressionEditorY = NodePosY + 600;
	Material->GetExpressionCollection().AddExpression(WorldPosNode);

	// ========================================
	// 9. Object Position WS 노드 (오브젝트 중심)
	// ========================================
	UMaterialExpressionObjectPositionWS* ObjectPosNode =
		NewObject<UMaterialExpressionObjectPositionWS>(Material);
	ObjectPosNode->MaterialExpressionEditorX = NodePosX - 600;
	ObjectPosNode->MaterialExpressionEditorY = NodePosY + 900;
	Material->GetExpressionCollection().AddExpression(ObjectPosNode);

	// ========================================
	// 10. Object Radius 노드 (오브젝트 반경)
	// ========================================
	UMaterialExpressionObjectBounds* ObjectBoundsNode = NewObject<UMaterialExpressionObjectBounds>(Material);
	ObjectBoundsNode->MaterialExpressionEditorX = NodePosX;
	ObjectBoundsNode->MaterialExpressionEditorY = NodePosY + 1200;
	Material->GetExpressionCollection().AddExpression(ObjectBoundsNode);
	
	// 11. Custom Node (Ray Marching HLSL)
	UMaterialExpressionCustom* CustomNode =
		NewObject<UMaterialExpressionCustom>(Material);
	CustomNode->MaterialExpressionEditorX = NodePosX - 600;
	CustomNode->MaterialExpressionEditorY = NodePosY + 1500;
	CustomNode->OutputType = CMOT_Float4;	// RGBA 출력
	CustomNode->Code = GetRayMarchHLSLCode();
	CustomNode->Description = TEXT("RayMarch");
	CustomNode->IncludeFilePaths.Add(TEXT("/VolumetricAuroraShaders/Private/Noise.ush"));

	// Custom Node 입력 핀 설정
	CustomNode->Inputs.Empty();

	// 입력 1: 텍스처 오브젝트
	FCustomInput TextureInput;
	TextureInput.InputName = TEXT("VolumeTexture");
	TextureInput.Input.Expression = TextureNode;
	CustomNode->Inputs.Add(TextureInput);

	// 입력 2: 카메라 위치
	FCustomInput CameraPosInput;
	CameraPosInput.InputName = TEXT("CameraPos");
	CameraPosInput.Input.Expression = CameraPosNode;
	CustomNode->Inputs.Add(CameraPosInput);

	// 입력 3: 월드 위치
	FCustomInput WorldPosInput;
	WorldPosInput.InputName = TEXT("WorldPos");
	WorldPosInput.Input.Expression = WorldPosNode;
	CustomNode->Inputs.Add(WorldPosInput);

	// 입력 4: 오브젝트 중심 위치
	FCustomInput ObjectPosInput;
	ObjectPosInput.InputName = TEXT("ObjectPos");
	ObjectPosInput.Input.Expression = ObjectPosNode;
	CustomNode->Inputs.Add(ObjectPosInput);

	// 입력 5: 오브젝트 Half Extent (각 축별 반경)
	FCustomInput ObjectExtentInput;
	ObjectExtentInput.InputName = TEXT("ObjectExtent");
	ObjectExtentInput.Input.Expression = ObjectBoundsNode;
	CustomNode->Inputs.Add(ObjectExtentInput);

	Material->GetExpressionCollection().AddExpression(CustomNode);

	// 12. Link Material Output
	Material->GetEditorOnlyData()->EmissiveColor.Expression = CustomNode;
	Material->GetEditorOnlyData()->EmissiveColor.OutputIndex = 0;	// RGB

	Material->GetEditorOnlyData()->Opacity.Expression = CustomNode;
	Material->GetEditorOnlyData()->Opacity.OutputIndex = 0;			// 임시로 같은 출력(추후 수정)

	// 13. Compile and Save
	// 변경 시작 알림
	Material->PreEditChange(nullptr);
	// 셰이더 컴파일 트리거
	Material->PostEditChange();

	// 에셋 레지스트리에 등록 (콘텐츠 브라우저에 표시)
	FAssetRegistryModule::AssetCreated(Material);
	// 저장 필요 표시
	Package->MarkPackageDirty();

	// Save
	FString PackageFileName = FPackageName::LongPackageNameToFilename(FullPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Material, *PackageFileName, SaveArgs);

	UE_LOG(LogTemp, Log, TEXT("RayMarch Material Created : %s"), *FullPath);

	return Material;
#else
	// return nullptr when runtime
	return nullptr;
#endif
}

FString URayMarchMaterialGenerator::GetRayMarchHLSLCode()
{
    return TEXT(R"(
        // ==========================================
        // 입력 변수 (Custom Node 입력 핀에서 받음)
        // ==========================================
        // CameraPos    - 카메라 월드 위치 (float3)
        // WorldPos     - 현재 픽셀의 월드 위치 (float3)
        // ObjectPos    - 오브젝트 중심 월드 위치 (float3)
        // ObjectRadius - 오브젝트 바운딩 구 반경 (float)
        // VolumeTexture - 샘플링할 2D 텍스처

        // ==========================================
        // Bounds 계산
        // ==========================================
        // ObjectExtent = 각 축별 Half Extent (스케일 반영됨)
        // 이 값들은 6개 평면의 위치를 결정:
        //   X = BoundsMin.x (좌측), X = BoundsMax.x (우측)
        //   Y = BoundsMin.y (하단), Y = BoundsMax.y (상단)
        //   Z = BoundsMin.z (전면), Z = BoundsMax.z (후면)
		// 예: 스케일 (10, 10, 1)인 큐브 → ObjectExtent ≈ (500, 500, 50)
		float3 BoundsMin = ObjectPos - ObjectExtent;
		float3 BoundsMax = ObjectPos + ObjectExtent;
        
        // ==========================================
        // Ray 설정
        // ==========================================
        // Ray 시작점 = 카메라 위치
        // Ray 방향 = 카메라에서 현재 픽셀을 향하는 단위 벡터
        // (GPU가 픽셀마다 병렬로 실행하므로 각 픽셀마다 다른 Ray)
        float3 RayOrigin = CameraPos;
        float3 RayDir = normalize(WorldPos - CameraPos);

        // ==========================================
        // Ray-Box 교차 계산 (Slab Method)
        // ==========================================
        // 목표: Ray가 박스에 들어가는 t값(tEntry)과 나오는 t값(tExit) 계산
        // Ray 위의 점 = RayOrigin + RayDir * t

        // 나누기를 곱하기로 변환 (성능 최적화)
        float3 InvRayDir = 1.0 / RayDir;

        // 각 축의 두 평면과 Ray가 교차하는 t 값 계산
        // 예: T1.x = Ray가 X=BoundsMin.x 평면과 만나는 t
        //     T2.x = Ray가 X=BoundsMax.x 평면과 만나는 t
        float3 T1 = (BoundsMin - RayOrigin) * InvRayDir;
        float3 T2 = (BoundsMax - RayOrigin) * InvRayDir;
        
        // Ray 방향에 따라 T1 > T2일 수 있으므로 정렬
        // TMin = 각 축에서 먼저 만나는 평면의 t
        // TMax = 각 축에서 나중에 만나는 평면의 t
        float3 TMin = min(T1, T2);
        float3 TMax = max(T1, T2);
        
        // 세 축 구간의 교집합 계산
        // "박스 안" = X범위 안 AND Y범위 안 AND Z범위 안
        // tEntry = 세 축 모두 박스 안에 들어간 시점 (가장 늦게 들어간 축 기준)
        // tExit = 하나라도 박스 밖으로 나간 시점 (가장 먼저 나간 축 기준)
        float tEntry = max(max(TMin.x, TMin.y), TMin.z);
        float tExit = min(min(TMax.x, TMax.y), TMax.z);
        
        // 교차 실패 조건:
        // 1. tEntry > tExit: 교집합 없음 (Ray가 박스를 빗나감)
        // 2. tExit < 0: 박스가 카메라 뒤에 있음
        if (tEntry > tExit || tExit < 0)
        {
            return float4(0, 0, 0, 0);
        }
        
        // 카메라가 박스 안에 있으면 tEntry < 0
        // Ray는 t=0부터 시작하므로 0으로 보정
        tEntry = max(tEntry, 0);

        // ==========================================
        // Ray Marching 설정
        // ==========================================
        // MaxSteps: Ray를 따라 샘플링할 횟수 (높을수록 품질↑ 성능↓)
        // StepSize: 각 샘플링 간격 (박스 통과 거리 / 샘플 수)
        int MaxSteps = 64;
        float StepSize = (tExit - tEntry) / MaxSteps;
        
        // 누적 색상과 알파 (Front-to-Back 블렌딩용)
        float3 AccumulatedColor = float3(0, 0, 0);
        float AccumulatedAlpha = 0;

        // ==========================================
        // Ray Marching 루프
        // ==========================================
        // Ray를 따라 일정 간격으로 이동하며 각 위치에서 텍스처 샘플링
        for (int i = 0; i < MaxSteps; i++)
        {
            // 현재 샘플링 위치 계산
            // t = tEntry에서 시작, StepSize씩 증가
            float t = tEntry + StepSize * i;
            float3 CurrentPos = RayOrigin + RayDir * t;
            
            // ==========================================
            // UVW 좌표 계산
            // ==========================================
            // 월드 좌표를 0~1 범위로 정규화
            // BoundsMin → (0,0,0), BoundsMax → (1,1,1)
            float3 UVW = (CurrentPos - BoundsMin) / (BoundsMax - BoundsMin);
            
            // Z 범위 체크 (Bounds 밖이면 스킵)
            // 부동소수점 오차로 약간 벗어날 수 있으므로 체크
            if (UVW.z < 0 || UVW.z > 1)
            {
                continue;
            }
            
            // ==========================================
            // 2D 텍스처 샘플링
            // ==========================================
            // X, Y 좌표만 사용하여 2D 텍스처에서 값 추출
            // saturate: 0~1 범위로 클램핑 (부동소수점 오차 방지)
            // SampleLevel: 밉맵 레벨 0에서 샘플링 (LOD 고정)
            float2 UV = saturate(UVW.xy);
            float4 TexSample = VolumeTexture.SampleLevel(VolumeTextureSampler, UV, 0);
            float Value = TexSample.r;  // R 채널 사용
            
            // ==========================================
            // 음수/양수 영역 판별 및 색상 누적
            // ==========================================
            // 텍스처 값 0 ~ 0.5 → 음수 취급 → 색상 칠함
            // 텍스처 값 0.5 ~ 1 → 양수 취급 → 투명 (아무것도 안 함)
            if (Value < 0.5)
            {
                // Density: 값이 0에 가까울수록 진하게 (0~1 범위)
                // Value=0 → Density=1, Value=0.5 → Density=0
                float Density = (0.5 - Value) * 2.0;
                float3 Color = float3(0.2, 0.8, 0.4);  // 녹색 계열
                
                // Front-to-Back 알파 블렌딩
                // 이미 누적된 불투명도만큼 새 색상 기여도 감소
                AccumulatedColor += Color * Density * (1 - AccumulatedAlpha);
                AccumulatedAlpha += Density * (1 - AccumulatedAlpha);
                
                // Early Exit: 거의 불투명해지면 더 이상 샘플링 불필요
                if (AccumulatedAlpha > 0.95) break;
            }
        }

        // 최종 색상과 알파 반환
        return float4(AccumulatedColor, AccumulatedAlpha);
    )");
}