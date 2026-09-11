#include <pch.h>
#include "miles.h"
#include "project.h"

// MPRJ Controllers
MilesController_s::MilesController_s(const char* stringTable, const MilesController_v13_s& a)
	: min(a.min), max(a.max), defaultValue(a.defaultValue), mathFuncOffset(a.mathFuncOffset),
	type(a.type)
{
	this->name = stringTable + a.nameOffset;
}

MilesController_s::MilesController_s(const char* stringTable, const MilesController_v46_s& a)
	: min(a.min), max(a.max), defaultValue(a.defaultValue), mathFuncOffset(-1),
	type(a.type) // not sure where this var is yet
{
	this->name = stringTable + a.nameOffset;
}

// MPRJ Buses
MilesBus_s::MilesBus_s(const char* stringTable, const MilesBus_v46_s& a)
	: volumeDb(a.volumeDb), outputGainVolumeDb(a.outputGainVolumeDb), pitchSt(a.pitchSt), isExported(false), busIdx_3A(a.busIdx_3A),
	busIdx_3C(a.busIdx_3C), busIdx_3E(a.busIdx_3E), busIdx_40(a.busIdx_40), busIdx_42(a.busIdx_42),
	busIdx_46(a.busIdx_46)
{
	this->name = a.nameOffset == 0xFFFFFFFF ? "(none)" : stringTable + a.nameOffset;
}

// MPRJ
const bool CMilesAudioProject::ParseFile(const std::filesystem::path& path)
{
	Log("MRPJ: Trying to load file: %s\n", path.string().c_str());

	filePath = path;

	if (!FileSystem::ReadFileData(path.string(), &m_fileBuf))
		return false;

	MilesProjectHeader_Short_s* hdrShort = reinterpret_cast<MilesProjectHeader_Short_s*>(m_fileBuf.get());

	if (hdrShort->magic != 'CPRJ')
		return false;

	this->fileVersion = hdrShort->version;

	if (fileVersion < 0)
		return false;

	if (!this->ParseFromHeader())
	{
		Log("MPRJ: Tried to parse unimplemented file version %i.\n", hdrShort->version);
		return false;
	}
	else return true;
}

const bool CMilesAudioProject::ParseFromHeader()
{
	switch (fileVersion)
	{
	case 0x2E: // s30
	{
		this->Construct(GetPtr<MilesProjectHeader_v46_s>(0));

		return true;
	}
	}

	return false;
}

// File Info Window

void CMilesAudioProject::DrawFileInfoWindow()
{
	if (ImGui::BeginTabBar("Miles Project Tab Bar"))
	{
		if (ImGui::BeginTabItem("Controllers", nullptr, ImGuiTabItemFlags_NoReorder))
		{
			for (auto& it : this->controllers)
			{
				ImGui::Text("%s", it.name.c_str());
				if (ImGui::BeginChild(std::format("Controller: {}", it.name).c_str(), ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY))
				{
					ImGui::TextUnformatted(std::format(
						"Type: {}\nRange: [{}, {}]\nDefault: {}",
						it.type, it.min, it.max, it.defaultValue).c_str());
				}
				ImGui::EndChild();
			}

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Buses", nullptr, ImGuiTabItemFlags_NoReorder))
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_TextDisabled));
			if (!this->hasAllBusNames)	ImGui::TextWrapped("This project was compiled without bus names; Bus names shown here are auto-generated");
			ImGui::PopStyleColor();

			for (auto& it : this->buses)
			{
				const bool hasBusIndices = it.busIdx_3A != -1 || it.busIdx_3C != -1 || it.busIdx_3E != -1 || it.busIdx_40 != -1 || it.busIdx_42 != -1 || it.busIdx_46 != -1;

#define GET_BUS_NAME(idx) (idx == -1) ? "" : "\t" + this->buses.at(idx).name + "\n"
				const ImVec2 textCursor = ImGui::GetCursorScreenPos();
				const ImVec2 textSize = ImGui::CalcTextSize(it.name.c_str());
				ImGui::Text("%s", it.name.c_str());

				if (it.isExported)
				{
					ImGui::GetWindowDrawList()->AddLine(textCursor + ImVec2(0.f, textSize.y), textCursor + textSize, 0xFFFFFFFF);
					ImGui::SameLine();
					ImGuiExt::HelpMarker("This is an exported bus. Exported buses are able to be referenced by their name and controlled from game code");
				}

				if (ImGui::BeginChild(std::format("Bus: {}", it.name).c_str(), ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY))
				{
					ImGui::TextUnformatted(std::format("Volume: {} dB\nPitch: {} st", it.volumeDb, it.pitchSt).c_str());

					if(hasBusIndices)
					{
						ImGui::TextUnformatted(
							std::format(
								"Linked Buses:\n{}{}{}{}{}{}",
								GET_BUS_NAME(it.busIdx_3A),
								GET_BUS_NAME(it.busIdx_3C),
								GET_BUS_NAME(it.busIdx_3E),
								GET_BUS_NAME(it.busIdx_40),
								GET_BUS_NAME(it.busIdx_42),
								//GET_BUS_NAME(it.busIdx_44),
								GET_BUS_NAME(it.busIdx_46)
							).c_str());

						ImGui::SameLine();
						ImGuiExt::HelpMarker("Note: I'm currently not sure how they are linked, but these values are used in some way by Miles to reference one bus from another");

					}
				}
				ImGui::EndChild();
			}

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}