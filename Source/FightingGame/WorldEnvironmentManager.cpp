#include "WorldEnvironmentManager.h"

#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/PostProcessVolume.h"

AWorldEnvironmentManager::AWorldEnvironmentManager()
{
    PrimaryActorTick.bCanEverTick = true;

    PrimaryActorTick.bStartWithTickEnabled = true;
}

void AWorldEnvironmentManager::BeginPlay()
{
    Super::BeginPlay();

    TimeOfDay = FMath::Clamp(TimeOfDay, 0.0f, 24.0f);

    if (SunLight)
    {
        if (UDirectionalLightComponent* SunComponent =
            Cast<UDirectionalLightComponent>(SunLight->GetLightComponent()))
        {
            SunComponent->ForwardShadingPriority = SunForwardShadingPriority;
        }
    }

    if (MoonLight)
    {
        if (UDirectionalLightComponent* MoonComponent =
            Cast<UDirectionalLightComponent>(MoonLight->GetLightComponent()))
        {
            MoonComponent->ForwardShadingPriority = MoonForwardShadingPriority;
        }
    }

    UpdateSunAndMoon();
    UpdateWeather();
}

void AWorldEnvironmentManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bPaused)
    {
        UpdateTime(DeltaTime);
    }

    UpdateSunAndMoon();
    UpdateSkyLight();
    UpdateWeather();
}

// TIME OF DAY


void AWorldEnvironmentManager::UpdateTime(float DeltaTime)
{
    if (!bRunTimeOfDay)
    {
        return;
    }

    if (DayLengthInMinutes <= 0.0f)
    {
        return;
    }

    // Convert real seconds into game hours.

    const float GameHoursPerSecond =
        24.0f / (DayLengthInMinutes * 60.0f);

    TimeOfDay += GameHoursPerSecond * DeltaTime;

    if (TimeOfDay >= 24.0f)
    {
        TimeOfDay -= 24.0f;
    }
}

void AWorldEnvironmentManager::SetTimeOfDay(float NewTime)
{
    TimeOfDay = FMath::Fmod(NewTime, 24.0f);

    if (TimeOfDay < 0.0f)
    {
        TimeOfDay += 24.0f;
    }

    UpdateSunAndMoon();
}

void AWorldEnvironmentManager::SetTimeOfDayNormalized(float NormalizedTime)
{
    NormalizedTime = FMath::Clamp(NormalizedTime, 0.0f, 1.0f);

    SetTimeOfDay(NormalizedTime * 24.0f);
}

float AWorldEnvironmentManager::GetTimeOfDay() const
{
    return TimeOfDay;
}

void AWorldEnvironmentManager::SetTimeOfDayPaused(bool bNewPaused)
{
    bPaused = bNewPaused;
}


// SUN + MOON


float AWorldEnvironmentManager::CalculateSunElevation() const
{
    /*
     * 06:00  = horizon
     * 12:00  = highest point
     * 18:00  = horizon
     *
     * Positive values mean the sun is above the horizon.
     */

    const float SolarTime = TimeOfDay - 6.0f;

    const float Angle =
        (SolarTime / 12.0f) * PI;

    return FMath::Sin(Angle);
}

float AWorldEnvironmentManager::CalculateDaylightAlpha() const
{
    const float Elevation = CalculateSunElevation();

    /*
     * Smooth transition around sunrise/sunset.
     *
     * Slightly extended transition gives us a softer
     * cinematic dawn/dusk instead of an abrupt cutoff.
     */

    return FMath::Clamp(
        FMath::SmoothStep(0.0f, 0.20f, Elevation),
        0.0f,
        1.0f
    );
}

