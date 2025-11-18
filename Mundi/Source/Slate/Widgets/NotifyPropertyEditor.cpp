#include "pch.h"
#include "NotifyPropertyEditor.h"
#include "Source/Runtime/Engine/Animation/AnimNotify/AnimNotify.h"
#include "Source/Runtime/Engine/Animation/AnimNotify/SoundAnimNotify.h"
#include "Source/Runtime/Engine/Animation/AnimNotify/CameraShakeAnimNotify.h"
#include "ThirdParty/imgui/imgui.h"

FNotifyPropertyEditor::FNotifyPropertyEditor()
	: OnDelete(nullptr)
{
}

FNotifyPropertyEditor::~FNotifyPropertyEditor()
{
}

FNotifyPropertyEditResult FNotifyPropertyEditor::Render(UAnimNotify*& InOutNotify, float MaxTime)
{
	FNotifyPropertyEditResult Result;

	if (!InOutNotify)
	{
		RenderEmptyState();
		return Result;
	}

	UAnimNotify* Notify = InOutNotify;

	// Common properties
	Result.bNameChanged = RenderCommonProperties(Notify, MaxTime);
	Result.bTimeChanged = Result.bNameChanged; // Time은 Common에 포함됨

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Type-specific properties
	if (dynamic_cast<USoundAnimNotify*>(Notify))
	{
		Result.bPropertiesChanged = RenderSoundNotifyProperties(Notify);
	}
	else if (dynamic_cast<UCameraShakeAnimNotify*>(Notify))
	{
		Result.bPropertiesChanged = RenderCameraShakeNotifyProperties(Notify);
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Delete button
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));

	if (ImGui::Button("Delete Notify", ImVec2(-1, 30)))
	{
		if (OnDelete)
		{
			OnDelete(Notify);
		}
		InOutNotify = nullptr;
		Result.bDeleted = true;
	}

	ImGui::PopStyleColor(3);

	return Result;
}

void FNotifyPropertyEditor::RenderEmptyState()
{
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
	ImGui::TextWrapped("Select a notify from the timeline to edit its properties.");
	ImGui::PopStyleColor();
}

void FNotifyPropertyEditor::SetOnDeleteCallback(std::function<void(UAnimNotify*)> Callback)
{
	OnDelete = Callback;
}

bool FNotifyPropertyEditor::RenderCommonProperties(UAnimNotify* Notify, float MaxTime)
{
	bool bChanged = false;

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.9f, 1.0f, 1.0f));

	// Notify Name
	FString notifyName = Notify->GetNotifyName().ToString();
	char nameBuffer[128];
	strncpy_s(nameBuffer, notifyName.c_str(), sizeof(nameBuffer) - 1);
	nameBuffer[sizeof(nameBuffer) - 1] = '\0';

	ImGui::Text("Name:");
	ImGui::SameLine(100);
	ImGui::PushItemWidth(-1);
	if (ImGui::InputText("##NotifyName", nameBuffer, sizeof(nameBuffer)))
	{
		Notify->SetNotifyName(FName(std::string(nameBuffer)));
		bChanged = true;
	}
	ImGui::PopItemWidth();

	// Time to Notify
	float timeToNotify = Notify->GetTimeToNotify();
	ImGui::Text("Time:");
	ImGui::SameLine(100);
	ImGui::PushItemWidth(-1);
	if (ImGui::DragFloat("##NotifyTime", &timeToNotify, 0.01f, 0.0f, MaxTime, "%.3f"))
	{
		Notify->SetTimeToNotify(timeToNotify);
		bChanged = true;
	}
	ImGui::PopItemWidth();

	ImGui::PopStyleColor();

	return bChanged;
}

bool FNotifyPropertyEditor::RenderSoundNotifyProperties(UAnimNotify* Notify)
{
	USoundAnimNotify* SoundNotify = dynamic_cast<USoundAnimNotify*>(Notify);
	if (!SoundNotify)
		return false;

	bool bChanged = false;

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 1.0f, 0.7f, 1.0f));
	ImGui::Text("Sound Notify");
	ImGui::PopStyleColor();

	ImGui::Spacing();

	// Volume
	float volume = SoundNotify->GetVolume();
	ImGui::Text("Volume:");
	ImGui::SameLine(100);
	ImGui::PushItemWidth(-1);
	if (ImGui::SliderFloat("##Volume", &volume, 0.0f, 1.0f, "%.2f"))
	{
		SoundNotify->SetVolume(volume);
		bChanged = true;
	}
	ImGui::PopItemWidth();

	// Pitch
	float pitch = SoundNotify->GetPitch();
	ImGui::Text("Pitch:");
	ImGui::SameLine(100);
	ImGui::PushItemWidth(-1);
	if (ImGui::SliderFloat("##Pitch", &pitch, 0.1f, 3.0f, "%.2f"))
	{
		SoundNotify->SetPitch(pitch);
		bChanged = true;
	}
	ImGui::PopItemWidth();

	// Sound (read-only for now, just display path)
	ImGui::Text("Sound:");
	ImGui::SameLine(100);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
	USound* Sound = SoundNotify->GetSound();
	if (Sound)
	{
		FString soundPath = Sound->GetFilePath();
		size_t lastSlash = soundPath.find_last_of("/\\");
		FString soundName = (lastSlash != FString::npos) ? soundPath.substr(lastSlash + 1) : soundPath;
		ImGui::TextWrapped("%s", soundName.c_str());
	}
	else
	{
		ImGui::TextWrapped("(None)");
	}
	ImGui::PopStyleColor();

	return bChanged;
}

