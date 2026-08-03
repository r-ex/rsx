#include <pch.h>
#include <core/utils/exportsettings.h>
#include <game/rtech/utils/bsp/bspflags.h>

RSXSettings_t g_rsxSettings{ .exportNormalRecalcSetting = eNormalExportRecalc::NML_RECALC_NONE, .exportTextureNameSetting = eTextureExportName::TXTR_NAME_TEXT,
	.exportMaterialTextures = true, .exportPathsFull = false, .exportAssetDeps = false, .disableCachedNames = false, .previewedSkinIndex = 0,
	.qcMajorVersion = 49, .qcMinorVersion = 0, .exportRigSequences = true, .exportModelSkin = false, .exportModelMatsTruncated = false,
	.exportQCIFiles = false, .useOrigScriptExportExtensions = false, .exportPhysicsContentsFilter = static_cast<uint32_t>(TRACE_MASK_ALL), .bridgePort = 3033,
	.exportDirectory = "",
};

void RSXSettings_t::SetFromCLI(const CCommandLine* cli)
{
	if (const char* const nmlRecalc = cli->GetParamValue("--nmlrecalc"))
	{
		if (!_stricmp(nmlRecalc, "none"))
			this->exportNormalRecalcSetting = NML_RECALC_NONE;
		else if (!_stricmp(nmlRecalc, "directx"))
			this->exportNormalRecalcSetting = NML_RECALC_DX;
		else if (!_stricmp(nmlRecalc, "opengl"))
			this->exportNormalRecalcSetting = NML_RECALC_OGL;
	}

	if (const char* const texNames = cli->GetParamValue("--texturenames"))
	{
		if (!_stricmp(texNames, "guid"))
			this->exportTextureNameSetting = TXTR_NAME_GUID;
		else if (!_stricmp(texNames, "stored"))
			this->exportTextureNameSetting = TXTR_NAME_REAL;
		else if (!_stricmp(texNames, "text"))
			this->exportTextureNameSetting = TXTR_NAME_TEXT;
		else if (!_stricmp(texNames, "semantic"))
			this->exportTextureNameSetting = TXTR_NAME_SMTC;
	}

	this->exportMaterialTextures = cli->HasParam("-matltextures");
	this->exportPathsFull = cli->HasParam("-exportfullpaths");
	this->exportAssetDeps = cli->HasParam("-exportdependencies");
	this->disableCachedNames = cli->HasParam("-nocachedb");

	if (const char* const qcMajorStr = cli->GetParamValue("--qcmajor"))
		this->qcMajorVersion = static_cast<uint16_t>(atoi(qcMajorStr));

	if (const char* const qcMinorStr = cli->GetParamValue("--qcminor"))
		this->qcMinorVersion = static_cast<uint16_t>(atoi(qcMinorStr));

	this->exportRigSequences = cli->HasParam("-exportrigsequences");
	this->exportModelSkin = false; // todo: maybe make an option to replace this for exporting all skins, since skins cant be picked on CLI
	this->exportModelMatsTruncated = cli->HasParam("-truncatemodelmats");
	this->exportQCIFiles = cli->HasParam("-useqci");

	// i'm not too happy with this being "--exportdir", so this may change at some point
	if (const char* const exportPath = cli->GetParamValue("--exportdir"))
		this->exportDirectory = exportPath;
}