void AWorldEnvironmentManager::UpdateSunAndMoon()
{
    if (!SunLight && !MoonLight)
    {
        return;
    }


    // SUN ROTATION


    if (SunLight)
    {
        /*
         * 06:00 -> Pitch 0
         * 12:00 -> Pitch -90
         * 18:00 -> Pitch 0
         * 00:00 -> Pitch +90
         */

        const float SolarAngle =
            ((TimeOfDay - 6.0f) / 24.0f) * 360.0f;

        const float SunPitch =
            -90.0f * FMath::Sin(FMath::DegreesToRadians(SolarAngle));

        FRotator SunRotation(
            SunPitch,
            SunYaw,
            0.0f
        );

        SunLight->SetActorRotation(SunRotation);

        UDirectionalLightComponent* SunComponent =
            Cast<UDirectionalLightComponent>(SunLight->GetLightComponent());

        if (SunComponent)
        {
            const float Elevation =
                CalculateSunElevation();

            const float DayAlpha =
                CalculateDaylightAlpha();

            /*
             * Base solar intensity now follows the same
             * smoothstep curve used for the moon's night fade,
             * instead of a separate, more aggressive pow curve.
             */

            float Intensity =
                MaxSunIntensity * DayAlpha;

            /*
             * Optional floor for stylistic control; 0 by default
             * since the Moon now handles night illumination.
             */

            Intensity =
                FMath::Max(
                    Intensity,
                    MaxSunIntensity * MinSunIntensityAtNight
                );

            SunComponent->SetIntensity(Intensity);


            // SUN COLOR

            const float SunsetFactor =
                1.0f -
                FMath::Clamp(
                    Elevation,
                    0.0f,
                    1.0f
                );

            /*
             * Only make the sun strongly orange near
             * sunrise/sunset.
             */

            const float WarmFactor =
                FMath::SmoothStep(
                    0.0f,
                    0.45f,
                    SunsetFactor
                );

            const FLinearColor SunColor =
                FLinearColor::LerpUsingHSV(
                    SunDayColor,
                    SunSunsetColor,
                    WarmFactor
                );

            SunComponent->SetLightColor(
                SunColor
            );
        }
    }


    // MOON


    if (MoonLight)
    {
        const float SolarAngle =
            ((TimeOfDay - 6.0f) / 24.0f) * 360.0f;

        const float SunPitch =
            -90.0f *
            FMath::Sin(
                FMath::DegreesToRadians(SolarAngle)
            );

        const float MoonPitch =
            -SunPitch;

        const FRotator MoonRotation(
            MoonPitch,
            MoonYaw,
            0.0f
        );

        MoonLight->SetActorRotation(
            MoonRotation
        );

        UDirectionalLightComponent* MoonComponent =
            Cast<UDirectionalLightComponent>(MoonLight->GetLightComponent());

        if (MoonComponent)
        {
            const float DayAlpha =
                CalculateDaylightAlpha();

            const float NightAlpha =
                1.0f - DayAlpha;

            /*
             * Moonlight fades in as sunlight fades out.
             */

            const float MoonIntensity =
                MaxMoonIntensity *
                FMath::SmoothStep(
                    0.0f,
                    1.0f,
                    NightAlpha
                );

            MoonComponent->SetIntensity(
                MoonIntensity
            );

            MoonComponent->SetLightColor(
                MoonColor
            );
        }
    }
}

// SKY LIGHT


void AWorldEnvironmentManager::UpdateSkyLight()
{
    if (!bRecaptureSkyLight)
    {
        return;
    }

    if (!SkyLight)
    {
        return;
    }

    SkyRecaptureTimer += GetWorld()->GetDeltaSeconds();

    if (SkyRecaptureTimer < SkyRecaptureInterval)
    {
        return;
    }

    SkyRecaptureTimer = 0.0f;

    USkyLightComponent* SkyComponent =
        SkyLight->GetLightComponent();

    if (SkyComponent)
    {
        SkyComponent->RecaptureSky();
    }
}


// WEATHER


void AWorldEnvironmentManager::SetWeather(
    EWeatherType NewWeather)
{
    CurrentWeather = NewWeather;

    UpdateWeather();
}

EWeatherType AWorldEnvironmentManager::GetWeather() const
{
    return CurrentWeather;
}

void AWorldEnvironmentManager::UpdateWeather()
{

}