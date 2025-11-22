#include "pch.h"
#include "ParticleModuleRequired.h"
#include "ParticleData.h"

// payload size를 고정된 값으로 지정
UParticleModuleRequired::UParticleModuleRequired() :
    UParticleModule(REQUIRED_MODULE_PAYLOAD_SIZE)
{}

void UParticleModuleRequired::Spawn(FBaseParticle* Particle, float EmitterTime)
{
    if (!Particle) return;

    // 파티클의 기본 크기 설정 (테스트용으로 고정값 사용)
    Particle->Size = FVector(10.0f, 10.0f, 1.0f);
    Particle->BaseSize = Particle->Size;

    // 파티클의 기본 색상 설정 (테스트용으로 밝은 노란색)
    Particle->Color = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);
    Particle->BaseColor = Particle->Color;

    // 회전 초기화
    Particle->Rotation = 0.0f;
    Particle->RotationRate = 0.0f;
    Particle->BaseRotationRate = 0.0f;
}

void UParticleModuleRequired::Update(FBaseParticle* Particle, float DeltaTime)
{
    if (!Particle) return;

    // RelativeTime 업데이트
    Particle->RelativeTime += DeltaTime;

    // 위치 업데이트 (속도 기반)
    Particle->OldLocation = Particle->Location;
    Particle->Location += Particle->Velocity * DeltaTime;

    // 회전 업데이트
    Particle->Rotation += Particle->RotationRate * DeltaTime;
}