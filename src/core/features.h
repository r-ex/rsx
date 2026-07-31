#pragma once

#define BEGIN_FEATURE_STRING() static const std::string FEATURE_STRING = {
#define FEATURE_STR_ENTRY(v) std::string((v) ? (__COUNTER__ != 0 ? ";" : "") + std::string(#v) : "")
#define END_FEATURE_STRING() }

// [ASSET FEATURES]
#define HAS_ODL_ASSET true
#define HAS_QC true
#define HAS_BSP_SUPPORT false

// [GENERAL FEATURES]
#define ADVANCED_MODEL_PREVIEW false
#define HAS_BONED_MODELS true

// [DEBUG FEATURES]
//#define DEBUG_NO_ASEQ_POSTLOAD // - DEBUG ONLY - disables (very) slow postloading for animseq assets
//#define DEBUG_IMGUI_DEMO       // - DEBUG ONLY - compiles in a call to ImGui::ShowDemoWindow
//#define DEBUG_LOAD_SHADERS_DISK

BEGIN_FEATURE_STRING()
	FEATURE_STR_ENTRY(HAS_ODL_ASSET) +
	FEATURE_STR_ENTRY(HAS_QC) +
	FEATURE_STR_ENTRY(HAS_BSP_SUPPORT) +
	FEATURE_STR_ENTRY(ADVANCED_MODEL_PREVIEW) +
	FEATURE_STR_ENTRY(HAS_BONED_MODELS)
END_FEATURE_STRING();