#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldEnvironmentManager.generated.h"

class ADirectionalLight;
class ASkyLight;
class ASkyAtmosphere;
class AExponentialHeightFog;
class APostProcessVolume;

UENUM(BlueprintType)
enum class EWeatherType : uint8
{
    Clear   UMETA(DisplayName = "Clear"),
    Cloudy  UMETA(DisplayName = "Cloudy"),
    Rain    UMETA(DisplayName = "Rain"),
    Storm   UMETA(DisplayName = "Storm"),
    Fog     UMETA(DisplayName = "Fog"),
    Snow    UMETA(DisplayName = "Snow")
};

UCLASS(Blueprintable)
class FIGHTINGGAME_API AWorldEnvironmentManager : public AActor
{
    GENERATED_BODY()

public:

    AWorldEnvironmentManager();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;


    // TIME OF DAY


    UFUNCTION(BlueprintCallable, Category = "Time Of Day")
    void SetTimeOfDay(float NewTime);

    UFUNCTION(BlueprintCallable, Category = "Time Of Day")
    void SetTimeOfDayNormalized(float NormalizedTime);

    UFUNCTION(BlueprintPure, Category = "Time Of Day")
    float GetTimeOfDay() const;

    UFUNCTION(BlueprintCallable, Category = "Time Of Day")
    void SetTimeOfDayPaused(bool bPaused);


    // WEATHER


    UFUNCTION(BlueprintCallable, Category = "Weather")
    void SetWeather(EWeatherType NewWeather);

    UFUNCTION(BlueprintPure, Category = "Weather")
    EWeatherType GetWeather() const;

protected:


    // REFERENCES


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "References")
    TObjectPtr<ADirectionalLight> SunLight;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "References")
    TObjectPtr<ADirectionalLight> MoonLight;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "References")
    TObjectPtr<ASkyLight> SkyLight;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "References")
    TObjectPtr<ASkyAtmosphere> SkyAtmosphere;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "References")
    TObjectPtr<AExponentialHeightFog> HeightFog;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "References")
    TObjectPtr<APostProcessVolume> PostProcessVolume;


    // TIME OF DAY SETTINGS


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time Of Day",
        meta = (ClampMin = "0.0", ClampMax = "24.0"))
    float TimeOfDay = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time Of Day",
        meta = (ClampMin = "0.1"))
    float DayLengthInMinutes = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time Of Day")
    bool bRunTimeOfDay = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time Of Day")
    bool bPaused = false;


    // SUN SETTINGS


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun")
    float SunYaw = -30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun")
    float MaxSunIntensity = 100000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinSunIntensityAtNight = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun",
        meta = (ClampMin = "0", ClampMax = "2"))
    int32 SunForwardShadingPriority = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun")
    FLinearColor SunDayColor = FLinearColor(1.0f, 0.93f, 0.82f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun")
    FLinearColor SunSunsetColor = FLinearColor(1.0f, 0.45f, 0.20f);


    // MOON SETTINGS
 

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moon")
    float MoonYaw = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moon")
    float MaxMoonIntensity = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moon",
        meta = (ClampMin = "0", ClampMax = "2"))
    int32 MoonForwardShadingPriority = 0;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moon")
    FLinearColor MoonColor = FLinearColor(0.45f, 0.55f, 1.0f);


    // SKY LIGHT


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sky")
    bool bRecaptureSkyLight = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sky",
        meta = (ClampMin = "0.1"))
    float SkyRecaptureInterval = 0.5f;


    // WEATHER


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
    EWeatherType CurrentWeather = EWeatherType::Clear;

private:

    void UpdateTime(float DeltaTime);
    void UpdateSunAndMoon();
    void UpdateSkyLight();
    void UpdateWeather();

    float CalculateSunElevation() const;
    float CalculateDaylightAlpha() const;

private:

    float SkyRecaptureTimer = 0.0f;
};