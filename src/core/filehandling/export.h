#pragma once
#include <game/rtech/cpakfile.h>

// mdl.cpp
void HandleModelLoad(std::vector<std::string> filePaths);

// rpak.cpp
FORCEINLINE void HandleExportBindingForAsset(CAsset* const asset, const bool exportDependencies);
void HandlePakAssetExportList(std::deque<CAsset*> selectedAssets, const bool exportDependencies);
void HandleExportAllPakAssets(std::vector<CGlobalAssetData::AssetLookup_t>* const pakAssets, const bool exportDependencies);
void HandleExportSelectedAssetType(std::vector<CGlobalAssetData::AssetLookup_t> pakAssets, const bool exportDependencies);

// list.cpp
void HandleListExportPakAssets(const HWND handle, std::vector<CGlobalAssetData::AssetLookup_t>* assets);
void HandleListExport(const HWND handle, std::vector<std::string> listElements);