bool FNotifyPropertyEditor::RenderCameraShakeNotifyProperties(UAnimNotify* Notify)
{
	UCameraShakeAnimNotify* ShakeNotify = dynamic_cast<UCameraShakeAnimNotify*>(Notify);
	if (!ShakeNotify)
		return false;

	bool bChanged = false;

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.5f, 1.0f));
	ImGui::Text("Camera Shake Notify");
	ImGui::PopStyleColor();

	ImGui::Spacing();

	// Duration
	float duration = ShakeNotify->GetDuration();
	ImGui::Text("Duration:");
	ImGui::SameLine(150);
	ImGui::PushItemWidth(-1);
	if (ImGui::DragFloat("##Duration", &duration, 0.01f, 0.0f, 10.0f, "%.2fs"))
	{
		ShakeNotify->SetDuration(duration);
		bChanged = true;
	}
	ImGui::PopItemWidth();

	// Amplitude Location
	float ampLoc = ShakeNotify->GetAmplitudeLocation();
	ImGui::Text("Amplitude Loc:");
	ImGui::SameLine(150);
	ImGui::PushItemWidth(-1);
	if (ImGui::DragFloat("##AmpLoc", &ampLoc, 0.5f, 0.0f, 100.0f, "%.1f"))
	{
		ShakeNotify->SetAmplitudeLocation(ampLoc);
		bChanged = true;
	}
	ImGui::PopItemWidth();

	// Amplitude Rotation
	float ampRot = ShakeNotify->GetAmplitudeRotationDeg();
	ImGui::Text("Amplitude Rot:");
	ImGui::SameLine(150);
	ImGui::PushItemWidth(-1);
	if (ImGui::DragFloat("##AmpRot", &ampRot, 0.1f, 0.0f, 45.0f, "%.1f°"))
	{
		ShakeNotify->SetAmplitudeRotationDeg(ampRot);
		bChanged = true;
	}
	ImGui::PopItemWidth();

	// Frequency
	float frequency = ShakeNotify->GetFrequency();
	ImGui::Text("Frequency:");
	ImGui::SameLine(150);
	ImGui::PushItemWidth(-1);
	if (ImGui::DragFloat("##Frequency", &frequency, 0.5f, 0.1f, 100.0f, "%.1f"))
	{
		ShakeNotify->SetFrequency(frequency);
		bChanged = true;
	}
	ImGui::PopItemWidth();

	// Priority
	int32 priority = ShakeNotify->GetPriority();
	ImGui::Text("Priority:");
	ImGui::SameLine(150);
	ImGui::PushItemWidth(-1);
	if (ImGui::DragInt("##Priority", &priority, 1, -100, 100))
	{
		ShakeNotify->SetPriority(priority);
		bChanged = true;
	}
	ImGui::PopItemWidth();

	// Mix Ratio
	float mixRatio = ShakeNotify->GetMixRatio();
	ImGui::Text("Mix Ratio:");
	ImGui::SameLine(150);
	ImGui::PushItemWidth(-1);
	if (ImGui::SliderFloat("##MixRatio", &mixRatio, 0.0f, 1.0f, "%.2f"))
	{
		ShakeNotify->SetMixRatio(mixRatio);
		bChanged = true;
	}
	ImGui::PopItemWidth();

	ImGui::Spacing();
	ImGui::Text("Noise Mode:");
	EShakeNoise noiseMode = ShakeNotify->GetNoiseMode();
	const char* noiseModeNames[] = { "Sine", "Perlin", "Mixed" };
	int currentMode = static_cast<int>(noiseMode);

	ImGui::PushItemWidth(-1);
	if (ImGui::Combo("##NoiseMode", &currentMode, noiseModeNames, IM_ARRAYSIZE(noiseModeNames)))
	{
		ShakeNotify->SetNoiseMode(static_cast<EShakeNoise>(currentMode));
		bChanged = true;
	}
	ImGui::PopItemWidth();

	return bChanged;
}
