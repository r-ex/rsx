#include <pch.h>

#include <core/mdl/qc.h>

extern inline void StaticPropFlipFlop(Vector& in);

namespace qc
{
	static Version_t g_QCExportVersion = s_QCVersion_P2; // global version for exporting qc, only downside is you can't set per model, but who was gonna anyway?

	//
	// QCFile
	//

	bool QCFile::CommandToText(const Command_t* const cmd)
	{
		// cmd buff
		CommandWriteFunc_t const func = cmd->info->writeFunc;
		assertm(func, "writer func was invalid");
		if (!func)
		{
			printf("a %s command was skipped because it didn't have a write function", cmd->info->name);
			return false;
		}

		const size_t remaining = func(cmd, cmdbuf, maxCommandLength);
		assertm(maxCommandLength >= remaining, "ran out of space");

		buffer.WriteString(cmdbuf);

		return true;
	}

	bool QCFile::ParseToText(const bool useIncludeFiles)
	{
		// sort commands
		// grouped by command in groups of the same type
		const size_t numCommands = NumCommands();
		
		// make sure we don't allocate more memory after being sorted for include commands
		if (useIncludeFiles)
		{
			commands.reserve(numCommands + CommandType_t::QCI_COUNT);
		}

		const Command_t** const sorted = reinterpret_cast<const Command_t** const>(buffer.Writer());
		const size_t sortedSize = sizeof(intptr_t) * (numCommands + (CommandType_t::QCI_COUNT * useIncludeFiles));
		buffer.AdvanceWriter(sortedSize);

		const Command_t** temp = reinterpret_cast<const Command_t** const>(buffer.Writer());
		assertm(buffer.Capacity() >= (sizeof(intptr_t) * numCommands * 2), "out of space!");

		size_t numSortedTotal = 0ull;
		for (uint16_t i = 0; i < CommandType_t::QCI_COUNT; i++)
		{
			const uint32_t numSorted = s_CommandTypeSortFunc[i](commands.data(), temp, numCommands, static_cast<CommandType_t>(i));

			if (!numSorted)
				continue;

			memcpy_s(sorted + numSortedTotal, sortedSize - (numSortedTotal * sizeof(intptr_t)), temp, numSorted * sizeof(intptr_t));
			numSortedTotal += numSorted;
		}

		// save base position
		buffer.SetTextStart();

		// write qci files if used
		if (useIncludeFiles)
		{
			std::filesystem::path qcifile(path);
			const std::string stem = path.stem().string();

			for (uint16_t i = 0; i < CommandType_t::QCI_COUNT; i++)
			{
				if (!s_CommandTypeNames[i])
					continue;

				bool typeUsed = false;
				CommandList_t prevCmd = QC_CMDBASE;
				for (size_t cmdIdx = 0; cmdIdx < numCommands; cmdIdx++)
				{
					const Command_t* const cmd = sorted[cmdIdx];

					// check if this command is supported for the desired version
					if (!cmd->info->VersionSupported(g_QCExportVersion))
						continue;

					const CommandType_t type = cmd->GetType();

					// not the correct type
					if (type != i)
						continue;

					typeUsed = true;

					// add new line if it's a different command and in the same group
					if (prevCmd != cmd->GetCmd() && s_CommandList[prevCmd].type == type)
					{
						buffer.WriteCharacter('\n');
					}

					CommandToText(cmd);

					prevCmd = cmd->GetCmd();
				}

				// skip if unused
				if (!typeUsed)
					continue;

				// save this because we do some funky shit
				const size_t fileLength = buffer.TextLength();

				const char* const filename = buffer.Writer();
				buffer.WriteFormatted("%s%s%s\0", stem.c_str(), s_CommandTypeNames[i], s_QCFileExtensions[QCFileExt::QC_EXT_QCI]);
				qcifile.replace_filename(filename);

				// write the qci file
#ifndef STREAMIO
				std::ofstream file(qcifile);
				file.write(buffer.Text(), fileLength);
#else
				StreamIO file(qcifile, eStreamIOMode::Write);
				file.write(buffer.Text(), fileLength);
#endif // !STREAMIO

				buffer.WriterToText();

				// include command
				// todo the placement of these can have huge implications on compile
				CmdParse(this, QC_INCLUDE, filename, 1, true);
				sorted[NumCommands() - 1] = &commands.back();

				buffer.SetTextStart();
			}
		}

		CommandList_t prevCmd = QC_CMDBASE;
		for (size_t cmdIdx = 0; cmdIdx < NumCommands(); cmdIdx++)
		{
			const Command_t* const cmd = sorted[cmdIdx];

			// check if this command is supported for the desired version
			if (!cmd->info->VersionSupported(g_QCExportVersion))
				continue;

			// if we are using qci files, skip commands that are written in qci files
			if (useIncludeFiles && s_CommandTypeNames[cmd->GetType()])
				continue;

			// commands should be in blocks with how we've sorted
			if (prevCmd != cmd->GetCmd())
			{
				buffer.WriteCharacter('\n');
			}

			CommandToText(cmd);

			prevCmd = cmd->GetCmd();
		}

#ifndef STREAMIO
		std::ofstream file(path);
		file.write(buffer.Text(), buffer.TextLength());
#else
		StreamIO file(path, eStreamIOMode::Write);
		file.write(buffer.Text(), buffer.TextLength());
#endif // !STREAMIO

		return true;
	}

	void QCFile::SetExportVersion(const Version_t version)
	{
		g_QCExportVersion = version;
	}


	//
	// GENERIC COMMANDS
	//

	// parse option formatting and call write function for option data
	void CommandOption_Write(const CommandOption_t* option, CTextBuffer* const text, const bool writeSpace, const CommentStyle_t commentStyle)
	{
		assertm((option->data.ParentLine() && option->data.NewLine()) == false, "'QC_FMT_PARENTLINE' and 'QC_FMT_NEWLINE' are not meant to be used together");

		// add a whitespace
		if (writeSpace)
		{
			text->WriteCharacter(' ');
		}

		// file comments
		if (option->data.IsComment() && commentStyle == QC_COMMENT_ONELINE)
		{
			PTEXTBUFFER_WRITE_STRING_CT(text, "//");
		}
		else if (option->data.IsComment() && commentStyle == QC_COMMENT_MULTILINE)
		{
			PTEXTBUFFER_WRITE_STRING_CT(text, "/*");
		}

		// write the name, before curly brackets
		if (option->data.WriteName())
		{
			text->WriteString(option->desc->name);

			// we don't want a white space without options
			if (option->desc->type != QC_OPT_NONE)
			{
				text->WriteCharacter(' ');
			}
		}

		// array contained within curly brackets
		if (option->data.ArrayBracket())
		{
			PTEXTBUFFER_WRITE_STRING_CT(text, "{ ");
		}

		// write the data
		CommandOptionDataFunc_t const func = s_CommandOptionDataFunc[option->TypeAsChar()];
		assertm(func, "write func was nullptr");
		func(&option->data, text);

		// array contained within curly brackets
		if (option->data.ArrayBracket())
		{
			PTEXTBUFFER_WRITE_STRING_CT(text, " }");
		}

		// file comments
		if (option->data.IsComment() && commentStyle == QC_COMMENT_MULTILINE)
		{
			PTEXTBUFFER_WRITE_STRING_CT(text, "*/");
		}

		// add a new line
		if (option->data.NewLine())
		{
			text->WriteCharacter('\n');
		}
	}

	inline const CommandOption_t* const CommandGeneric_LastWritten(const CommandOption_t* options, const uint32_t currentIndex)
	{
		if (currentIndex == 0u)
			return nullptr;

		const CommandOption_t* const option = options + (currentIndex - 1);

		if (option->desc->VersionSupported(g_QCExportVersion))
			return option;

		return CommandGeneric_LastWritten(options, currentIndex - 1);
	}

	size_t CommandGeneric_Write(const Command_t* const command, char* buffer, size_t bufferSize)
	{
		const CommandInfo_t* const info = command->info;
		const CommandOption_t* const options = command->options;
		const uint32_t numOptions = command->numOptions;

		const bool formatComment = command->IsComment(); // are we using comment formatting on this command?

		CTextBuffer text(buffer, bufferSize);
		text.SetTextStart();

		// comment out this comand?
		if (formatComment)
		{
			TEXTBUFFER_WRITE_STRING_CT(text, "//");
		}

		text.WriteString(info->name);

		bool oneLine = true;
		for (uint32_t i = 0; i < numOptions; i++)
		{
			const CommandOption_t* const option = options + i;

			// check if this option is supported on desired qc version
			if (!option->desc->VersionSupported(g_QCExportVersion))
				continue;

			if (!option->data.ParentLine())
			{
				oneLine = false;
				continue;
			}

			CommandOption_Write(option, &text, true, formatComment ? QC_COMMENT_NONE : QC_COMMENT_MULTILINE);
		}

		if (oneLine)
		{
			// add a new line
			text.WriteCharacter('\n');

			// set our buffer for read
			text.WriterToText();
			return text.Capacity();
		}

		// comment out this comand?
		if (formatComment)
		{
			TEXTBUFFER_WRITE_STRING_CT(text, "/*");
		}

		// where do we want to place the curly bracket for our other options?
		if (command->BracketStyle())
		{
			text.WriteCharacter(' ');
		}
		else
		{
			text.WriteCharacter('\n');
		}

		// add a bracket entry
		TEXTBUFFER_WRITE_STRING_CT(text, "{\n");

		text.IncreaseIndenation();
		for (uint32_t i = 0; i < numOptions; i++)
		{
			const CommandOption_t* const option = options + i;

			// check if this option is supported on desired qc version
			if (!option->desc->VersionSupported(g_QCExportVersion))
				continue;

			if (option->data.ParentLine())
				continue;

			// here we're checking if the previous command was written on the same line, don't use an indent if that's the case
			bool writeSpace = true;
			const CommandOption_t* const prevOption = CommandGeneric_LastWritten(options, i);

			// the previous command was the last on its line or there was no previous command
			if (prevOption == nullptr || prevOption->data.NewLine() || prevOption->data.ParentLine())
			{
				text.WriteIndentation();
				writeSpace = false;
			}

			CommandOption_Write(option, &text, writeSpace, formatComment ? QC_COMMENT_NONE : QC_COMMENT_ONELINE);
		}
		text.DecreaseIndenation();

		// add a bracket exit
		TEXTBUFFER_WRITE_STRING_CT(text, "}\n");

		// comment out this comand?
		if (formatComment)
		{
			TEXTBUFFER_WRITE_STRING_CT(text, "*/");
		}

		text.WriterToText();

		return text.Capacity();
	}


	//
	// OPTION DATA
	//

	inline void CommandOption_WriteNone(const CommandOptionData_t* const data, CTextBuffer* const text)
	{
		UNUSED(data);
		UNUSED(text);

		assertm(!data->count, "should contain no options");
	}

	inline void CommandOption_WriteString(const CommandOptionData_t* const data, CTextBuffer* const text)
	{
		const char* const*strings = nullptr;

		if (data->count > 1u)
		{
			strings = reinterpret_cast<const char* const*>(data->value.ptr);
		}
		else
		{
			strings = reinterpret_cast<const char* const*>(&data->value.ptr);
		}

		for (uint32_t i = 0u; i < data->count; i++)
		{
			// write whitespace if this isn't the last string
			if (i > 0u)
			{
				text->WriteCharacter(' ');
			}

			// [rika]: wrap in quotation marks
			text->WriteCharacter('\"');

			text->WriteString(strings[i]);

			// [rika]: wrap in quotation marks
			text->WriteCharacter('\"');
		}
	}

	inline void CommandOption_WriteFloat(const CommandOptionData_t* const data, CTextBuffer* const text)
	{
		const float* floats = nullptr;

		if (data->count > 2u)
		{
			floats = reinterpret_cast<const float* const>(data->value.ptr);
		}
		else
		{
			floats = reinterpret_cast<const float* const>(&data->value.raw);
		}

		for (uint32_t i = 0u; i < data->count; i++)
		{
			// write whitespace if this isn't the last string
			if (i > 0u)
			{
				text->WriteCharacter(' ');
			}

			text->WriteFormatted("%f", floats[i]);
		}
	}

	inline void CommandOption_WriteChar(const CommandOptionData_t* const data, CTextBuffer* const text)
	{
		const char* bytes = nullptr;

		if (data->count > 8u)
		{
			bytes = reinterpret_cast<const char* const>(data->value.ptr);
		}
		else
		{
			bytes = reinterpret_cast<const char* const>(&data->value.raw);
		}

		for (uint32_t i = 0u; i < data->count; i++)
		{
			// write whitespace if this isn't the last string
			if (i > 0u)
			{
				text->WriteCharacter(' ');
			}

			text->WriteFormatted("%i", bytes[i]);
		}
	}

	inline void CommandOption_WriteByte(const CommandOptionData_t* const data, CTextBuffer* const text)
	{
		const uint8_t* bytes = nullptr;

		if (data->count > 8u)
		{
			bytes = reinterpret_cast<const uint8_t* const>(data->value.ptr);
		}
		else
		{
			bytes = reinterpret_cast<const uint8_t* const>(&data->value.raw);
		}

		for (uint32_t i = 0u; i < data->count; i++)
		{
			// write whitespace if this isn't the last string
			if (i > 0u)
			{
				text->WriteCharacter(' ');
			}

			text->WriteFormatted("%u", bytes[i]);
		}
	}

	inline void CommandOption_WriteInt(const CommandOptionData_t* const data, CTextBuffer* const text)
	{
		const int* integers = nullptr;

		if (data->count > 2u)
		{
			integers = reinterpret_cast<const int* const>(data->value.ptr);
		}
		else
		{
			integers = reinterpret_cast<const int* const>(&data->value.raw);
		}

		for (uint32_t i = 0u; i < data->count; i++)
		{
			// write whitespace if this isn't the last string
			if (i > 0u)
			{
				text->WriteCharacter(' ');
			}

			text->WriteFormatted("%i", integers[i]);
		}
	}

	inline void CommandOption_WriteUInt(const CommandOptionData_t* const data, CTextBuffer* const text)
	{
		const uint32_t* integers = nullptr;

		if (data->count > 2u)
		{
			integers = reinterpret_cast<const uint32_t* const>(data->value.ptr);
		}
		else
		{
			integers = reinterpret_cast<const uint32_t* const>(&data->value.raw);
		}

		for (uint32_t i = 0u; i < data->count; i++)
		{
			// write whitespace if this isn't the last string
			if (i > 0u)
			{
				text->WriteCharacter(' ');
			}

			text->WriteFormatted("%u", integers[i]);
		}
	}

	// todo add other types
	inline void CommandOption_WritePair(const CommandOptionData_t* const data, CTextBuffer* const text)
	{
		const CommandOptionPair_t* const pair = reinterpret_cast<const CommandOptionPair_t* const>(data->value.ptr);

		for (uint32_t i = 0u; i < 2u; i++)
		{
			s_CommandOptionDataFunc[pair->type[i]](&pair->data[i], text);

			if (i == 0)
			{
				text->WriteCharacter(' ');
			}
		}
	}

	// todo add other types
	inline void CommandOption_WriteArray(const CommandOptionData_t* const data, CTextBuffer* const text)
	{
		const CommandOptionArray_t* const array = reinterpret_cast<const CommandOptionArray_t* const>(data->value.ptr);

		const bool formatComment = false; // todo: formatting

		uint32_t numInBracket = 0u;
		for (uint32_t i = 0u; i < array->numOptions; i++)
		{
			const CommandOption_t* const option = array->options + i;

			// check if this option is supported on desired qc version
			if (!option->desc->VersionSupported(g_QCExportVersion))
				continue;

			if (!option->data.ParentLine())
			{
				numInBracket++;
				continue;
			}

			CommandOption_Write(option, text, true, formatComment ? QC_COMMENT_NONE : QC_COMMENT_MULTILINE);
		}

		if (numInBracket == 0u)
		{
			// new line for option/command if desired
			if (array->NewLine())
			{
				text->WriteCharacter('\n');
			}

			return;
		}

		//  new line for array
		text->WriteCharacter('\n');

		// add a bracket entry
		text->WriteIndentation();

		// comment out this comand?
		if (formatComment)
		{
			PTEXTBUFFER_WRITE_STRING_CT(text, "/*");
		}

		PTEXTBUFFER_WRITE_STRING_CT(text, "{\n");

		text->IncreaseIndenation(); // indent once more!
		for (uint32_t i = 0u, idx = 0u; idx < numInBracket; i++)
		{
			if (i >= array->numOptions)
				break;

			const CommandOption_t* const option = array->options + i;

			// check if this option is supported on desired qc version
			if (!option->desc->VersionSupported(g_QCExportVersion))
				continue;

			if (option->data.ParentLine())
				continue;

			// here we're checking if the previous command was written on the same line, don't use an indent if that's the case
			bool writeSpace = true;
			const CommandOption_t* const prevOption = CommandGeneric_LastWritten(array->options, i);

			// the previous command was the last on its line or there was no previous command
			if (prevOption == nullptr || prevOption->data.NewLine() || prevOption->data.ParentLine())
			{
				text->WriteIndentation();
				writeSpace = false;
			}

			CommandOption_Write(option, text, writeSpace, formatComment ? QC_COMMENT_NONE : QC_COMMENT_ONELINE);

			idx++;
		}
		text->DecreaseIndenation(); // we're outside, remove it

		// add a bracket exit
		text->WriteIndentation();
		text->WriteCharacter('}');

		// comment out this comand?
		if (formatComment)
		{
			PTEXTBUFFER_WRITE_STRING_CT(text, "*/");
		}

		// new line for option/command if desired
		if (array->NewLine())
		{
			text->WriteCharacter('\n');
		}
	}

	constexpr CommandOptionDesc_t s_CommandGeneric_Option_None(QC_OPT_NONE, "value");
	CommandOption_t* CommandNone_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		UNUSED(file);
		UNUSED(data);
		UNUSED(count);
		UNUSED(store);

		*numOptions = 0;

		return nullptr;
	}

	constexpr CommandOptionDesc_t s_CommandGeneric_Option_String(QC_OPT_STRING, "value");
	CommandOption_t* CommandString_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		UNUSED(count);

		CommandOption_t* const options = new CommandOption_t[1];
		options->Init(&s_CommandGeneric_Option_String);

		if (store)
		{
			options->SaveStr(file, reinterpret_cast<const char* const>(data));
		}
		else
		{
			options->SetPtr(data);
		}

		*numOptions = 1;

		return options;
	}

	constexpr CommandOptionDesc_t s_CommandGeneric_Option_Float(QC_OPT_FLOAT, "value");
	CommandOption_t* CommandFloat_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		CommandOption_t* const options = new CommandOption_t[1];
		options->Init(&s_CommandGeneric_Option_Float);

		if (count > 2) {
			if (store)
				options->SavePtr(file, data, count, sizeof(float) * count);
			else
				options->SetPtr(data, count);
		}
		else
		{
			options->SetRaw(data, count, sizeof(float) * count);
		}

		*numOptions = 1;

		return options;
	}

	constexpr CommandOptionDesc_t s_CommandGeneric_Option_Byte(QC_OPT_BYTE, "value");
	CommandOption_t* CommandByte_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		CommandOption_t* const options = new CommandOption_t[1];
		options->Init(&s_CommandGeneric_Option_Byte);

		if (count > 8)
		{
			if (store)
				options->SavePtr(file, data, count, count);
			else
				options->SetPtr(data, count);
		}
		else
		{
			options->SetRaw(data, count, count);
		}

		*numOptions = 1;

		return options;
	}

	constexpr CommandOptionDesc_t s_CommandGeneric_Option_Int(QC_OPT_INT, "value");
	CommandOption_t* CommandInt_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		CommandOption_t* const options = new CommandOption_t[1];
		options->Init(&s_CommandGeneric_Option_Int);

		if (count > 2)
		{
			if (store)
				options->SavePtr(file, data, count, sizeof(int) * count);
			else
				options->SetPtr(data, count);
		}
		else
		{
			options->SetRaw(data, count, sizeof(int) * count);
		}

		*numOptions = 1;

		return options;
	}

	constexpr CommandOptionDesc_t s_CommandGeneric_Option_Pair(QC_OPT_PAIR, "value");
	CommandOption_t* CommandPair_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		assertm(count == 1, "array of pairs not supported in this func");

		const CommandOptionPair_t* pair = reinterpret_cast<const CommandOptionPair_t* const>(data);

		CommandOption_t* const options = new CommandOption_t[1];

		options->Init(&s_CommandGeneric_Option_Pair);

		if (store)
		{
			CommandOptionPair_t tmp;
			memcpy_s(&tmp, sizeof(CommandOptionPair_t), pair, sizeof(CommandOptionPair_t));

			for (uint32_t idx = 0; idx < 2; idx++)
			{
				CommandOptionData_t* const pairData = &tmp.data[idx];

				switch (pair->type[idx])
				{
				case CommandOptionType_t::QC_OPT_STRING:
				{
					// todo: fix this!
					assertm(pairData->count == 1, "no array string ty");
					pairData->SaveStr(file, reinterpret_cast<const char*>(pairData->value.ptr));

					break;
				}
				case CommandOptionType_t::QC_OPT_FLOAT:
				case CommandOptionType_t::QC_OPT_INT:
				{
					if (pairData->count <= 2)
						break;

					pairData->SavePtr(file, pairData->value.ptr, pairData->count, 4u * pairData->count);

					break;
				}
				case CommandOptionType_t::QC_OPT_BYTE:
				{
					if (pairData->count <= 8)
						break;

					pairData->SavePtr(file, pairData->value.ptr, pairData->count, pairData->count);

					break;
				}
				case CommandOptionType_t::QC_OPT_NONE:
				case CommandOptionType_t::QC_OPT_PAIR:
				default:
				{
					break;
				}
				}
			}

			options->SavePtr(file, &tmp, count, sizeof(CommandOptionPair_t));
		}
		else
		{

			options->SavePtr(file, data, count, sizeof(CommandOptionPair_t));
		}

		*numOptions = 1;

		return options;
	}


	//
	// CUSTOM COMMANDS
	//

	//
	// static options
	//
	
	// misc
	constexpr CommandOptionDesc_t s_CommandGeneric_Option_Name(QC_OPT_STRING, "name");
	constexpr CommandOptionDesc_t s_CommandGeneric_Option_Pos(QC_OPT_FLOAT, "pos");
	constexpr CommandOptionDesc_t s_CommandGeneric_Option_Rot(QC_OPT_FLOAT, "rot");
	constexpr CommandOptionDesc_t s_CommandGeneric_Option_Bone(QC_OPT_STRING, "bone");
	constexpr CommandOptionDesc_t s_CommandGeneric_Option_BlankLine(QC_OPT_NONE, "blank", QC_FMT_NEWLINE); // just write a new line

	// materials
	constexpr CommandOptionDesc_t s_CommandTextureGroup_Option_Name(QC_OPT_STRING, "skin_name", QC_FMT_NONE, s_QCVersion_R5_080, s_QCVersion_R5_RETAIL);
	constexpr CommandOptionDesc_t s_CommandTextureGroup_Option_Skin(QC_OPT_STRING, "skin", (QC_FMT_ARRAY | QC_FMT_NEWLINE));

	// models
	constexpr CommandOptionDesc_t s_CommandBodyGroup_Option_Studio(QC_OPT_STRING, "studio", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandBodyGroup_Option_Blank(QC_OPT_NONE, "blank", s_CommandOptionFormatNameNewLine);

	constexpr CommandOptionDesc_t s_CommandLOD_Option_Threshold(QC_OPT_FLOAT, "threshold");
	constexpr CommandOptionDesc_t s_CommandLOD_Option_ReplaceModel(QC_OPT_PAIR, "replacemodel", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandLOD_Option_RemoveModel(QC_OPT_STRING, "removemodel", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandLOD_Option_ReplaceBone(QC_OPT_PAIR, "replacebone", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandLOD_Option_ReplaceMaterial(QC_OPT_PAIR, "replacematerial", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandLOD_Option_ShadowLODMaterials(QC_OPT_NONE, "use_shadowlod_materials", s_CommandOptionFormatNameNewLine);

	// bones
	constexpr CommandOptionDesc_t s_CommandDefineBone_Option_Parent(QC_OPT_STRING, "parent");
	//static const CommandOptionDesc_t s_CommandDefineBone_Option_Pos(QC_OPT_FLOAT, "pos");
	//static const CommandOptionDesc_t s_CommandDefineBone_Option_Rot(QC_OPT_FLOAT, "rot");
	constexpr CommandOptionDesc_t s_CommandDefineBone_Option_FixupPos(QC_OPT_FLOAT, "fixupPos");
	constexpr CommandOptionDesc_t s_CommandDefineBone_Option_FixupRot(QC_OPT_FLOAT, "fixupRot");
	constexpr const char* const s_CommandDefineBone_Value_Parent = "";
	constexpr CommandOptionDesc_t s_CommandAttachment_Option_Type(QC_OPT_STRING, "type");

	// jiggly wiggly boing boing
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_IsRigid(QC_OPT_ARRAY, "is_rigid", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_IsFlexible(QC_OPT_ARRAY, "is_flexible", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_BaseSpring(QC_OPT_ARRAY, "has_base_spring", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_IsBoing(QC_OPT_ARRAY, "is_boing", s_CommandOptionFormatNameNewLine);

	// common
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_Length(QC_OPT_FLOAT, "length", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_TipMass(QC_OPT_FLOAT, "tip_mass", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_TipFriction(QC_OPT_FLOAT, "tip_friction", s_CommandOptionFormatNameNewLine, s_QCVersion_R5_080, s_QCVersion_R5_RETAIL);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_NoFlex(QC_OPT_NONE, "no_flex", s_CommandOptionFormatNameNewLine, s_QCVersion_R5_080, s_QCVersion_R5_RETAIL);

	// constraints
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_AngleConstraint(QC_OPT_FLOAT, "angle_constraint", s_CommandOptionFormatNameNewLine);

	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_YawConstraint(QC_OPT_FLOAT, "yaw_constraint", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_YawFriction(QC_OPT_FLOAT, "yaw_friction", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_YawBounce(QC_OPT_FLOAT, "yaw_bounce", s_CommandOptionFormatNameNewLine);

	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_PitchConstraint(QC_OPT_FLOAT, "pitch_constraint", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_PitchFriction(QC_OPT_FLOAT, "pitch_friction", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_PitchBounce(QC_OPT_FLOAT, "pitch_bounce", s_CommandOptionFormatNameNewLine);

	// flexible
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_YawStiffness(QC_OPT_FLOAT, "yaw_stiffness", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_YawDamping(QC_OPT_FLOAT, "yaw_damping", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_PitchStiffness(QC_OPT_FLOAT, "pitch_stiffness", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_PitchDamping(QC_OPT_FLOAT, "pitch_damping", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_AlongStiffness(QC_OPT_FLOAT, "along_stiffness", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_AlongDamping(QC_OPT_FLOAT, "along_damping", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_AllowLengthFlex(QC_OPT_NONE, "allow_length_flex", s_CommandOptionFormatNameNewLine);

	// base spring
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_BaseStiffness(QC_OPT_FLOAT, "stiffness", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_BaseDamping(QC_OPT_FLOAT, "damping", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_LeftConstraint(QC_OPT_FLOAT, "left_constraint", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_LeftFriction(QC_OPT_FLOAT, "left_friction", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_UpConstraint(QC_OPT_FLOAT, "up_constraint", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_UpFriction(QC_OPT_FLOAT, "up_friction", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_ForwardConstraint(QC_OPT_FLOAT, "forward_constraint", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_ForwardFriction(QC_OPT_FLOAT, "forward_friction", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_BaseMass(QC_OPT_FLOAT, "base_mass", s_CommandOptionFormatNameNewLine);

	// boing
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_BoingImpactSpeed(QC_OPT_FLOAT, "impact_speed", s_CommandOptionFormatNameNewLine, s_QCVersion_TF2, s_QCVersion_TF2);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_BoingImpactAngle(QC_OPT_FLOAT, "impact_angle", s_CommandOptionFormatNameNewLine, s_QCVersion_TF2, s_QCVersion_TF2);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_BoingDampingRate(QC_OPT_FLOAT, "damping_rate", s_CommandOptionFormatNameNewLine, s_QCVersion_TF2, s_QCVersion_TF2);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_BoingFrequency(QC_OPT_FLOAT, "frequency", s_CommandOptionFormatNameNewLine, s_QCVersion_TF2, s_QCVersion_TF2);
	constexpr CommandOptionDesc_t s_CommandJiggleBone_Option_BoingAmplitude(QC_OPT_FLOAT, "amplitude", s_CommandOptionFormatNameNewLine, s_QCVersion_TF2, s_QCVersion_TF2);

	// animation
	constexpr CommandOptionDesc_t s_CommandPoseParam_Option_Start(QC_OPT_FLOAT, "start");
	constexpr CommandOptionDesc_t s_CommandPoseParam_Option_End(QC_OPT_FLOAT, "end");
	constexpr CommandOptionDesc_t s_CommandPoseParam_Option_Wrap(QC_OPT_NONE, "wrap", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandPoseParam_Option_Loop(QC_OPT_FLOAT, "loop", s_CommandOptionFormatNameParentLine);

	constexpr CommandOptionDesc_t s_CommandIKChain_Option_Knee(QC_OPT_FLOAT, "knee", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandIKChain_Option_Angle(QC_OPT_FLOAT, "angle_unknown", s_CommandOptionFormatNameParentLine, s_QCVersion_R2, s_QCVersion_R5_RETAIL);

	constexpr CommandOptionDesc_t s_CommandIKAutoPlayLock_Option_LockPosition(QC_OPT_FLOAT, "lock_position");
	constexpr CommandOptionDesc_t s_CommandIKAutoPlayLock_Option_InheritRotation(QC_OPT_FLOAT, "inherit_rotation");

	constexpr CommandOptionDesc_t s_CommandSectionFrames_Option_FrameCount(QC_OPT_INT, "frame_count");
	constexpr CommandOptionDesc_t s_CommandSectionFrames_Option_MinFrames(QC_OPT_INT, "min_frames");

	constexpr CommandOptionDesc_t s_CommandWeightList_Option_Weight(QC_OPT_PAIR, "weight", QC_FMT_NEWLINE);

	// collision
	constexpr CommandOptionDesc_t s_CommandContents_Option_Flag(QC_OPT_STRING, "flag");

	constexpr CommandOptionDesc_t s_CommandHBox_Option_Group(QC_OPT_INT, "group");
	constexpr CommandOptionDesc_t s_CommandHBox_Option_Min(QC_OPT_FLOAT, "min");
	constexpr CommandOptionDesc_t s_CommandHBox_Option_Max(QC_OPT_FLOAT, "max");
	constexpr CommandOptionDesc_t s_CommandHBox_Option_Angle(QC_OPT_FLOAT, "angle", s_CommandOptionFormatDefault, s_QCVersion_CSGO, s_QCVersion_CSGO);
	constexpr CommandOptionDesc_t s_CommandHBox_Option_Radius(QC_OPT_FLOAT, "radius", s_CommandOptionFormatDefault, s_QCVersion_CSGO, s_QCVersion_CSGO);
	constexpr CommandOptionDesc_t s_CommandHBox_Option_HitData(QC_OPT_STRING, "hitdata_group", s_CommandOptionFormatNameParentLine, s_QCVersion_R2, s_QCVersion_R5_RETAIL);
	constexpr CommandOptionDesc_t s_CommandHBox_Option_Crit(QC_OPT_NONE, "force_crit", s_CommandOptionFormatNameParentLine, s_QCVersion_R2, s_QCVersion_R5_150);

	//
	// functions
	//

	// misc
	CommandOption_t* CommandConstDirectLight_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		UNUSED(file);
		UNUSED(count);
		UNUSED(store);

		const uint8_t intensityPacked = reinterpret_cast<const uint8_t* const>(data)[0];

		const uint32_t usedOptions = 1;
		CommandOption_t* options = new CommandOption_t[usedOptions];

		const float intensity = static_cast<float>(intensityPacked) / 255.0f;
		options[0].Init(&s_CommandGeneric_Option_Float);
		options[0].SetRaw(&intensity, 1, sizeof(float));

		*numOptions = usedOptions;

		return options;
	}
	
	CommandOption_t* CommandIllumPosition_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		UNUSED(count);
		UNUSED(store);

		const IllumPositionData_t* const illumdata = reinterpret_cast<const IllumPositionData_t* const>(data);

		const uint32_t usedOptions = illumdata->useAttachment() ? 2 : 1;
		CommandOption_t* options = new CommandOption_t[usedOptions];

		Vector illumposition;
		if (illumdata->useAttachment() && usedOptions > 1u)
		{
			// [rika]: forget if you can index into this type like this
			illumposition.Init(illumdata->localmatrix->m_flMatVal[0][3], illumdata->localmatrix->m_flMatVal[1][3], illumdata->localmatrix->m_flMatVal[2][3]);

			options[1].Init(&s_CommandGeneric_Option_Bone);
			options[1].SetPtr(illumdata->localbone);
		}
		else
		{
			illumposition = *illumdata->illumposition;
			StaticPropFlipFlop(illumposition);
		}

		options[0].Init(&s_CommandGeneric_Option_Pos);
		options[0].SavePtr(file, &illumposition, 3, sizeof(Vector)); // store it so we don't cause the Funny


		*numOptions = usedOptions;

		return options;
	}
	
	CommandOption_t* CommandMaxEyeDeflect_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		UNUSED(file);
		UNUSED(count);
		UNUSED(store);

		const float flMaxEyeDeflection = *reinterpret_cast<const float* const>(data);

		const uint32_t usedOptions = 1;
		CommandOption_t* options = new CommandOption_t[usedOptions];

		const float fixedAngle = RAD2DEG(acosf(flMaxEyeDeflection));
		options[0].Init(&s_CommandGeneric_Option_Float);
		options[0].SetRaw(&fixedAngle, 1u, sizeof(float)); // store it so we don't cause the Funny

		*numOptions = usedOptions;

		return options;
	}

	// write function for qc because the string requires some post processing
	size_t CommandKeyvalues_Write(const Command_t* const command, char* buffer, size_t bufferSize)
	{
		const CommandInfo_t* const info = command->info;
		const CommandOption_t* const options = command->options;
		const uint32_t numOptions = command->numOptions;

		assertm(info->id == CommandList_t::QC_KEYVALUES, "command was not $keyvalues");
		assertm(numOptions == 1u, "$keyvalues can only have one option");
		assertm(options[0].desc->type == CommandOptionType_t::QC_OPT_STRING, "option was not the correct type");

		const bool formatComment = command->IsComment(); // are we using comment formatting on this command?

		CTextBuffer text(buffer, bufferSize);
		text.SetTextStart();

		// comment out this comand?
		if (formatComment)
		{
			TEXTBUFFER_WRITE_STRING_CT(text, "/*");
		}

		text.WriteString(info->name);

		const char* keyvalues = reinterpret_cast<const char*>(options[0].data.value.ptr);

#ifdef _DEBUG
		constexpr const char* const basekey = "mdlkeyvalue";

		const char* const result = strstr(keyvalues, basekey);
		assertm(result == keyvalues, "keyvalues did not start with 'mdlkeyvalue'");
#endif // _DEBUG

		kv_parser::Token_t rootToken(&keyvalues, kv_parser::TOKEN_KEY);
		rootToken.ReplaceToken(nullptr, kv_parser::TOKEN_NONE); // discard mdlkeyvalue

		rootToken.Serialize(&text);

		// comment out this comand?
		if (formatComment)
		{
			TEXTBUFFER_WRITE_STRING_CT(text, "*/");
		}

		// add a new line
		text.WriteCharacter('\n');

		text.WriterToText();
		return text.Capacity();
	}

	size_t CommandCollText_Write(const Command_t* const command, char* buffer, size_t bufferSize)
	{
		const CommandInfo_t* const info = command->info;
		const CommandOption_t* const options = command->options;
		const uint32_t numOptions = command->numOptions;

		assertm(info->id == CommandList_t::QC_COLLISIONTEXT, "command was not $collisiontext");
		assertm(numOptions == 1u, "$collisiontext can only have one option");
		assertm(options[0].desc->type == CommandOptionType_t::QC_OPT_STRING, "option was not the correct type");

		const bool formatComment = command->IsComment(); // are we using comment formatting on this command?

		CTextBuffer text(buffer, bufferSize);
		text.SetTextStart();

		// comment out this comand?
		if (formatComment)
		{
			TEXTBUFFER_WRITE_STRING_CT(text, "/*");
		}

		text.WriteString(info->name);
		TEXTBUFFER_WRITE_STRING_CT(text, "\n{\n");

		const char* properties = reinterpret_cast<const char* const>(options[0].data.value.ptr);

		text.IncreaseIndenation();
		while (properties[0])
		{
			kv_parser::Token_t token(&properties, kv_parser::TOKEN_KEY);
			token.Serialize(&text);
		}
		text.DecreaseIndenation();

		text.WriteCharacter('}');

		// comment out this comand?
		if (formatComment)
		{
			TEXTBUFFER_WRITE_STRING_CT(text, "*/");
		}

		// add a new line
		text.WriteCharacter('\n');

		text.WriterToText();
		return text.Capacity();
	}

	// materials
	CommandOption_t* CommandTextureGroup_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		UNUSED(count);
		UNUSED(store);

		const TextureGroupData_t* const skinData = reinterpret_cast<const TextureGroupData_t* const>(data);
		const bool hasNames = skinData->HasNames();

		const uint32_t usedOptions = skinData->numSkinFamilies + 1 + (hasNames * skinData->numSkinFamilies);
		CommandOption_t* options = new CommandOption_t[usedOptions];

		options[0].Init(&s_CommandGeneric_Option_Name);
		options[0].SetPtr("skinfamilies");

		const char** skinGroup = new const char* [skinData->numIndices];
		const int16_t* skinFamily = skinData->skins;
		const char** skinNames = skinData->names;

		for (uint32_t i = 1; i < usedOptions; i++)
		{
			if (hasNames)
			{
				options[i].Init(&s_CommandTextureGroup_Option_Name);
				options[i].SetPtr(*skinNames);

				skinNames++;
				i++;
			}

			// get the material for a skin
			for (uint32_t indice = 0; indice < skinData->numIndices; indice++)
			{
				const int16_t slot = skinData->indices[indice]; // get the slot of a changed material
				const int16_t material = skinFamily[slot]; // get the material index for a given slot in a skin family

				skinGroup[indice] = skinData->materials[material];
			}

			options[i].Init(&s_CommandTextureGroup_Option_Skin);

			// not an array so just save pointer to that string
			if (skinData->numIndices == 1)
				options[i].SetPtr(skinGroup[0]);
			else
				options[i].SavePtr(file, skinGroup, skinData->numIndices, sizeof(intptr_t) * skinData->numIndices); // store it so we don't cause the Funny

			skinFamily += skinData->numSkinRef;
		}

		FreeAllocArray(skinGroup);

		*numOptions = usedOptions;

		return options;
	}

	// bones
	CommandOption_t* CommandDefineBone_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		UNUSED(count);
		UNUSED(store);

		const BoneData_t* const boneData = reinterpret_cast<const BoneData_t* const>(data);

		const uint32_t usedOptions = boneData->UseFixups() ? 6 : 4;
		CommandOption_t* options = new CommandOption_t[usedOptions];

		options[0].Init(&s_CommandGeneric_Option_Bone);
		options[0].SetPtr(boneData->name);

		options[1].Init(&s_CommandDefineBone_Option_Parent);
		options[1].SetPtr(boneData->parent ? boneData->parent : s_CommandDefineBone_Value_Parent);

		options[2].Init(&s_CommandGeneric_Option_Pos);
		options[2].SetPtr(boneData->position, 3);

		const QAngle angles(*boneData->rotation);
		options[3].Init(&s_CommandGeneric_Option_Rot);
		options[3].SavePtr(file, &angles, 3, sizeof(QAngle));

		// [rika]: command doesn't need to have this data, write only if provided
		if (usedOptions >= 6 && boneData->UseFixups())
		{
			Vector fixupPosition;
			QAngle fixupAngles;
			MatrixAngles(*boneData->fixupMatrix, fixupAngles, fixupPosition);

			options[4].Init(&s_CommandDefineBone_Option_FixupPos);
			options[4].SavePtr(file, &fixupPosition, 3, sizeof(Vector));

			options[5].Init(&s_CommandDefineBone_Option_FixupRot);
			options[5].SavePtr(file, &fixupAngles, 3, sizeof(QAngle));
		}

		*numOptions = usedOptions;

		return options;
	}

	CommandOption_t* CommandJointContents_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		UNUSED(file);
		UNUSED(count);
		UNUSED(store);

		const BoneData_t* const boneData = reinterpret_cast<const BoneData_t* const>(data);
		const int contents = boneData->contents;

		uint32_t usedFlags = __popcnt(contents);
		if (usedFlags == 0u)
		{
			usedFlags = 1u;
		}

		const uint32_t usedOptions = usedFlags + 1;
		CommandOption_t* options = new CommandOption_t[usedOptions];

		uint32_t optionsIndex = 0u;

		// set bone name
		options[optionsIndex].Init(&s_CommandGeneric_Option_Bone);
		options[optionsIndex].SetPtr(boneData->name);
		optionsIndex++;

		CommandContents_ParseOptions(options, usedOptions, optionsIndex, contents);

		*numOptions = usedOptions;

		return options;
	}

	CommandOption_t* CommandScreenAlign_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store)
	{
		UNUSED(count);

		const BoneData_t* const boneData = reinterpret_cast<const BoneData_t* const>(data);

		const uint32_t usedOptions = 2u;
		CommandOption_t* options = new CommandOption_t[usedOptions];

		uint32_t optionsIndex = 0u;

		// set bone name
		options[optionsIndex].Init(&s_CommandGeneric_Option_Bone);
		if (store)
		{
			options[optionsIndex].SaveStr(file, boneData->name);
		}
		else
		{
			options[optionsIndex].SetPtr(boneData->name);
		}
		optionsIndex++;

		options[optionsIndex].Init(&s_CommandGeneric_Option_Name);
		options[optionsIndex].SetPtr(s_ScreenAlignType[boneData->ScreenAlignShape()]);
		optionsIndex++;

		*numOptions = usedOptions;

		return options;
	}

	CommandOption_t* CommandBoneSaveFrame_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store)
	{
		UNUSED(count);

		const BoneData_t* const boneData = reinterpret_cast<const BoneData_t* const>(data);

		const bool savePosition = boneData->flags & BoneData_t::QCB_HAS_SAVEFRAME_POS;
		const bool saveRotation = boneData->flags & (BoneData_t::QCB_HAS_SAVEFRAME_ROT64 | BoneData_t::QCB_HAS_SAVEFRAME_ROT32);

		const uint32_t usedOptions = 1u + savePosition + saveRotation;
		CommandOption_t* options = new CommandOption_t[usedOptions];

		uint32_t optionsIndex = 0u;

		// set bone name
		options[optionsIndex].Init(&s_CommandGeneric_Option_Bone);
		if (store)
		{
			options[optionsIndex].SaveStr(file, boneData->name);
		}
		else
		{
			options[optionsIndex].SetPtr(boneData->name);
		}
		optionsIndex++;

		if (savePosition)
		{
			options[optionsIndex].Init(&s_CommandGeneric_Option_Name);
			options[optionsIndex].SetPtr(s_BoneSaveFrameType[BoneData_t::BONESAVE_POS]);
			optionsIndex++;
		}

		if (saveRotation)
		{
			options[optionsIndex].Init(&s_CommandGeneric_Option_Name);
			options[optionsIndex].SetPtr(s_BoneSaveFrameType[BoneData_t::BONESAVE_ROT]);
			optionsIndex++;
		}

		*numOptions = usedOptions;

		return options;
	}

	const uint32_t CommandJiggleBone_ParseCommonOptions(const JiggleData_t* const jiggleData, CommandOptionArray_t* const jiggle_property, uint32_t optionsIndex)
	{
		// default of 10.0f so we will write regardless
		jiggle_property->options[optionsIndex].Init(&s_CommandJiggleBone_Option_Length);
		jiggle_property->options[optionsIndex].SetRaw(&jiggleData->length, 1u, sizeof(float));

		// check for a default value
		if (jiggleData->tipMass != 0.0f)
		{
			jiggle_property->options[optionsIndex].Init(&s_CommandJiggleBone_Option_TipMass);
			jiggle_property->options[optionsIndex].SetRaw(&jiggleData->tipMass, 1u, sizeof(float));

			optionsIndex++;
		}

		// check for a default value
		if (jiggleData->tipFriction != 0.0f)
		{
			jiggle_property->options[optionsIndex].Init(&s_CommandJiggleBone_Option_TipFriction);
			jiggle_property->options[optionsIndex].SetRaw(&jiggleData->tipFriction, 1u, sizeof(float));

			optionsIndex++;
		}

		// if JIGGLE_HAS_FLEX is not set on an apex jigglebone
		if (jiggleData->HasNoFlex())
		{
			jiggle_property->options[optionsIndex].Init(&s_CommandJiggleBone_Option_NoFlex);

			optionsIndex++;
		}

		if (jiggleData->HasAngleConstraint() && optionsIndex < jiggle_property->numOptions)
		{
			const float angleLimit = RAD2DEG(jiggleData->angleLimit);
			jiggle_property->options[optionsIndex].Init(&s_CommandJiggleBone_Option_AngleConstraint);
			jiggle_property->options[optionsIndex].SetRaw(&angleLimit, 1u, sizeof(float));

			optionsIndex++;
		}

		// friction and bounce options get read from qc if flag isn't set, but are never used by the game if it isn't, do not bother writing
		if (jiggleData->HasYawConstraint() && optionsIndex < jiggle_property->numOptions)
		{
			const Vector2D constraint(RAD2DEG(jiggleData->minYaw), RAD2DEG(jiggleData->maxYaw));
			jiggle_property->options[optionsIndex].Init(&s_CommandJiggleBone_Option_YawConstraint);
			jiggle_property->options[optionsIndex].SetRaw(&constraint, 2u, sizeof(Vector2D));

			optionsIndex++;

			// check for a default value
			if (jiggleData->yawFriction != 0.0f)
			{
				jiggle_property->options[optionsIndex].Init(&s_CommandJiggleBone_Option_YawFriction);
				jiggle_property->options[optionsIndex].SetRaw(&jiggleData->yawFriction, 1u, sizeof(float));

				optionsIndex++;
			}

			// check for a default value
			if (jiggleData->yawBounce != 0.0f)
			{
				jiggle_property->options[optionsIndex].Init(&s_CommandJiggleBone_Option_YawBounce);
				jiggle_property->options[optionsIndex].SetRaw(&jiggleData->yawBounce, 1u, sizeof(float));

				optionsIndex++;
			}
		}

		// friction and bounce options get read from qc if flag isn't set, but are never used by the game if it isn't, do not bother writing
		if (jiggleData->HasPitchConstraint() && optionsIndex < jiggle_property->numOptions)
		{
			const Vector2D constraint(RAD2DEG(jiggleData->minPitch), RAD2DEG(jiggleData->maxPitch));
			jiggle_property->options[optionsIndex].Init(&s_CommandJiggleBone_Option_PitchConstraint);
			jiggle_property->options[optionsIndex].SetRaw(&constraint, 2u, sizeof(Vector2D)); // takes minYaw and maxYaw

			optionsIndex++;

			// check for a default value
			if (jiggleData->pitchFriction != 0.0f)
			{
				jiggle_property->options[optionsIndex].Init(&s_CommandJiggleBone_Option_PitchFriction);
				jiggle_property->options[optionsIndex].SetRaw(&jiggleData->pitchFriction, 1u, sizeof(float));

				optionsIndex++;
			}

			// check for a default value
			if (jiggleData->pitchBounce != 0.0f)
			{
				jiggle_property->options[optionsIndex].Init(&s_CommandJiggleBone_Option_PitchBounce);
				jiggle_property->options[optionsIndex].SetRaw(&jiggleData->pitchBounce, 1u, sizeof(float));

				optionsIndex++;
			}
		}

		return optionsIndex;
	}

	CommandOption_t* CommandJiggleBone_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		UNUSED(file);
		UNUSED(count);
		UNUSED(store);

		const JiggleData_t* const jiggleData = reinterpret_cast<const JiggleData_t* const>(data);

		const uint32_t usedOptions = 1 + jiggleData->IsFlexible() + jiggleData->IsRigid() + jiggleData->HasBaseSpring() + jiggleData->IsBoing(); // this should be a max of three because is_rigid and is_flexible as well as has_base_spring and is_boing are mutually exclusive
		CommandOption_t* options = new CommandOption_t[usedOptions];

		options[0].Init(&s_CommandGeneric_Option_Bone);
		options[0].SetPtr(jiggleData->bone);

		uint32_t optionsIndex = 1u;

		// is_flexible and is_rigid
		if (jiggleData->IsFlexible() && optionsIndex < usedOptions)
		{
			uint32_t flexibleOptionIndex = 0u;

			const uint32_t numFlexibleOptions = jiggleData->CommonJiggleOptionCount() + jiggleData->FlexibleOptionCount();
			CommandOptionArray_t is_flexible(file, numFlexibleOptions);

			// stiffness defaults to 100.0f
			if (jiggleData->yawStiffness != 100.0f)
			{
				is_flexible.options[flexibleOptionIndex].Init(&s_CommandJiggleBone_Option_YawStiffness);
				is_flexible.options[flexibleOptionIndex].SetRaw(&jiggleData->yawStiffness, 1u, sizeof(float));

				flexibleOptionIndex++;
			}

			// damping defaults to 0.0f
			if (jiggleData->yawDamping != 0.0f)
			{
				is_flexible.options[flexibleOptionIndex].Init(&s_CommandJiggleBone_Option_YawDamping);
				is_flexible.options[flexibleOptionIndex].SetRaw(&jiggleData->yawDamping, 1u, sizeof(float));

				flexibleOptionIndex++;
			}

			// stiffness defaults to 100.0f
			if (jiggleData->pitchStiffness != 100.0f)
			{
				is_flexible.options[flexibleOptionIndex].Init(&s_CommandJiggleBone_Option_PitchStiffness);
				is_flexible.options[flexibleOptionIndex].SetRaw(&jiggleData->pitchStiffness, 1u, sizeof(float));

				flexibleOptionIndex++;
			}

			// damping defaults to 0.0f
			if (jiggleData->pitchDamping != 0.0f)
			{
				is_flexible.options[flexibleOptionIndex].Init(&s_CommandJiggleBone_Option_PitchDamping);
				is_flexible.options[flexibleOptionIndex].SetRaw(&jiggleData->pitchDamping, 1u, sizeof(float));

				flexibleOptionIndex++;
			}

			if (!jiggleData->HasLengthConstraint())
			{
				// stiffness defaults to 100.0f
				if (jiggleData->alongStiffness != 100.0f)
				{
					is_flexible.options[flexibleOptionIndex].Init(&s_CommandJiggleBone_Option_AlongStiffness);
					is_flexible.options[flexibleOptionIndex].SetRaw(&jiggleData->alongStiffness, 1u, sizeof(float));

					flexibleOptionIndex++;
				}

				// damping defaults to 0.0f
				if (jiggleData->alongDamping != 0.0f)
				{
					is_flexible.options[flexibleOptionIndex].Init(&s_CommandJiggleBone_Option_AlongDamping);
					is_flexible.options[flexibleOptionIndex].SetRaw(&jiggleData->alongDamping, 1u, sizeof(float));

					flexibleOptionIndex++;
				}

				is_flexible.options[flexibleOptionIndex].Init(&s_CommandJiggleBone_Option_AllowLengthFlex);

				flexibleOptionIndex++;
			}

			flexibleOptionIndex = CommandJiggleBone_ParseCommonOptions(jiggleData, &is_flexible, flexibleOptionIndex);

			// truncate because we allow for the max number of jiggle parameters it could have, options are stored in the QCFile's buffer so this is okay
			is_flexible.TruncateOptions(flexibleOptionIndex);

			options[optionsIndex].Init(&s_CommandJiggleBone_Option_IsFlexible);
			options[optionsIndex].SavePtr(file, &is_flexible, 1u, sizeof(CommandOptionArray_t));

			optionsIndex++;
		}
		else if (jiggleData->IsRigid() && optionsIndex < usedOptions)
		{
			uint32_t rigidOptionIndex = 0u;

			CommandOptionArray_t is_rigid(file, jiggleData->CommonJiggleOptionCount());
			rigidOptionIndex = CommandJiggleBone_ParseCommonOptions(jiggleData, &is_rigid, rigidOptionIndex);

			// truncate because we allow for the max number of jiggle parameters it could have, options are stored in the QCFile's buffer so this is okay
			is_rigid.TruncateOptions(rigidOptionIndex);

			options[optionsIndex].Init(&s_CommandJiggleBone_Option_IsRigid);
			options[optionsIndex].SavePtr(file, &is_rigid, 1u, sizeof(CommandOptionArray_t));

			optionsIndex++;
		}

		// has_base_spring and is_boing
		if (jiggleData->HasBaseSpring() && optionsIndex < usedOptions)
		{			
			uint32_t springOptionIndex = 0u;

			CommandOptionArray_t has_spring_base(file, jiggleData->BaseSpringOptionCount());

			// stiffness defaults to 100.0f
			if (jiggleData->baseStiffness != 100.0f)
			{
				has_spring_base.options[springOptionIndex].Init(&s_CommandJiggleBone_Option_BaseStiffness);
				has_spring_base.options[springOptionIndex].SetRaw(&jiggleData->baseStiffness, 1u, sizeof(float));

				springOptionIndex++;
			}

			// damping defaults to 0.0f
			if (jiggleData->baseDamping != 0.0f)
			{
				has_spring_base.options[springOptionIndex].Init(&s_CommandJiggleBone_Option_BaseDamping);
				has_spring_base.options[springOptionIndex].SetRaw(&jiggleData->baseDamping, 1u, sizeof(float));

				springOptionIndex++;
			}

			// left
			// min defaults to -100.0f and max defaults to 100.0f
			if (jiggleData->baseMinLeft != -100.0f || jiggleData->baseMaxLeft != 100.0f)
			{
				const Vector2D constraint(jiggleData->baseMinLeft, jiggleData->baseMaxLeft);
				has_spring_base.options[springOptionIndex].Init(&s_CommandJiggleBone_Option_LeftConstraint);
				has_spring_base.options[springOptionIndex].SetRaw(&constraint, 1u, sizeof(Vector2D));

				springOptionIndex++;
			}

			// damping defaults to 0.0f
			if (jiggleData->baseLeftFriction != 0.0f)
			{
				has_spring_base.options[springOptionIndex].Init(&s_CommandJiggleBone_Option_LeftFriction);
				has_spring_base.options[springOptionIndex].SetRaw(&jiggleData->baseLeftFriction, 1u, sizeof(float));

				springOptionIndex++;
			}

			// up
			// min defaults to -100.0f and max defaults to 100.0f
			if (jiggleData->baseMinUp != -100.0f || jiggleData->baseMaxUp != 100.0f)
			{
				const Vector2D constraint(jiggleData->baseMinUp, jiggleData->baseMaxUp);
				has_spring_base.options[springOptionIndex].Init(&s_CommandJiggleBone_Option_UpConstraint);
				has_spring_base.options[springOptionIndex].SetRaw(&constraint, 1u, sizeof(Vector2D));

				springOptionIndex++;
			}

			// damping defaults to 0.0f
			if (jiggleData->baseUpFriction != 0.0f)
			{
				has_spring_base.options[springOptionIndex].Init(&s_CommandJiggleBone_Option_UpFriction);
				has_spring_base.options[springOptionIndex].SetRaw(&jiggleData->baseUpFriction, 1u, sizeof(float));

				springOptionIndex++;
			}

			// forward
			// min defaults to -100.0f and max defaults to 100.0f
			if (jiggleData->baseMinForward != -100.0f || jiggleData->baseMaxForward != 100.0f)
			{
				const Vector2D constraint(jiggleData->baseMinForward, jiggleData->baseMaxForward);
				has_spring_base.options[springOptionIndex].Init(&s_CommandJiggleBone_Option_ForwardConstraint);
				has_spring_base.options[springOptionIndex].SetRaw(&constraint, 1u, sizeof(Vector2D));

				springOptionIndex++;
			}

			// damping defaults to 0.0f
			if (jiggleData->baseForwardFriction != 0.0f)
			{
				has_spring_base.options[springOptionIndex].Init(&s_CommandJiggleBone_Option_ForwardFriction);
				has_spring_base.options[springOptionIndex].SetRaw(&jiggleData->baseForwardFriction, 1u, sizeof(float));

				springOptionIndex++;
			}

			// thiccness
			if (jiggleData->baseMass != 0.0f)
			{
				has_spring_base.options[springOptionIndex].Init(&s_CommandJiggleBone_Option_BaseMass);
				has_spring_base.options[springOptionIndex].SetRaw(&jiggleData->baseMass, 1u, sizeof(float));

				springOptionIndex++;
			}

			// truncate because we allow for the max number of jiggle parameters it could have, options are stored in the QCFile's buffer so this is okay
			has_spring_base.TruncateOptions(springOptionIndex);

			options[optionsIndex].Init(&s_CommandJiggleBone_Option_BaseSpring);
			options[optionsIndex].SavePtr(file, &has_spring_base, 1u, sizeof(CommandOptionArray_t));

			optionsIndex++;
		}
		else if (jiggleData->IsBoing() && optionsIndex < usedOptions)
		{
			// 0x0045CC50 studiomdl.exe (teamfortress2)

			uint32_t boingOptionIndex = 0u;

			CommandOptionArray_t is_boing(file, jiggleData->BaseSpringOptionCount());

			// impact_speed defaults to 100.0f
			if (jiggleData->boingImpactSpeed != 100.0f)
			{
				is_boing.options[boingOptionIndex].Init(&s_CommandJiggleBone_Option_BoingImpactSpeed);
				is_boing.options[boingOptionIndex].SetRaw(&jiggleData->boingImpactSpeed, 1u, sizeof(float));

				boingOptionIndex++;
			}

			// impact_angle defaults to 0.70709997
			if (jiggleData->boingImpactAngle != 0.70709997f)
			{
				const float boingImpactAngle = RAD2DEG(acosf(jiggleData->boingImpactAngle));
				is_boing.options[boingOptionIndex].Init(&s_CommandJiggleBone_Option_BoingImpactSpeed);
				is_boing.options[boingOptionIndex].SetRaw(&boingImpactAngle, 1u, sizeof(float));

				boingOptionIndex++;
			}

			// damping_rate defaults to 0.25
			if (jiggleData->boingDampingRate != 0.25f)
			{
				is_boing.options[boingOptionIndex].Init(&s_CommandJiggleBone_Option_BoingDampingRate);
				is_boing.options[boingOptionIndex].SetRaw(&jiggleData->boingDampingRate, 1u, sizeof(float));

				boingOptionIndex++;
			}

			// frequency defaults to 30.0
			if (jiggleData->boingFrequency != 30.0f)
			{
				is_boing.options[boingOptionIndex].Init(&s_CommandJiggleBone_Option_BoingFrequency);
				is_boing.options[boingOptionIndex].SetRaw(&jiggleData->boingFrequency, 1u, sizeof(float));

				boingOptionIndex++;
			}

			// amplitude defaults to 0.34999999
			if (jiggleData->boingAmplitude != 0.34999999f)
			{
				is_boing.options[boingOptionIndex].Init(&s_CommandJiggleBone_Option_BoingAmplitude);
				is_boing.options[boingOptionIndex].SetRaw(&jiggleData->boingAmplitude, 1u, sizeof(float));

				boingOptionIndex++;
			}

			// truncate because we allow for the max number of jiggle parameters it could have, options are stored in the QCFile's buffer so this is okay
			is_boing.TruncateOptions(boingOptionIndex);

			options[optionsIndex].Init(&s_CommandJiggleBone_Option_IsBoing);
			options[optionsIndex].SavePtr(file, &is_boing, 1u, sizeof(CommandOptionArray_t));

			optionsIndex++;

			optionsIndex++;
		}

		*numOptions = usedOptions;

		return options;
	}

	CommandOption_t* CommandAttachment_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		UNUSED(count);
		UNUSED(store);

		const AttachmentData_t* const attachmentData = reinterpret_cast<const AttachmentData_t* const>(data);

		// name, bone, pos, type (+ rotation if rotate type)
		const uint32_t usedOptions = 4 + (!attachmentData->WorldAlign() ? 1 : 0);
		CommandOption_t* options = new CommandOption_t[usedOptions];

		// set name
		options[0].Init(&s_CommandGeneric_Option_Name);
		options[0].SetPtr(attachmentData->name);

		// set bone
		options[1].Init(&s_CommandGeneric_Option_Bone);
		options[1].SetPtr(attachmentData->localbone);

		// do these need to be altered for static prop ? studiomdl seems to recalc them if the model is a static prop
		Vector pos;
		QAngle rot;
		MatrixAngles(*attachmentData->localmatrix, rot, pos);

		// set pos
		options[2].Init(&s_CommandGeneric_Option_Pos);
		options[2].SavePtr(file, &pos, 3, sizeof(Vector));

		// set type
		// todo: absolute and maybe rigid detecting? absolute probably possible, rigid eeehhh
		const AttachmentData_t::Type_t type = attachmentData->WorldAlign() ? AttachmentData_t::Type_t::ATTACH_WORLD_ALIGN : AttachmentData_t::Type_t::ATTACH_ROTATE;

		options[3].Init(&s_CommandAttachment_Option_Type);
		options[3].SetPtr(s_AttachmentTypeNames[type]);

		if (type == AttachmentData_t::Type_t::ATTACH_ROTATE && usedOptions >= 5)
		{
			// valve devwiki states that these are "weird", and crowbar's parsing of this reflects that. gets adjusted a lot in studiomdl too.
			// however, the output of rot seems to match crowbar's shifting (rot.y, -rot.z, -rot.x), perhaps zeq's MatrixAngles function is off?

			options[4].Init(&s_CommandGeneric_Option_Rot);
			options[4].SavePtr(file, &rot, 3, sizeof(QAngle));
		}

		*numOptions = usedOptions;

		return options;
	}

	// models
	CommandOption_t* CommandBodyGroup_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		UNUSED(count);

		const BodyGroupData_t* const bodyGroupData = reinterpret_cast<const BodyGroupData_t* const>(data);

		const uint32_t usedOptions = bodyGroupData->numParts + 1;
		CommandOption_t* options = new CommandOption_t[usedOptions];

		options[0].Init(&s_CommandGeneric_Option_Name);

		if (store)
			options[0].SaveStr(file, bodyGroupData->name);
		else
			options[0].SetPtr(bodyGroupData->name);

		uint32_t optionsIndex = 1;

		for (uint32_t i = 0; i < bodyGroupData->numParts; i++)
		{
			if (optionsIndex + i >= usedOptions)
				break;

			// model uses 'blank' option
			if (!bodyGroupData->parts[i])
			{
				options[optionsIndex + i].Init(&s_CommandBodyGroup_Option_Blank);
				continue;
			}

			// model uses 'studio' option
			options[optionsIndex + i].Init(&s_CommandBodyGroup_Option_Studio);

			if (store)
			{
				options[optionsIndex + i].SaveStr(file, bodyGroupData->parts[i]);
				continue;
			}

			options[optionsIndex + i].SetPtr(bodyGroupData->parts[i]);
		}

		*numOptions = usedOptions;

		return options;
	}

	CommandOption_t* CommandLOD_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		UNUSED(count);

		const LodData_t* const lodGroupData = reinterpret_cast<const LodData_t* const>(data);

		const uint32_t usedOptions = lodGroupData->numModels + lodGroupData->numBoneUsed + lodGroupData->UseThreshold() + lodGroupData->useShadowLODMaterials + lodGroupData->numReplacementMaterials;
		CommandOption_t* options = new CommandOption_t[usedOptions];

		uint32_t optionsIndex = 0;

		if (lodGroupData->UseThreshold())
		{
			options[0].Init(&s_CommandLOD_Option_Threshold);
			options[0].SetRaw(&lodGroupData->threshold, 1, sizeof(float));

			optionsIndex++;
		}

		// parse potential model options
		for (uint32_t i = 0; i < lodGroupData->numModels; i++)
		{
			if (optionsIndex + i >= usedOptions)
				break;

			const char* const baseName = lodGroupData->modelBaseNames[i];
			const char* const replaceName = lodGroupData->modelReplaceNames[i];

			// model is ignored
			if (!baseName)
			{
				continue;
			}

			// model is removed
			if (!replaceName)
			{
				options[optionsIndex].Init(&s_CommandLOD_Option_RemoveModel);

				if (store)
					options[optionsIndex].SaveStr(file, baseName);
				else
					options[optionsIndex].SetPtr(baseName);

				optionsIndex++;
				continue;
			}

			// model is replaced
			options[optionsIndex].Init(&s_CommandLOD_Option_ReplaceModel);

			if (store)
			{
				CommandOptionPair_t replaceModel(file, baseName, replaceName);
				options[optionsIndex].SavePtr(file, &replaceModel, 1, sizeof(CommandOptionPair_t));
			}
			else
			{
				CommandOptionPair_t replaceModel(baseName, replaceName);
				options[optionsIndex].SavePtr(file, &replaceModel, 1, sizeof(CommandOptionPair_t));
			}

			optionsIndex++;
		}

		// parse potential bone options
		for (uint32_t slot = 0; slot < lodGroupData->numBoneSlots; slot++)
		{
			if (optionsIndex >= usedOptions)
				break;

			const char* const baseName = lodGroupData->boneBaseNames[slot];
			const char* const replaceName = lodGroupData->boneReplaceNames[slot];

			// bone is ignored
			if (!baseName || !replaceName)
				continue;

			// bone is replaced
			options[optionsIndex].Init(&s_CommandLOD_Option_ReplaceBone);

			if (store)
			{
				CommandOptionPair_t replaceBone(file, baseName, replaceName);
				options[optionsIndex].SavePtr(file, &replaceBone, 1, sizeof(CommandOptionPair_t));
			}
			else
			{
				CommandOptionPair_t replaceBone(baseName, replaceName);
				options[optionsIndex].SavePtr(file, &replaceBone, 1, sizeof(CommandOptionPair_t));
			}

			optionsIndex++;
		}

		// do replacement materials, if there are any
		for (uint32_t i = 0; i < lodGroupData->numReplacementMaterials; i++)
		{
			if (optionsIndex >= usedOptions)
				break;

			const char* const baseName = lodGroupData->materialBaseNames[i];
			const char* const replaceName = lodGroupData->materialReplaceNames[i];

			// bone is replaced
			options[optionsIndex].Init(&s_CommandLOD_Option_ReplaceMaterial);

			if (store)
			{
				CommandOptionPair_t replaceMaterial(file, baseName, replaceName);
				options[optionsIndex].SavePtr(file, &replaceMaterial, 1, sizeof(CommandOptionPair_t));
			}
			else
			{
				CommandOptionPair_t replaceMaterial(baseName, replaceName);
				options[optionsIndex].SavePtr(file, &replaceMaterial, 1, sizeof(CommandOptionPair_t));
			}

			optionsIndex++;
		}

		// to be tested
		if (lodGroupData->useShadowLODMaterials && optionsIndex < usedOptions)
		{
			options[optionsIndex].Init(&s_CommandLOD_Option_ShadowLODMaterials);
			optionsIndex++;
		}

		assertm(usedOptions >= optionsIndex, "exceeded max usable options");
		*numOptions = optionsIndex;

		return options;
	}

	// animations
	CommandOption_t* CommandPoseParam_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		UNUSED(count);

		const PoseParamData_t* const poseParamData = reinterpret_cast<const PoseParamData_t* const>(data);

		const uint32_t usedOptions = 3u + poseParamData->IsLooped();
		CommandOption_t* options = new CommandOption_t[usedOptions];

		options[0].Init(&s_CommandGeneric_Option_Name);

		if (store)
			options[0].SaveStr(file, poseParamData->name);
		else
			options[0].SetPtr(poseParamData->name);

		if (usedOptions >= 3)
		{
			options[1].Init(&s_CommandPoseParam_Option_Start);
			options[1].SetRaw(&poseParamData->start, 1, sizeof(float));

			options[2].Init(&s_CommandPoseParam_Option_End);
			options[2].SetRaw(&poseParamData->end, 1, sizeof(float));
		}

		if (poseParamData->IsLooped() && usedOptions >= 4)
		{
			if (poseParamData->IsWrapped())
			{
				options[3].Init(&s_CommandPoseParam_Option_Wrap);
			}
			else
			{
				options[3].Init(&s_CommandPoseParam_Option_Loop);
				options[3].SetRaw(&poseParamData->loop, 1, sizeof(float));
			}
		}

		*numOptions = usedOptions;

		return options;
	}

	CommandOption_t* CommandIKChain_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		UNUSED(count);

		const IKChainData_t* const ikChainData = reinterpret_cast<const IKChainData_t* const>(data);

		const bool hasKnee = ikChainData->UseKnee();

		const uint32_t usedOptions = 3u + hasKnee; // name, bone, [knee], [angle]
		CommandOption_t* options = new CommandOption_t[usedOptions];

		options[0].Init(&s_CommandGeneric_Option_Name);

		if (store)
			options[0].SaveStr(file, ikChainData->name);
		else
			options[0].SetPtr(ikChainData->name);

		uint32_t optionsIndex = 1u;

		// bone
		if (usedOptions > optionsIndex)
		{
			options[optionsIndex].Init(&s_CommandGeneric_Option_Bone);

			if (store)
				options[optionsIndex].SaveStr(file, ikChainData->bone);
			else
				options[optionsIndex].SetPtr(ikChainData->bone);

			optionsIndex++;
		}

		// knee
		if (hasKnee && usedOptions > optionsIndex)
		{
			options[optionsIndex].Init(&s_CommandIKChain_Option_Knee);

			if (store)
				options[optionsIndex].SavePtr(file, ikChainData->knee, 3u, sizeof(Vector));
			else
				options[optionsIndex].SetPtr(ikChainData->knee, 3u);

			optionsIndex++;
		}

		// weird angle
		if (usedOptions > optionsIndex)
		{
			const float angleQuirky = RAD2DEG(acosf(ikChainData->angleUnknown));
			options[optionsIndex].Init(&s_CommandIKChain_Option_Angle);
			options[optionsIndex].SetRaw(&angleQuirky, 1, sizeof(float));

			optionsIndex++;
		}

		*numOptions = usedOptions;

		return options;
	}

	void CommandIKAutoPlayLock_ParseOptions(QCFile* const file, CommandOption_t* const options, const uint32_t usedOptions, const bool store, const IKLockData_t* const ikLockData)
	{
		UNUSED(usedOptions);

		options[0].Init(&s_CommandGeneric_Option_Name);

		if (store)
		{
			options[0].SaveStr(file, ikLockData->chain);
		}
		else
		{
			options[0].SetPtr(ikLockData->chain);
		}

		options[1].Init(&s_CommandIKAutoPlayLock_Option_LockPosition);
		options[1].SetRaw(&ikLockData->flPosWeight, 1, sizeof(float));

		options[2].Init(&s_CommandIKAutoPlayLock_Option_InheritRotation);
		options[2].SetRaw(&ikLockData->flLocalQWeight, 1, sizeof(float));
	}

	CommandOption_t* CommandIKAutoPlayLock_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		UNUSED(count);

		const IKLockData_t* const ikLockData = reinterpret_cast<const IKLockData_t* const>(data);

		const uint32_t usedOptions = 3u;
		CommandOption_t* options = new CommandOption_t[usedOptions];

		CommandIKAutoPlayLock_ParseOptions(file, options, usedOptions, store, ikLockData);

		*numOptions = usedOptions;

		return options;
	}

	CommandOption_t* CommandSectionFrames_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		UNUSED(file);
		UNUSED(count);
		UNUSED(store);

		const SectionFrameData_t* const sectionFrameData = reinterpret_cast<const SectionFrameData_t* const>(data);

		const uint32_t usedOptions = 2u;
		CommandOption_t* options = new CommandOption_t[usedOptions];

		options[0].Init(&s_CommandSectionFrames_Option_FrameCount);
		options[0].SetRaw(&sectionFrameData->sectionFrameCount, 1, sizeof(int));

		options[1].Init(&s_CommandSectionFrames_Option_MinFrames);
		options[1].SetRaw(&sectionFrameData->minFramesForSections, 1, sizeof(int));

		*numOptions = usedOptions;

		return options;
	}

	constexpr CommandOptionDesc_t s_CommandAnimBlockSize_Option_Size(QC_OPT_INT, "blocksize", s_CommandOptionFormatDefault);
	constexpr CommandOptionDesc_t s_CommandAnimBlockSize_Option_NoStall(QC_OPT_NONE, "nostall", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandAnimBlockSize_Option_HighRes(QC_OPT_NONE, "highres", s_CommandOptionFormatNameParentLine, s_QCVersion_L4D, s_QCVersion_R1);
	constexpr CommandOptionDesc_t s_CommandAnimBlockSize_Option_LowRes(QC_OPT_NONE, "lowres", s_CommandOptionFormatNameParentLine, s_QCVersion_L4D, s_QCVersion_R1);
	constexpr CommandOptionDesc_t s_CommandAnimBlockSize_Option_NumFrames(QC_OPT_INT, "numframes", s_CommandOptionFormatNameParentLine, s_QCVersion_L4D2, s_QCVersion_R1);
	constexpr CommandOptionDesc_t s_CommandAnimBlockSize_Option_CacheHighRes(QC_OPT_NONE, "cachehighres", s_CommandOptionFormatNameParentLine, s_QCVersion_L4D2, s_QCVersion_R1);

	CommandOption_t* CommandAnimBlockSize_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		UNUSED(file);
		UNUSED(count);
		UNUSED(store);

		const AnimBlockData_t* const animBlockData = reinterpret_cast<const AnimBlockData_t* const>(data);

		// align block size to nice numbers
		int blockSizeInKB = (animBlockData->maxBlockSizeInBytes >> 10);
		if (blockSizeInKB > 32)
		{
			blockSizeInKB = 64;
		}
		else if (blockSizeInKB > 16)
		{
			blockSizeInKB = 32;
		}
		else
		{
			blockSizeInKB = 16;
		}

		const uint32_t usedOptions = animBlockData->EstimateOptionCount();
		CommandOption_t* options = new CommandOption_t[usedOptions];

		uint32_t optionsIndex = 0u;

		options[optionsIndex].Init(&s_CommandAnimBlockSize_Option_Size);
		options[optionsIndex].SetRaw(&blockSizeInKB, 1, sizeof(int));
		optionsIndex++;

		if (animBlockData->noStall)
		{
			options[optionsIndex].Init(&s_CommandAnimBlockSize_Option_NoStall);
			optionsIndex++;
		}

		if (animBlockData->highRes)
		{
			options[optionsIndex].Init(&s_CommandAnimBlockSize_Option_HighRes);
			optionsIndex++;
		}

		if (animBlockData->lowRes)
		{
			options[optionsIndex].Init(&s_CommandAnimBlockSize_Option_LowRes);
			optionsIndex++;
		}

		if (animBlockData->UseNumFrames())
		{
			options[optionsIndex].Init(&s_CommandAnimBlockSize_Option_NumFrames);
			options[optionsIndex].SetRaw(&animBlockData->numFrames, 1, sizeof(int));
			optionsIndex++;
		}

		if (animBlockData->cacheHighRes)
		{
			options[optionsIndex].Init(&s_CommandAnimBlockSize_Option_CacheHighRes);
			optionsIndex++;
		}

		*numOptions = usedOptions;

		return options;
	}

	CommandOption_t* CommandWeightList_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		UNUSED(file);
		UNUSED(count);
		UNUSED(store);

		const WeightListData_t* const weightListData = reinterpret_cast<const WeightListData_t* const>(data);

		const uint32_t usedOptions = weightListData->numWeights + (weightListData->isDefault ? 0u : 1u);
		CommandOption_t* options = new CommandOption_t[usedOptions];

		uint32_t optionsIndex = 0u;

		if (weightListData->isDefault == false)
		{
			options[optionsIndex].Init(&s_CommandGeneric_Option_Name);
			options[optionsIndex].SetPtr(weightListData->name);
			optionsIndex++;
		}

		for (int i = 0; i < weightListData->numWeights; i++)
		{
			const CommandOptionPair_t weightData(weightListData->bones[i], weightListData->weights[i]);

			options[optionsIndex].Init(&s_CommandWeightList_Option_Weight);
			options[optionsIndex].SavePtr(file, &weightData, 1u, sizeof(CommandOptionPair_t));
			optionsIndex++;
		}

		*numOptions = usedOptions;

		return options;
	}

	// general
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_Filepath(QC_OPT_STRING, "filepath");
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_FPS(QC_OPT_FLOAT, "fps", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_Loop(QC_OPT_NONE, "loop", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_NoForceLoop(QC_OPT_NONE, "noforceloop", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_Snap(QC_OPT_NONE, "snap", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_Post(QC_OPT_NONE, "post", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_NoAnim(QC_OPT_NONE, "noanimation", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_NoAnimKeepDuration(QC_OPT_NONE, "noanim_keepduration", s_CommandOptionFormatNameNewLine, s_QCVersion_CSGO, s_QCVersion_R5_RETAIL); // not in p2, but in csgo, and seemingly in r1
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_NoAutoIK(QC_OPT_NONE, "noautoik", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_AutoIK(QC_OPT_NONE, "autoik", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_Subtract(QC_OPT_STRING, "subtract", s_CommandOptionFormatNameNewLine); // move back to s_CommandOptionFormatNameParentLine when supported, being on end of line as comment fails compile
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_WeightList(QC_OPT_STRING, "weightlist", s_CommandOptionFormatNameNewLine);

	// motions
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_MotionAxis(QC_OPT_STRING, "motion_axis", s_CommandOptionFormatDefault);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_WalkFrame_EndFrame(QC_OPT_INT, "endframe", s_CommandOptionFormatDefault);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_WalkFrame(QC_OPT_ARRAY, "walkframe", s_CommandOptionFormatNameNewLine);

	// ik rule
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_IKRule(QC_OPT_ARRAY, "ikrule", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_IKFixup(QC_OPT_ARRAY, "ikfixup", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_IKRule_Chain(QC_OPT_STRING, "chain", s_CommandOptionFormatDefault);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_IKRule_Touch(QC_OPT_STRING, "touch", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_IKRule_Footstep(QC_OPT_NONE, "footstep", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_IKRule_Attachment(QC_OPT_STRING, "attachment", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_IKRule_Release(QC_OPT_NONE, "release", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_IKRule_Unlatch(QC_OPT_NONE, "unlatch", s_CommandOptionFormatNameParentLine);

	constexpr CommandOptionDesc_t s_CommandAnimation_Option_IKRule_Height(QC_OPT_FLOAT, "height", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_IKRule_Target(QC_OPT_INT, "target", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_IKRule_Range(QC_OPT_INT, "range", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_IKRule_Floor(QC_OPT_FLOAT, "floor", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_IKRule_Radius(QC_OPT_FLOAT, "radius", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_IKRule_Contact(QC_OPT_FLOAT, "contact", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_IKRule_FakeOrigin(QC_OPT_FLOAT, "fakeorigin", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_IKRule_FakeRotate(QC_OPT_FLOAT, "fakerotate", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_IKRule_Bone(QC_OPT_STRING, "bone", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_IKRule_EndHeight(QC_OPT_FLOAT, "endheight", s_CommandOptionFormatNameParentLine, s_QCVersion_R1, s_QCVersion_R5_RETAIL);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_IKRule_UseSeq(QC_OPT_NONE, "usesequence", s_CommandOptionFormatNameParentLine);

	// respawn
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_RootMotion(QC_OPT_NONE, "rm", s_CommandOptionFormatNameParentLine, s_QCVersion_R1, s_QCVersion_R5_RETAIL);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_SuppressGesture(QC_OPT_NONE, "suppgest", s_CommandOptionFormatNameParentLine, s_QCVersion_R5_080, s_QCVersion_R5_RETAIL);
	constexpr CommandOptionDesc_t s_CommandAnimation_Option_DefaultPose(QC_OPT_NONE, "defaultpose", s_CommandOptionFormatNameNewLine, s_QCVersion_R5_080, s_QCVersion_R5_RETAIL);

	// seqeunce
	constexpr CommandOptionDesc_t s_CommandSequence_Option_Activity(QC_OPT_PAIR, "activity", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_Delta(QC_OPT_NONE, "delta", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_AutoPlay(QC_OPT_NONE, "autoplay", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_RealTime(QC_OPT_NONE, "realtime", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_Hidden(QC_OPT_NONE, "hidden", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_WorldSpace(QC_OPT_NONE, "worldspace", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_FadeIn(QC_OPT_FLOAT, "fadein", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_FadeOut(QC_OPT_FLOAT, "fadeout", s_CommandOptionFormatNameNewLine);

	constexpr CommandOptionDesc_t s_CommandSequence_Option_Event(QC_OPT_ARRAY, "event", QC_FMT_ARRAY | QC_FMT_NEWLINE);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_Event_Key(QC_OPT_NONE, "event", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_Event_ID(QC_OPT_INT, "event_id", s_CommandOptionFormatDefault);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_Event_Frame(QC_OPT_INT, "event_frame", s_CommandOptionFormatDefault);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_Event_Options(QC_OPT_STRING, "event_options", s_CommandOptionFormatDefault);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_Event_Unk(QC_OPT_FLOAT, "event_unk", s_CommandOptionFormatDefault, s_QCVersion_R5_120, s_QCVersion_R5_RETAIL); // should be 12.3, todo support this

	constexpr CommandOptionDesc_t s_CommandSequence_Option_Blends(QC_OPT_STRING, "blends", QC_FMT_NEWLINE);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_BlendWidth(QC_OPT_INT, "blendwidth", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_Blend(QC_OPT_PAIR, "blend", s_CommandOptionFormatNameNewLine);

	constexpr CommandOptionDesc_t s_CommandSequence_Option_Node(QC_OPT_STRING, "node", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_Transition(QC_OPT_PAIR, "transition", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_RTransition(QC_OPT_PAIR, "rtransition", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_EntryNode(QC_OPT_STRING, "entrynode", s_CommandOptionFormatNameNewLine, s_QCVersion_R5_080, s_QCVersion_R5_RETAIL);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_ExitNode(QC_OPT_STRING, "exitnode", s_CommandOptionFormatNameNewLine, s_QCVersion_R5_080, s_QCVersion_R5_RETAIL);

	constexpr CommandOptionDesc_t s_CommandSequence_Option_ExitPhase(QC_OPT_FLOAT, "exitphase", s_CommandOptionFormatNameNewLine);

	constexpr CommandOptionDesc_t s_CommandSequence_Option_AddLayer(QC_OPT_ARRAY, "addlayer", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_BlendLayer(QC_OPT_ARRAY, "blendlayer", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_Layer_Local(QC_OPT_NONE, "local", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_Layer_Pose(QC_OPT_STRING, "poseparameter", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_Layer_NoBlend(QC_OPT_NONE, "noblend", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_Layer_Spline(QC_OPT_NONE, "spline", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_Layer_XFade(QC_OPT_NONE, "xfade", s_CommandOptionFormatNameParentLine);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_Layer_RealTime(QC_OPT_NONE, "realtime", s_CommandOptionFormatNameParentLine, s_QCVersion_R1, s_QCVersion_R5_RETAIL);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_Layer_Unk2000(QC_OPT_NONE, "unk_2000", s_CommandOptionFormatNameParentLine, s_QCVersion_R1, s_QCVersion_R5_RETAIL);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_Layer_Range(QC_OPT_INT, "range", s_CommandOptionFormatDefault);

	constexpr CommandOptionDesc_t s_CommandSequence_Option_IKLock(QC_OPT_ARRAY, "iklock", s_CommandOptionFormatNameNewLine);

	constexpr CommandOptionDesc_t s_CommandSequence_Option_PoseCycle(QC_OPT_STRING, "posecycle", s_CommandOptionFormatNameNewLine);

	constexpr CommandOptionDesc_t s_CommandSequence_Option_KeyValues(QC_OPT_STRING, "keyvalues", s_CommandOptionFormatNameNewLine);

	constexpr CommandOptionDesc_t s_CommandSequence_Option_ActMod(QC_OPT_ARRAY, "activitymodifier", s_CommandOptionFormatNameNewLine, s_QCVersion_P2, s_QCVersion_MAX);
	constexpr CommandOptionDesc_t s_CommandSequence_Option_ActMod_Negate(QC_OPT_NONE, "negate", s_CommandOptionFormatNameParentLine, s_QCVersion_R1, s_QCVersion_R5_RETAIL);

	constexpr CommandOptionDesc_t s_CommandSequence_Option_IKResetMask(QC_OPT_STRING, "ikresetmask", s_CommandOptionFormatNameParentLine, s_QCVersion_R1, s_QCVersion_R5_RETAIL);

	void CommandAnimation_ParseOption_Motion(QCFile* const file, CommandOption_t* const options, uint32_t& optionsIndex, const AnimationData_t* const animData)
	{
		// motion spans the entire animation
		// undesirable results ?
		/*if (animData->numMotions == 1)
		{
			const char* motionAxis[AnimationMotion_t::MOTION_OPT_COUNT]{};
			const AnimationMotion_t* const motion = animData->motions;

			animData->BuildAxisForMotion(0, motionAxis);
			const int numAxis = motion->AxisCount();

			options[optionsIndex].Init(&s_CommandAnimation_Option_MotionAxis);
			options[optionsIndex].SavePtr(file, motionAxis, numAxis, sizeof(char*) * numAxis);
			optionsIndex++;

			return;
		}*/

		const char* motionAxis[AnimationMotion_t::MOTION_OPT_COUNT]{};

		for (int i = 0; i < animData->numMotions; i++)
		{
			const AnimationMotion_t* const motion = animData->motions + i;

			animData->BuildAxisForMotion(i, motionAxis);
			const int numAxis = motion->AxisCount();

			CommandOptionArray_t walkframeArray(file, 2u);

			walkframeArray.options[0].Init(&s_CommandAnimation_Option_WalkFrame_EndFrame);
			walkframeArray.options[0].SetRaw(&motion->endframe, 1u, sizeof(int));

			walkframeArray.options[1].Init(&s_CommandAnimation_Option_MotionAxis);
			walkframeArray.options[1].SavePtr(file, motionAxis, numAxis, sizeof(char*) * numAxis);

			options[optionsIndex].Init(&s_CommandAnimation_Option_WalkFrame);
			options[optionsIndex].SavePtr(file, &walkframeArray, 1u, sizeof(CommandOptionArray_t));
			optionsIndex++;
		}
	}

	void CommandAnimation_ParseOption_IKRule(QCFile* const file, const AnimationData_t* const animData, const AnimationIKRule_t* const ikRule, CommandOptionArray_t* const ikRuleArray)
	{
		CommandOption_t* const options = ikRuleArray->options;
		uint32_t optionsIndex = 0;

		options[optionsIndex].Init(&s_CommandAnimation_Option_IKRule_Chain);
		options[optionsIndex].SetPtr(ikRule->chainname);
		optionsIndex++;

		switch (ikRule->type)
		{
		case IK_SELF:
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_IKRule_Footstep);
			options[optionsIndex].SetPtr(ikRule->bonename);
			optionsIndex++;

			break;
		}
		case IK_GROUND:
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_IKRule_Touch);
			optionsIndex++;

			break;
		}
		case IK_ATTACHMENT:
		{
			assertm(ikRule->attachment, "ik rule was attachment without attachment name");

			options[optionsIndex].Init(&s_CommandAnimation_Option_IKRule_Attachment);
			options[optionsIndex].SetPtr(ikRule->attachment);
			optionsIndex++;

			break;
		}
		case IK_RELEASE:
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_IKRule_Release);
			optionsIndex++;

			break;
		}
		case IK_UNLATCH:
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_IKRule_Unlatch);
			optionsIndex++;

			break;
		}
		}

		if (ikRule->height != 0.0f)
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_IKRule_Height);
			options[optionsIndex].SetRaw(&ikRule->height, 1u, sizeof(float));
			optionsIndex++;
		}

		if (ikRule->floor != 0.0f)
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_IKRule_Floor);
			options[optionsIndex].SetRaw(&ikRule->floor, 1u, sizeof(float));
			optionsIndex++;
		}

		if (ikRule->radius != 0.0f)
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_IKRule_Radius);
			options[optionsIndex].SetRaw(&ikRule->radius, 1u, sizeof(float));
			optionsIndex++;
		}

		if (ikRule->endHeight != 0.0f)
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_IKRule_EndHeight);
			options[optionsIndex].SetRaw(&ikRule->endHeight, 1u, sizeof(float));
			optionsIndex++;
		}

		if (ikRule->slot != ikRule->chain)
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_IKRule_Target);
			options[optionsIndex].SetRaw(&ikRule->slot, 1u, sizeof(int));
			optionsIndex++;
		}

		if (ikRule->start != 0.0f || ikRule->peak != 0.0f || ikRule->tail != 1.0f || ikRule->end != 1.0f)
		{
			int range[4]{};
			const float numFrames = static_cast<float>(animData->numFrames - 1);

			range[0] = static_cast<int>(numFrames * ikRule->start);
			range[1] = static_cast<int>(numFrames * ikRule->peak);
			range[2] = static_cast<int>(numFrames * ikRule->tail);
			range[3] = static_cast<int>(numFrames * ikRule->end);

			options[optionsIndex].Init(&s_CommandAnimation_Option_IKRule_Range);
			options[optionsIndex].SavePtr(file, range, 4u, sizeof(range));
			optionsIndex++;
		}

		if (ikRule->bone == -1)
		{
			if (ikRule->pos->x != 0.0f || ikRule->pos->y != 0.0f || ikRule->pos->z != 0.0f)
			{
				options[optionsIndex].Init(&s_CommandAnimation_Option_IKRule_FakeOrigin);
				options[optionsIndex].SetPtr(ikRule->pos, 3u);
				optionsIndex++;
			}

			if (ikRule->q->x != 0.0f || ikRule->q->y != 0.0f || ikRule->q->z != 0.0f || ikRule->q->w != 0.0f)
			{
				QAngle angles(*ikRule->q);

				options[optionsIndex].Init(&s_CommandAnimation_Option_IKRule_FakeRotate);
				options[optionsIndex].SavePtr(file, &angles, 3u, sizeof(QAngle));
				optionsIndex++;
			}
		}

		if (ikRule->bone > 0 && ikRule->type != IK_SELF)
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_IKRule_Bone);
			options[optionsIndex].SetPtr(ikRule->bonename);
			optionsIndex++;
		}

		// todo 'usesequence'

		ikRuleArray->TruncateOptions(optionsIndex);
	}

	void CommandAnimation_ParseOption_IKRule(QCFile* const file, CommandOption_t* const options, uint32_t& optionsIndex, const AnimationData_t* const animData)
	{
		for (int i = 0; i < animData->numIKRules; i++)
		{
			const AnimationIKRule_t* const ikRule = animData->ikRules + i;
			CommandOptionArray_t ikRuleArray(file, 16u);

			CommandAnimation_ParseOption_IKRule(file, animData, ikRule, &ikRuleArray);

			if (ikRule->isFixup)
			{
				options[optionsIndex].Init(&s_CommandAnimation_Option_IKFixup);
			}
			else
			{
				options[optionsIndex].Init(&s_CommandAnimation_Option_IKRule);
			}

			options[optionsIndex].SavePtr(file, &ikRuleArray, 1u, sizeof(CommandOptionArray_t));
			optionsIndex++;
		}
	}

	void CommandAnimation_ParseOption_Simple(QCFile* const file, CommandOption_t* const options, uint32_t& optionsIndex, const AnimationData_t* const animData, const bool forceNoParentLine)
	{
		UNUSED(file);

		const CommandFormat_t format = forceNoParentLine ? s_CommandOptionFormatNameNewLine : s_CommandOptionFormatNameParentLine;
		const AnimationOptions_t animOptions = animData->options;

		if (animOptions.GetUsedOptions() == 0u)
		{
			return;
		}

		if (animOptions.UseFPS())
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_FPS);
			options[optionsIndex].SetRaw(&animData->fps, 1, sizeof(float));
			optionsIndex++;
		}

		if (animOptions.IsLooping())
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_Loop);
			options[optionsIndex].SetFormat(format);
			optionsIndex++;
		}

		if (animOptions.HasSnap())
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_Snap);
			options[optionsIndex].SetFormat(format);
			optionsIndex++;
		}

		if (animOptions.IsDelta())
		{
			if (animOptions.features != AnimationOptions_t::ANIM_OPT_FEATURE_SEQ)
			{
				options[optionsIndex].Init(&s_CommandAnimation_Option_Subtract);
				options[optionsIndex].SetPtr(animData->subtractPath);
				options[optionsIndex].data.format |= QC_FMT_COMMENT; // temp
				optionsIndex++;
			}

			if (animOptions.features != AnimationOptions_t::ANIM_OPT_FEATURE_ANIM)
			{
				options[optionsIndex].Init(&s_CommandSequence_Option_Delta);
				optionsIndex++;
			}
		}

		if (animOptions.IsAutoPlay())
		{
			assertm(animOptions.features != AnimationOptions_t::ANIM_OPT_FEATURE_ANIM, "autoplay on animation?");

			options[optionsIndex].Init(&s_CommandSequence_Option_AutoPlay);
			optionsIndex++;
		}

		if (animOptions.HasPost())
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_Post);
			options[optionsIndex].SetFormat(format);
			optionsIndex++;
		}

		if (animOptions.HasNoAnim())
		{
			if (animData->numFrames > 1)
			{
				options[optionsIndex].Init(&s_CommandAnimation_Option_NoAnimKeepDuration);
			}
			else
			{
				options[optionsIndex].Init(&s_CommandAnimation_Option_NoAnim);
			}

			optionsIndex++;
		}

		if (animOptions.IsRealTime())
		{
			assertm(animOptions.features != AnimationOptions_t::ANIM_OPT_FEATURE_ANIM, "realtime on animation?");

			options[optionsIndex].Init(&s_CommandSequence_Option_RealTime);
			optionsIndex++;
		}

		if (animOptions.IsHidden())
		{
			assertm(animOptions.features != AnimationOptions_t::ANIM_OPT_FEATURE_ANIM, "hidden on animation?");

			options[optionsIndex].Init(&s_CommandSequence_Option_Hidden);
			optionsIndex++;
		}

		if (animOptions.IsWorldSpace())
		{
			assertm(animOptions.features != AnimationOptions_t::ANIM_OPT_FEATURE_ANIM, "worldspace on animation?");

			options[optionsIndex].Init(&s_CommandSequence_Option_WorldSpace);
			optionsIndex++;
		}

		if (animOptions.HasNoForceLoop())
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_NoForceLoop);
			options[optionsIndex].SetFormat(format);
			optionsIndex++;
		}

		if (animOptions.UseNoAutoIK())
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_NoAutoIK);
			optionsIndex++;
		}

		if (animOptions.UseAutoIK())
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_AutoIK);
			optionsIndex++;
		}

		// respawn options
		if (animOptions.HasRootMotion())
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_RootMotion);
			optionsIndex++;
		}

		if (animOptions.SuppressGestures())
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_SuppressGesture);
			optionsIndex++;
		}

		if (animOptions.UseDefaultPose())
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_DefaultPose);
			optionsIndex++;
		}
	}

	CommandOption_t* CommandAnimation_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		// 
		// unable to replicate = data was baked in and cannot be extracted
		// 
		// COMMANDS VALID FOR ANIMATION
		// fps				(done)
		// origin			(unable to replicate) (adjusts animation origin/offset)
		// rotate			(unable to replicate) (adjusts rotation of z axis)
		// angles			(unable to replicate) (adjusts rotation of angles)
		// scale			(unable to replicate) (adjusts scale of values)
		// loop				(done) (animation loops)
		// noforceloop		(done) (not forced into looping anim?)
		// startloop		(unable to replicate) (adjusts loop point?)
		// percentstartloop (unable to replicate) (adjusts loop point?)
		// fudgeloop		(unable to replicate) (last frame was not the same as the first, so make it match, specifically for motions)
		// snap				(done) (snaps to next blend, no interp)
		// frame/frames framestart (unable to replicate) (truncates animation)
		// blockname		(unable to replicate) (gets animation from provided source (mdl? dmx?))
		// post				(done) (CHECK FOR OTHER FLAGS ON SEQS!!!) (STUDIO_WORLD_AND_RELATIVE, STUDIO_WORLD, STUDIO_DELTA)
		// noautoik			(done) (check ikrules for default release ikrules)
		// autoik			(done) (check ikrules for default release ikrules)
		// cmdlist			(unable to replicate) (for a list of options)
		// motionrollback	(unable to replicate) (used to prevent 'popping' on looped animations with motions)
		// noanimblock		(todo) (animation doesn't use animblocks)
		// noanimblockstall (todo) (first section is stored locally instead of using stall frames)
		// nostallframes	(todo) (value is used for some checks but not for writing?)
		// motion axis		(done) (X, Y, Z, XR, YR, ZR, LX, LY, LZ, LXR, LYR, LZR, LM, LQ)
		// rm				(done)
		// suppgest			(done)
		// defaultpose		(done)

		// COMMANDS VALID FOR CMDLIST
		// fixuploop			(unable to replicate) (blends the end and start frames to smoothly transition)
		// weightlist			(done) (only used in sequence currently)
		// subtract				(todo) (use an empty smd to simply apply the delta flag)
		// presubtract			(unable to replicate) (subtract with extra steps)
		// alignto				(unable to replicate) (adjusts animation on compile)
		// align				(unable to replicate) (adjusts animation on compile)
		// alignboneto			(unable to replicate) (adjusts animation on compile)
		// alignbone			(unable to replicate) (adjusts animation on compile)
		// match				(unable to replicate) (adds to another animation on compile)
		// matchblend			(unable to replicate) (above but blending)
		// worldspaceblend		(unable to replicate) (adjusts animation on compile)
		// worldspaceblendloop	(unable to replicate) (above but loop)
		// rotateto				(unable to replicate) (rotates the entire animations)
		// ikrule				(incomplete)
		// ikfixup				(incomplete) (ikerrors, animation should be looped?)
		// walkframe			(done)
		// walkalignto			(unable to replicate) (parses extra data in, use walkframe)
		// walkalign			(unable to replicate) (parses extra data in, use walkframe)
		// derivative			(unable to replicate) (adjusts animation on compile)
		// noanimation			(done)
		// noanim_keepduration	(done)
		// lineardelta			(todo) (check autolayer flags, probably just a comment about this being used)
		// splinedelta			(todo) (check autolayer flags, probably just a comment about this being used)
		// compress				(unable to replicate) (adjusts framerate, do not know original framerate)
		// numframes			(unable to replicate) (truncates animation)
		// counterrotate		(unable to replicate) (rotates a bone)
		// counterrotateto		(unable to replicate) (rotates a bone)
		// localhierarchy		(todo)
		// forceboneposrot		(unable to replicate) (forces a bone translation and updates child bones)
		// bonedriver			(unable to replicate) (reference bone driver is not stored, and only updates local values)
		// reverse				(unable to replicate) (compiles the animation backwards, possible to decompile if any sign of it being used are stored(apex anim names?))
		// appendanim			(unable to replicate) (appends another animation on the end of this, compiled in)

		UNUSED(count);
		UNUSED(store);

		const AnimationData_t* const animData = reinterpret_cast<const AnimationData_t* const>(data);

		const uint32_t usedOptions = 2u + animData->EstimateOptionCount_Anim(); // name & filepath always present
		CommandOption_t* const options = new CommandOption_t[usedOptions];

		uint32_t optionsIndex = 0u;

		options[optionsIndex].Init(&s_CommandGeneric_Option_Name);
		options[optionsIndex].SetPtr(animData->name);
		optionsIndex++;

		// always expect this needs to be allocated
		options[optionsIndex].Init(&s_CommandAnimation_Option_Filepath);
		options[optionsIndex].SaveStr(file, animData->filepath);
		optionsIndex++;

		if (animData->numMotions && usedOptions >= (optionsIndex + animData->numMotions))
		{
			CommandAnimation_ParseOption_Motion(file, options, optionsIndex, animData);
		}

		if (animData->numIKRules && usedOptions >= (optionsIndex + animData->numIKRules))
		{
			CommandAnimation_ParseOption_IKRule(file, options, optionsIndex, animData);
		}

		CommandAnimation_ParseOption_Simple(file, options, optionsIndex, animData, false);

		*numOptions = usedOptions;

		return options;
	}

	void CommandSequence_ParseOption_Layer(QCFile* const file, const SequenceData_t* const seqData, const SequenceLayer_t* const layer, CommandOptionArray_t* const layerArray, const bool useBlendLayer)
	{
		uint32_t optionsIndex = 0u;

		CommandOption_t* const options = layerArray->options;

		options[optionsIndex].Init(&s_CommandGeneric_Option_Name);
		options[optionsIndex].SetPtr(layer->sequence);
		optionsIndex++;

		if (useBlendLayer)
		{
			int range[4]{};
			const float numFrames = static_cast<float>(seqData->numFrames - 1);

			range[0] = static_cast<int>(numFrames * layer->start);
			range[1] = static_cast<int>(numFrames * layer->peak);
			range[2] = static_cast<int>(numFrames * layer->tail);
			range[3] = static_cast<int>(numFrames * layer->end);

			options[optionsIndex].Init(&s_CommandSequence_Option_Layer_Range);
			options[optionsIndex].SavePtr(file, range, 4u, sizeof(range));
			optionsIndex++;
		}

		if (layer->flags & STUDIO_AL_LOCAL)
		{
			options[optionsIndex].Init(&s_CommandSequence_Option_Layer_Local);
			optionsIndex++;
		}

		if (useBlendLayer == false)
		{
			layerArray->TruncateOptions(optionsIndex);
			return;
		}

		if (layer->flags & STUDIO_AL_POSE)
		{
			options[optionsIndex].Init(&s_CommandSequence_Option_Layer_Pose);
			options[optionsIndex].SetPtr(layer->pose);
			optionsIndex++;
		}

		if (layer->flags & STUDIO_AL_NOBLEND)
		{
			options[optionsIndex].Init(&s_CommandSequence_Option_Layer_Local);
			optionsIndex++;
		}

		if (layer->flags & STUDIO_AL_SPLINE)
		{
			options[optionsIndex].Init(&s_CommandSequence_Option_Layer_Spline);
			optionsIndex++;
		}

		if (layer->flags & STUDIO_AL_XFADE)
		{
			options[optionsIndex].Init(&s_CommandSequence_Option_Layer_XFade);
			optionsIndex++;
		}

		if (layer->flags & STUDIO_AL_REALTIME)
		{
			options[optionsIndex].Init(&s_CommandSequence_Option_Layer_RealTime);
			optionsIndex++;
		}

		if (layer->flags & STUDIO_AL_2000)
		{
			options[optionsIndex].Init(&s_CommandSequence_Option_Layer_Unk2000);
			optionsIndex++;
		}

		layerArray->TruncateOptions(optionsIndex);
	}

	void CommandSequence_ParseOption_IKResetMask(QCFile* const file, CommandOption_t* const options, uint32_t& optionsIndex, const SequenceData_t* const seqData)
	{
		UNUSED(file);

		options[optionsIndex].Init(&s_CommandSequence_Option_IKResetMask);

		switch (seqData->ikResetMask)
		{
		case IK_SELF:
		{
			options[optionsIndex].SetPtr("touch");
			break;
		}
		case IK_WORLD:
		{
			assertm(false, "IK_WORLD is only set via runtime code");
			break;
		}
		case IK_GROUND:
		{
			options[optionsIndex].SetPtr("footstep");
			break;
		}
		case IK_ATTACHMENT:
		{
			options[optionsIndex].SetPtr("attachment");
			break;
		}
		case IK_RELEASE:
		{
			options[optionsIndex].SetPtr("release");
			break;
		}
		case IK_UNLATCH:
		{
			options[optionsIndex].SetPtr("unlatch");
			break;
		}
		}

		optionsIndex++;
	}

	CommandOption_t* CommandSequence_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		// event			(done)
		// activity			(done)
		// activitymodifier	(done)
		// snap				(done)
		// blendwidth		(done)
		// blend			(done)
		// calcblend		(unable to replicate) (adjusts param on compile)
		// blendref			(unable to replicate) (adjusts param on compile)
		// blendcomp		(unable to replicate) (adjusts param on compile)
		// blendcenter		(unable to replicate) (adjusts param on compile)
		// node				(done)
		// transition		(done)
		// rtransition		(done)
		// exitphase		(done)
		// delta			(done)
		// worldspace		(done)
		// post				(done)
		// predelta			(unable to replicate) (use delta, this does stuff on compile)
		// autoplay			(done)
		// fadein			(done)
		// fadeout			(done)
		// realtime			(done)
		// posecycle		(done)
		// hidden			(done)
		// addlayer			(done)
		// iklock			(done)
		// keyvalues		(done)
		// blendlayer		(done)

		// ikresetmask		(done)

		// other respawn stuff

		UNUSED(count);
		UNUSED(store);

		const SequenceData_t* const seqData = reinterpret_cast<const SequenceData_t* const>(data);

		const uint32_t usedOptions = seqData->EstimateOptionCount_Seq();
		CommandOption_t* options = new CommandOption_t[usedOptions];

		uint32_t optionsIndex = 0u;

		options[optionsIndex].Init(&s_CommandGeneric_Option_Name);
		options[optionsIndex].SetPtr(seqData->name);
		optionsIndex++;

		assertm(seqData->numBlends, "sequence without animations");
		if (seqData->numBlends > 1)
		{
			for (int i = 0; i < seqData->numBlends; i += seqData->GetWidth())
			{
				options[optionsIndex].Init(&s_CommandSequence_Option_Blends);

				if (seqData->GetWidth() == 1)
				{
					options[optionsIndex].SetPtr(seqData->blends[i], seqData->GetWidth());
				}
				else
				{
					options[optionsIndex].SavePtr(file, seqData->blends + i, seqData->GetWidth(), sizeof(char*) * seqData->GetWidth());
				}

				optionsIndex++;
			}

			options[optionsIndex].Init(&s_CommandSequence_Option_BlendWidth);
			options[optionsIndex].SetRaw(&seqData->GetWidth(), 1u, sizeof(int));
			optionsIndex++;
		}
		else
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_Filepath);
			options[optionsIndex].SaveStr(file, seqData->blends[0]);
			optionsIndex++;
		}

		for (int i = 0; i < 2; i++)
		{
			if (seqData->param[i] == nullptr)
			{
				continue;
			}

			const float min_max[2] = { seqData->paramstart[i], seqData->paramend[i] };

			CommandOptionPair_t seqBlend;
			seqBlend.SetData(0u, seqData->param[i], 1u, QC_OPT_STRING);
			seqBlend.SetData(1u, min_max, 2u, QC_OPT_FLOAT);

			options[optionsIndex].Init(&s_CommandSequence_Option_Blend);
			options[optionsIndex].SavePtr(file, &seqBlend, 1u, sizeof(CommandOptionPair_t));
			optionsIndex++;
		}

		if (seqData->weightlist)
		{
			options[optionsIndex].Init(&s_CommandAnimation_Option_WeightList);
			options[optionsIndex].SetPtr(seqData->weightlist);
			optionsIndex++;
		}

		if (seqData->numMotions && usedOptions >= (optionsIndex + seqData->numMotions))
		{
			CommandAnimation_ParseOption_Motion(file, options, optionsIndex, seqData);
		}

		// ik stuff
		if (seqData->numIKRules && usedOptions >= (optionsIndex + seqData->numIKRules))
		{
			CommandAnimation_ParseOption_IKRule(file, options, optionsIndex, seqData);
		}

		for (int i = 0; i < seqData->numIKLocks; i++)
		{
			const IKLockData_t* const seqIKLock = seqData->ikLocks + i;
			CommandOptionArray_t seqIKLockArray(file, 3u);

			CommandIKAutoPlayLock_ParseOptions(file, seqIKLockArray.options, 3u, store, seqIKLock);

			options[optionsIndex].Init(&s_CommandSequence_Option_IKLock);
			options[optionsIndex].SavePtr(file, &seqIKLockArray, 1u, sizeof(CommandOptionArray_t));
			optionsIndex++;
		}

		if (seqData->ikResetMask)
		{
			CommandSequence_ParseOption_IKResetMask(file, options, optionsIndex, seqData);
		}

		// activity
		if (seqData->HasActivity())
		{
			const CommandOptionPair_t seqActivity(seqData->activity, seqData->activityWeight);

			options[optionsIndex].Init(&s_CommandSequence_Option_Activity);
			options[optionsIndex].SavePtr(file, &seqActivity, 1u, sizeof(CommandOptionPair_t));
			optionsIndex++;
		}

		for (int i = 0; i < seqData->numActMods; i++)
		{
			const SequenceActMod_t* const seqActMod = seqData->actmods + i;
			CommandOptionArray_t seqActModArray(file, 1u + seqActMod->negate);

			seqActModArray.options[0].Init(&s_CommandGeneric_Option_Name);
			seqActModArray.options[0].SetPtr(seqActMod->activity);

			if (seqActMod->negate)
			{
				seqActModArray.options[1].Init(&s_CommandSequence_Option_ActMod_Negate);
			}

			options[optionsIndex].Init(&s_CommandSequence_Option_ActMod);
			options[optionsIndex].SavePtr(file, &seqActModArray, 1u, sizeof(CommandOptionArray_t));
			optionsIndex++;
		}

		// layers
		for (int i = 0; i < seqData->numLayers; i++)
		{
			const SequenceLayer_t* const seqLayer = seqData->layers + i;
			const bool useBlendLayer = seqLayer->UseBlendLayer();

			CommandOptionArray_t seqLayerArray(file, useBlendLayer ? 16u : 2u);

			CommandSequence_ParseOption_Layer(file, seqData, seqLayer, &seqLayerArray, useBlendLayer);

			const CommandOptionDesc_t* const layerDesc = seqLayer->UseBlendLayer() ? &s_CommandSequence_Option_BlendLayer : &s_CommandSequence_Option_AddLayer;

			options[optionsIndex].Init(layerDesc);
			options[optionsIndex].SavePtr(file, &seqLayerArray, 1u, sizeof(CommandOptionArray_t));
			optionsIndex++;
		}

		// nodes
		if (seqData->localentrynode && seqData->localexitnode)
		{
			if (seqData->nodeflags == 1)
			{
				const CommandOptionPair_t rTransition(seqData->entrynode, seqData->exitnode);

				options[optionsIndex].Init(&s_CommandSequence_Option_RTransition);
				options[optionsIndex].SavePtr(file, &rTransition, 1u, sizeof(CommandOptionPair_t));
				optionsIndex++;
			}
			else if (seqData->localentrynode != seqData->localexitnode)
			{
				const CommandOptionPair_t transition(seqData->entrynode, seqData->exitnode);

				options[optionsIndex].Init(&s_CommandSequence_Option_Transition);
				options[optionsIndex].SavePtr(file, &transition, 1u, sizeof(CommandOptionPair_t));
				optionsIndex++;
			}
			else
			{
				options[optionsIndex].Init(&s_CommandSequence_Option_Node);
				options[optionsIndex].SetPtr(seqData->entrynode);
				optionsIndex++;
			}
		}
		// apex seems to support setting these independently
		else if (seqData->localentrynode)
		{
			options[optionsIndex].Init(&s_CommandSequence_Option_EntryNode);
			options[optionsIndex].SetPtr(seqData->entrynode);
			optionsIndex++;
		}
		else if (seqData->localexitnode)
		{
			options[optionsIndex].Init(&s_CommandSequence_Option_ExitNode);
			options[optionsIndex].SetPtr(seqData->exitnode);
			optionsIndex++;
		}

		// misc
		if (seqData->fadeintime != s_Sequence_DefaultFadeIn)
		{
			options[optionsIndex].Init(&s_CommandSequence_Option_FadeIn);
			options[optionsIndex].SetRaw(&seqData->fadeintime, 1u, sizeof(float));
			optionsIndex++;
		}

		if (seqData->fadeouttime != s_Sequence_DefaultFadeOut)
		{
			options[optionsIndex].Init(&s_CommandSequence_Option_FadeOut);
			options[optionsIndex].SetRaw(&seqData->fadeouttime, 1u, sizeof(float));
			optionsIndex++;
		}

		if (seqData->options.HasCyclePose())
		{
			options[optionsIndex].Init(&s_CommandSequence_Option_PoseCycle);
			options[optionsIndex].SetPtr(seqData->cyclePose);
			optionsIndex++;
		}

		// events
		for (int i = 0; i < seqData->numEvents; i++)
		{
			const SequenceEvent_t* const seqEvent = seqData->events + i;
			CommandOptionArray_t seqEventArray(file, 4u + seqEvent->HasOptions());

			// cheat a little so our brackets are outside
			seqEventArray.options[0].Init(&s_CommandSequence_Option_Event_Key);

			if (seqEvent->UseEventName())
			{
				seqEventArray.options[1].Init(&s_CommandGeneric_Option_Name);
				seqEventArray.options[1].SetPtr(seqEvent->name);
			}
			else
			{
				seqEventArray.options[1].Init(&s_CommandSequence_Option_Event_ID);
				seqEventArray.options[1].SetRaw(&seqEvent->event, 1u, sizeof(int));
			}

			const int frame = static_cast<int>(static_cast<float>(seqData->numFrames - 1) * seqEvent->cycle);
			seqEventArray.options[2].Init(&s_CommandSequence_Option_Event_Frame);
			seqEventArray.options[2].SetRaw(&frame, 1u, sizeof(int));

			// what is this for?
			seqEventArray.options[3].Init(&s_CommandSequence_Option_Event_Unk);
			seqEventArray.options[3].SetRaw(&seqEvent->unk, 1u, sizeof(float));

			if (seqEvent->HasOptions())
			{
				seqEventArray.options[4].Init(&s_CommandSequence_Option_Event_Options);
				seqEventArray.options[4].SetPtr(seqEvent->options);
			}

			options[optionsIndex].Init(&s_CommandSequence_Option_Event);
			options[optionsIndex].SavePtr(file, &seqEventArray, 1u, sizeof(CommandOptionArray_t));
			optionsIndex++;
		}

		if (seqData->keyValues)
		{
			options[optionsIndex].Init(&s_CommandSequence_Option_KeyValues);
			options[optionsIndex].SetPtr(seqData->keyValues);
			optionsIndex++;
		}

		// these only get parsed if we have animations or blends, so if the animation is not on the parent line (multiple blends) we need to make sure these are not on the parent line
		CommandAnimation_ParseOption_Simple(file, options, optionsIndex, seqData, seqData->numBlends > 1);

		if (seqData->exitphase != 0.0f)
		{
			options[optionsIndex].Init(&s_CommandSequence_Option_ExitPhase);
			options[optionsIndex].SetRaw(&seqData->exitphase, 1u, sizeof(float));
			optionsIndex++;
		}

		*numOptions = usedOptions;

		return options;
	}

	// collision
	inline void CommandContents_ParseOptions(CommandOption_t* const options, const uint32_t usedOptions, uint32_t& optionsIndex, const int contents)
	{

		if (contents == CONTENTS_EMPTY)
		{
			options[optionsIndex].Init(&s_CommandContents_Option_Flag);
			options[optionsIndex].SetPtr(StudioContentFlagString(contents));
			optionsIndex++;

			return;
		}

		for (int bitIdx = 0; bitIdx < 32; bitIdx++)
		{
			if (optionsIndex == usedOptions)
			{
				break;
			}

			// shift here to get the content flag for this bit
			const int content = 1 << bitIdx;
			if ((contents & content) == 0)
			{
				continue;
			}

			options[optionsIndex].Init(&s_CommandContents_Option_Flag);
			options[optionsIndex].SetPtr(StudioContentFlagString(contents & content));
			optionsIndex++;
		}
	}

	CommandOption_t* CommandContents_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		UNUSED(file);
		UNUSED(count);
		UNUSED(store);

		const int contents = *reinterpret_cast<const int* const>(data);
		const uint32_t usedFlags = __popcnt(contents);

		const uint32_t usedOptions = usedFlags ? usedFlags : 1;
		CommandOption_t* options = new CommandOption_t[usedOptions];

		uint32_t optionsIndex = 0u;
		CommandContents_ParseOptions(options, usedOptions, optionsIndex, contents);

		*numOptions = usedOptions;

		return options;
	}

	CommandOption_t* CommandHBox_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store = false)
	{
		UNUSED(count);
		UNUSED(store);

		const Hitbox_t* const hitboxGroupData = reinterpret_cast<const Hitbox_t* const>(data);

		// group, bone, min, max, angle (csgo), radius (csgo), force crit (r2), name (optional)
		assertm(hitboxGroupData->forceCritPoint < 2, "invalid forceCritPoint value");
		const bool isCSGO = hitboxGroupData->IsCSGO();
		const bool hasName = hitboxGroupData->HasName();
		const bool hasHitDatGroup = hitboxGroupData->HasHitDataGroup();
		const uint32_t usedOptions = 4 + (isCSGO ? 2 : 0) + hasName + hasHitDatGroup + hitboxGroupData->forceCritPoint;
		CommandOption_t* options = new CommandOption_t[usedOptions];

		uint32_t optionsIndex = 0;

		if ((optionsIndex + 4) <= usedOptions)
		{
			options[0].Init(&s_CommandHBox_Option_Group);
			options[0].SetRaw(&hitboxGroupData->group, 1, sizeof(int));

			options[1].Init(&s_CommandGeneric_Option_Bone);
			options[1].SetPtr(hitboxGroupData->bone);

			options[2].Init(&s_CommandHBox_Option_Min);
			options[2].SetPtr(hitboxGroupData->bbmin, 3);

			options[3].Init(&s_CommandHBox_Option_Max);
			options[3].SetPtr(hitboxGroupData->bbmax, 3);

			optionsIndex += 4;
		}

		// box supports capsules
		if (isCSGO && (optionsIndex + 2) <= usedOptions)
		{
			QAngle ang(hitboxGroupData->bbangle->z, hitboxGroupData->bbangle->x, hitboxGroupData->bbangle->y);
			options[optionsIndex].Init(&s_CommandHBox_Option_Angle);
			options[optionsIndex].SavePtr(file, &ang, 3, sizeof(QAngle));

			options[optionsIndex + 1].Init(&s_CommandHBox_Option_Radius);
			options[optionsIndex + 1].SetRaw(&hitboxGroupData->flRadius);

			optionsIndex += 2;
		}

		// box has name
		if (hasName && (optionsIndex + 1) <= usedOptions)
		{
			options[optionsIndex].Init(&s_CommandGeneric_Option_Name);
			options[optionsIndex].SetPtr(hitboxGroupData->name);

			optionsIndex += 1;
		}

		// box has hitgroup
		if (hasHitDatGroup && (optionsIndex + 1) <= usedOptions)
		{
			options[optionsIndex].Init(&s_CommandHBox_Option_HitData);
			options[optionsIndex].SetPtr(hitboxGroupData->hitDataGroup);

			optionsIndex += 1;
		}

		// force box as crit
		if (hitboxGroupData->forceCritPoint && (optionsIndex + 1) <= usedOptions)
		{
			options[optionsIndex].Init(&s_CommandHBox_Option_Crit);

			optionsIndex += 1;
		}

		*numOptions = usedOptions;

		return options;
	}

	// misc
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_Mass(QC_OPT_FLOAT, "$mass", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_Concave(QC_OPT_NONE, "$concave", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_ConcavePerJoint(QC_OPT_NONE, "$concaveperjoint", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_MaxConvexPieces(QC_OPT_INT, "$maxconvexpieces", s_CommandOptionFormatNameNewLine, s_QCVersion_OB, s_QCVersion_MAX);
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_Inertia(QC_OPT_FLOAT, "$inertia", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_Damping(QC_OPT_FLOAT, "$damping", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_RotDamping(QC_OPT_FLOAT, "$rotdamping", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_Drag(QC_OPT_FLOAT, "$drag", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_RollingDrag(QC_OPT_FLOAT, "$rollingDrag", s_CommandOptionFormatNameNewLine);

	// joints
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_JointMerge(QC_OPT_PAIR, "$jointmerge", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_JointInertia(QC_OPT_PAIR, "$jointinertia", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_JointDamping(QC_OPT_PAIR, "$jointdamping", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_JointRotDamping(QC_OPT_PAIR, "$jointrotdamping", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_JointMassBias(QC_OPT_PAIR, "$jointmassbias", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_JointCollide(QC_OPT_PAIR, "$jointcollide", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_JointConstraint(QC_OPT_ARRAY, "$jointconstrain", s_CommandOptionFormatNameNewLine);

	// joint constrain
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_JointConstraintAxis(QC_OPT_STRING, "jointconstrain_axis");
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_JointConstraintAllowOption(QC_OPT_STRING, "jointconstrain_allow_option");
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_JointConstraintMinAngle(QC_OPT_FLOAT, "jointconstrain_minangle");
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_JointConstraintMaxAngle(QC_OPT_FLOAT, "jointconstrain_maxangle");
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_JointConstraintFriction(QC_OPT_FLOAT, "jointconstrain_friction");

	constexpr CommandOptionDesc_t s_CommandCollModel_Option_RootBone(QC_OPT_STRING, "$rootbone", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_NoSelfCollisions(QC_OPT_FLOAT, "$noselfcollisions", s_CommandOptionFormatNameNewLine);
	constexpr CommandOptionDesc_t s_CommandCollModel_Option_AnimatedFriction(QC_OPT_FLOAT, "$animatedfriction", s_CommandOptionFormatNameNewLine);

	CommandOption_t* CommandCollModel_ParseBinary(QCFile* const file, uint32_t* const numOptions, const void* const data, const uint32_t count, const bool store)
	{
		UNUSED(count);
		UNUSED(store);

#ifdef HAS_PHYSICSMODEL_PARSER
		using namespace PhysicsModel;

		const PhysicsData_t* const physData = reinterpret_cast<const PhysicsData_t* const>(data);
		const CParsedPhys* const parsedPhys = physData->parsed;

		const uint32_t usedOptions = physData->EstimateOptionCount();
		CommandOption_t* options = new CommandOption_t[usedOptions];
		uint32_t optionsIndex = 0u;

		// filepath to the mesh data
		options[optionsIndex].Init(&s_CommandGeneric_Option_Name);
		options[optionsIndex].SaveStr(file, physData->filename);
		optionsIndex++;

		// edit params data
		const EditParam* const editParam = parsedPhys->GetEditParam();
		if (editParam)
		{
			const float totalMass = editParam->GetTotalMass();
			options[optionsIndex].Init(&s_CommandCollModel_Option_Mass);
			options[optionsIndex].SetRaw(&totalMass, 1, sizeof(float));
			optionsIndex++;

			if (editParam->IsConcave())
			{
				options[optionsIndex].Init(&s_CommandCollModel_Option_Concave);
				optionsIndex++;
			}

			const char* const rootName = editParam->GetRootName();
			if (rootName && rootName[0])
			{
				options[optionsIndex].Init(&s_CommandCollModel_Option_RootBone);
				options[optionsIndex].SetPtr(rootName);
				optionsIndex++;
			}
		}

		if (parsedPhys->ConcavePerJoint())
		{
			options[optionsIndex].Init(&s_CommandCollModel_Option_ConcavePerJoint);
			optionsIndex++;
		}

		int maxConvexPieces = parsedPhys->MaxConvexPieces();
		if (maxConvexPieces >= defaultAllowedConvexPieces)
		{
			options[optionsIndex].Init(&s_CommandCollModel_Option_MaxConvexPieces);
			options[optionsIndex].SetRaw(&maxConvexPieces, 1, sizeof(int));
			optionsIndex++;
		}

		// default commands
		if (true)
		{
			const ParamFlags_t defaultFlags = parsedPhys->GetFlags();

			const Solid* const solid = parsedPhys->GetSolid(0);
			const ParamFlags_t solidFlags = solid->GetFlags();

			if (defaultFlags & PHYSPARAM_FLAG_HAS_INERTIA)
			{
				const float inertia = parsedPhys->GetInertia();
				options[optionsIndex].Init(&s_CommandCollModel_Option_Inertia);
				options[optionsIndex].SetRaw(&inertia, 1, sizeof(float));
				optionsIndex++;
			}

			if (defaultFlags & PHYSPARAM_FLAG_HAS_DAMPING)
			{
				const float damping = parsedPhys->GetDamping();
				options[optionsIndex].Init(&s_CommandCollModel_Option_Damping);
				options[optionsIndex].SetRaw(&damping, 1, sizeof(float));
				optionsIndex++;
			}

			if (defaultFlags & PHYSPARAM_FLAG_HAS_ROTDAMPING)
			{
				const float rotdamping = parsedPhys->GetRotDamping();
				options[optionsIndex].Init(&s_CommandCollModel_Option_RotDamping);
				options[optionsIndex].SetRaw(&rotdamping, 1, sizeof(float));
				optionsIndex++;
			}

			if (solidFlags & PHYSPARAM_FLAG_HAS_DRAG)
			{
				const float drag = solid->GetDrag();
				options[optionsIndex].Init(&s_CommandCollModel_Option_Drag);
				options[optionsIndex].SetRaw(&drag, 1, sizeof(float));
				optionsIndex++;
			}

			if (solidFlags & PHYSPARAM_FLAG_HAS_ROLLINGDRAG)
			{
				const float rollingDrag = solid->GetRollingDrag();
				options[optionsIndex].Init(&s_CommandCollModel_Option_RollingDrag);
				options[optionsIndex].SetRaw(&rollingDrag, 1, sizeof(float));
				optionsIndex++;
			}
		}

		// joint merge commands
		if (editParam && editParam->GetJointMergeCount())
		{
			options[optionsIndex].Init(&s_CommandGeneric_Option_BlankLine);
			optionsIndex++;

			for (int i = 0; i < editParam->GetJointMergeCount(); i++)
			{
				const EditParam::JointMerge_t* const jointMerge = editParam->GetJointMerge(i);

				const CommandOptionPair_t jointMergePair(jointMerge->origin, jointMerge->destination);
				options[optionsIndex].Init(&s_CommandCollModel_Option_JointMerge);
				options[optionsIndex].SavePtr(file, &jointMergePair, 1, sizeof(CommandOptionPair_t));
				optionsIndex++;
			}

		}

		// per joint commands
		for (uint32_t i = 0u; i < parsedPhys->GetSolidCount(); i++)
		{
			const Solid* const solid = parsedPhys->GetSolid(i);
			const ParamFlags_t solidFlags = solid->GetFlags();

			constexpr ParamFlags_t desiredFlags = (PHYSPARAM_FLAG_HAS_INERTIA | PHYSPARAM_FLAG_HAS_DAMPING | PHYSPARAM_FLAG_HAS_ROTDAMPING | PHYSPARAM_FLAG_HAS_MASSBIAS | PHYSOLID_FLAG_HAS_RAGDOLLCONSTRAINT);

			if ((solidFlags & desiredFlags) == false)
			{
				continue;
			}

			assertm(parsedPhys->GetSolidCount() > 1, "single solid physics models should have no per joint data");

			options[optionsIndex].Init(&s_CommandGeneric_Option_BlankLine);
			optionsIndex++;

			if (solidFlags & PHYSPARAM_FLAG_HAS_INERTIA)
			{
				const CommandOptionPair_t dataPair(solid->GetName(), solid->GetInertia());
				options[optionsIndex].Init(&s_CommandCollModel_Option_JointInertia);
				options[optionsIndex].SavePtr(file, &dataPair, 1, sizeof(CommandOptionPair_t));
				optionsIndex++;
			}

			if (solidFlags & PHYSPARAM_FLAG_HAS_DAMPING)
			{
				const CommandOptionPair_t dataPair(solid->GetName(), solid->GetDamping());
				options[optionsIndex].Init(&s_CommandCollModel_Option_JointDamping);
				options[optionsIndex].SavePtr(file, &dataPair, 1, sizeof(CommandOptionPair_t));
				optionsIndex++;
			}

			if (solidFlags & PHYSPARAM_FLAG_HAS_ROTDAMPING)
			{
				const CommandOptionPair_t dataPair(solid->GetName(), solid->GetRotDamping());
				options[optionsIndex].Init(&s_CommandCollModel_Option_JointRotDamping);
				options[optionsIndex].SavePtr(file, &dataPair, 1, sizeof(CommandOptionPair_t));
				optionsIndex++;
			}

			if (solidFlags & PHYSPARAM_FLAG_HAS_MASSBIAS)
			{
				const CommandOptionPair_t dataPair(solid->GetName(), solid->GetMassBias());
				options[optionsIndex].Init(&s_CommandCollModel_Option_JointMassBias);
				options[optionsIndex].SavePtr(file, &dataPair, 1, sizeof(CommandOptionPair_t));
				optionsIndex++;
			}

			if (solidFlags & PHYSOLID_FLAG_HAS_RAGDOLLCONSTRAINT)
			{
				const RagdollConstraint* const ragdollConstraint = solid->GetRagdollConstraint();

				for (int axis = 0; axis < RagdollConstraint::CONAXIS_COUNT; axis++)
				{
					const RagdollConstraint::Constraint_t* const constraint = ragdollConstraint->GetConstraint(axis);
					CommandOptionArray_t constraintArray(file, 6);

					constraintArray.options[0].Init(&s_CommandGeneric_Option_Bone);
					constraintArray.options[0].SetPtr(solid->GetName());

					constraintArray.options[1].Init(&s_CommandCollModel_Option_JointConstraintAxis);
					constraintArray.options[1].SetPtr(s_CollModel_JointConstraintAxis[axis]);

					const PhysicsData_t::JointConType_t type = physData->TypeFromJointConstraint(constraint);
					constraintArray.options[2].Init(&s_CommandCollModel_Option_JointConstraintAllowOption);
					constraintArray.options[2].SetPtr(s_CollModel_JointConstraintType[type]);

					constraintArray.options[3].Init(&s_CommandCollModel_Option_JointConstraintMinAngle);
					constraintArray.options[3].SetRaw(&constraint->min, 1, sizeof(float));

					constraintArray.options[4].Init(&s_CommandCollModel_Option_JointConstraintMaxAngle);
					constraintArray.options[4].SetRaw(&constraint->max, 1, sizeof(float));

					const float fric = constraint->friction * 5.0f; // normalized to 1.0f (0.20f on disk)
					constraintArray.options[5].Init(&s_CommandCollModel_Option_JointConstraintFriction);
					constraintArray.options[5].SetRaw(&fric, 1, sizeof(float));

					options[optionsIndex].Init(&s_CommandCollModel_Option_JointConstraint);
					options[optionsIndex].SavePtr(file, &constraintArray, 1, sizeof(CommandOptionArray_t));
					optionsIndex++;
				}
			}
		}

		// joint collision
		const CollisionRule* const collisionRule = parsedPhys->GetCollisionRule();
		if (collisionRule)
		{
			options[optionsIndex].Init(&s_CommandGeneric_Option_BlankLine);
			optionsIndex++;

			if (collisionRule->NoSelfCollisions())
			{
				options[optionsIndex].Init(&s_CommandCollModel_Option_NoSelfCollisions);
				optionsIndex++;
			}

			for (int i = 0; i < collisionRule->GetCollisionPairCount(); i++)
			{
				const CollisionRule::CollisionPair_t* const collisionPair = collisionRule->GetCollisionPair(i);

				const CommandOptionPair_t collisionPairPair(parsedPhys->GetSolid(collisionPair->solid0)->GetName(), parsedPhys->GetSolid(collisionPair->solid1)->GetName());
				options[optionsIndex].Init(&s_CommandCollModel_Option_JointCollide);
				options[optionsIndex].SavePtr(file, &collisionPairPair, 1, sizeof(CommandOptionPair_t));
				optionsIndex++;
			}
		}

		// $animatedfriction
		const AnimatedFriction* const animFric = parsedPhys->GetAnimatedFriction();
		if (animFric)
		{
			options[optionsIndex].Init(&s_CommandGeneric_Option_BlankLine);
			optionsIndex++;

			float values[PhysicsData_t::ANIMFRIC_COUNT]{};

			values[PhysicsData_t::ANIMFRIC_TIMEIN] = animFric->GetFrictionTimeIn();
			values[PhysicsData_t::ANIMFRIC_TIMEOUT] = animFric->GetFrictionTimeOut();
			values[PhysicsData_t::ANIMFRIC_TIMEHOLD] = animFric->GetFrictionTimeHold();
			values[PhysicsData_t::ANIMFRIC_MIN] = animFric->GetFrictionMin();
			values[PhysicsData_t::ANIMFRIC_MAX] = animFric->GetFrictionMax();

			options[optionsIndex].Init(&s_CommandCollModel_Option_AnimatedFriction);
			options[optionsIndex].SavePtr(file, values, PhysicsData_t::ANIMFRIC_COUNT, sizeof(float) * PhysicsData_t::ANIMFRIC_COUNT);
			optionsIndex++;
		}

		// I don't see a reason to shrink this buffer, it will just be deallocated once we're done writing anyway
		assertm(usedOptions >= optionsIndex, "exceeded max usable options");
		*numOptions = optionsIndex;

		return options;
#else
	UNUSED(data);
	UNUSED(file);

	*numOptions = 0u;
	return nullptr;
#endif // HAS_PHYSICSMODEL_PARSER
	}


	//
	// COMMAND SORTING
	//

	int CompareCommand(const void* a, const void* b)
	{
		const Command_t* const commandFirst = *reinterpret_cast<const Command_t* const* const>(a);
		const Command_t* const commandSecond = *reinterpret_cast<const Command_t* const* const>(b);

		const CommandList_t cmdListFirst = commandFirst->GetCmd();
		const CommandList_t cmdListSecond = commandSecond->GetCmd();

		if (cmdListFirst == cmdListSecond)
		{
			const int indexFirst = commandFirst->GetIndex();
			const int indexSecond = commandSecond->GetIndex();

			if (indexFirst < indexSecond)
			{
				return -1;
			}

			if (indexFirst > indexSecond)
			{
				return 1;
			}
			
			return 0;
		}

		if (cmdListFirst < cmdListSecond)
		{
			return -1;
		}

		if (cmdListFirst > cmdListSecond)
		{
			return 1;
		}

		assertm(false, "should not reach here");
		return 0;
	}

	inline const float CompareCommandLOD_GetThreshold(const Command_t* const command)
	{
		for (uint32_t i = 0u; i < command->numOptions; i++)
		{
			const CommandOption_t* const option = command->options + i;
			if (option->desc != &s_CommandLOD_Option_Threshold)
				continue;

			return *reinterpret_cast<const float* const>(&option->data.value.raw);
		}

		assertm(false, "lod did not have threshold");
		return -2.0f;
	}

	int CompareCommandLOD(const void* a, const void* b)
	{
		const Command_t* const commandFirst = *reinterpret_cast<const Command_t* const* const>(a);
		const Command_t* const commandSecond = *reinterpret_cast<const Command_t* const* const>(b);

		const float thresholdFirst = CompareCommandLOD_GetThreshold(commandFirst);
		const float thresholdSecond = CompareCommandLOD_GetThreshold(commandSecond);

		// todo: test!!!
		// sort shadowlod last
		if (thresholdFirst == -1.0f)
		{
			return 1;
		}

		if (thresholdFirst < thresholdSecond)
		{
			return -1;
		}

		if (thresholdFirst > thresholdSecond)
		{
			return 1;
		}

		return 0;
	}

	const uint32_t CommandSortDefault(const Command_t* const commands, const Command_t** const sorted, const size_t numCommands, const CommandType_t type)
	{
		uint32_t numSorted = 0u;

		for (size_t i = 0ull; i < numCommands; i++)
		{
			const Command_t* const command = commands + i;

			if (command->GetType() != type)
				continue;

			sorted[numSorted] = command;
			numSorted++;
		}

		if (numSorted == 0u)
			return numSorted;

		qsort(sorted, numSorted, sizeof(intptr_t), CompareCommand);

		return numSorted;
	}

	inline const bool CommandSortModel_UnsortedCMD(const CommandList_t cmd)
	{
		return (cmd == CommandList_t::QC_BODY || cmd == CommandList_t::QC_BODYGROUP || cmd == CommandList_t::QC_MODEL);
	}

	const uint32_t CommandSortModel(const Command_t* const commands, const Command_t** const sorted, const size_t numCommands, const CommandType_t type)
	{
		uint32_t numSorted = 0u;
		uint32_t numGeneralSorted = 0u;

		for (size_t i = 0ull; i < numCommands; i++)
		{
			const Command_t* const command = commands + i;

			if (command->GetType() != type)
				continue;

			if (!CommandSortModel_UnsortedCMD(command->GetCmd()))
			{
				numGeneralSorted++;
			}

			sorted[numSorted] = command;
			numSorted++;
		}

		// we want to preserve order, and don't need to parse through again if $maxverts and other commands are not present
		if (numGeneralSorted == 0u)
			return numSorted;

		const Command_t** const temp = sorted + numSorted;
		memcpy_s(temp, sizeof(intptr_t) * numSorted, sorted, sizeof(intptr_t) * numSorted);

		for (uint32_t sortIdx = 0u, copyIdx = numGeneralSorted, i = 0u; i < numSorted; i++)
		{
			const Command_t* const command = temp[i];

			if (!CommandSortModel_UnsortedCMD(command->GetCmd()))
			{
				assertm(sortIdx < numSorted, "invalid index");

				sorted[sortIdx] = temp[i];
				sortIdx++;
				continue;
			}

			assertm(copyIdx < numSorted, "invalid index");

			sorted[copyIdx] = temp[i];
			copyIdx++;
		}

		qsort(sorted, numGeneralSorted, sizeof(intptr_t), CompareCommand);

		return numSorted;
	}

	inline const bool CommandSortLOD_UnsortedCMD(const CommandList_t cmd)
	{
		return (cmd == CommandList_t::QC_ALLOWROOTLODS || cmd == CommandList_t::QC_MINLOD);
	}

	const uint32_t CommandSortLOD(const Command_t* const commands, const Command_t** const sorted, const size_t numCommands, const CommandType_t type)
	{
		uint32_t numSorted = 0u;
		uint32_t numSpecialSort = 0u;

		for (size_t i = 0ull; i < numCommands; i++)
		{
			const Command_t* const command = commands + i;

			if (command->GetType() != type)
				continue;

			sorted[numSorted] = command;
			numSorted++;

			if (CommandSortLOD_UnsortedCMD(command->GetCmd()))
			{
				numSpecialSort++;
			}
		}

		if (numSorted == 0)
			return numSorted;

		if (numSpecialSort)
		{
			const Command_t** const temp = sorted + numSorted;
			for (uint32_t i = 0u, spec = 0u, norm = 0u; i < numSorted; i++)
			{
				const Command_t* const command = sorted[i];

				if (CommandSortLOD_UnsortedCMD(command->GetCmd()))
				{
					temp[spec] = command;
					spec++;
					continue;
				}

				temp[numSpecialSort + norm] = command;
				norm++;
			}

			const size_t sortedSize = sizeof(intptr_t) * numSorted;
			memcpy_s(sorted, sortedSize, temp, sortedSize);
		}

		qsort(sorted + numSpecialSort, numSorted - numSpecialSort, sizeof(intptr_t), CompareCommandLOD);

		return numSorted;
	}

	const uint32_t CommandSortBox(const Command_t* const commands, const Command_t** const sorted, const size_t numCommands, const CommandType_t type)
	{
		uint32_t numSorted = 0u;

		for (size_t i = 0ull; i < numCommands; i++)
		{
			const Command_t* const command = commands + i;

			if (command->GetType() != type)
				continue;

			sorted[numSorted] = command;
			numSorted++;
		}

		// we want to preserve order, and don't need to parse through again if $maxverts is not present
		if (numSorted == 0)
			return numSorted;

		const Command_t** const temp = sorted + numSorted;
		memcpy_s(temp, sizeof(intptr_t) * numSorted, sorted, sizeof(intptr_t) * numSorted);

		uint32_t sortIdx = 0u;
		for (uint32_t tmpIdx = 0u; tmpIdx < numSorted; tmpIdx++)
		{
			const CommandList_t cmd = temp[tmpIdx]->GetCmd();
			if (cmd == CommandList_t::QC_HBOXSET || cmd == CommandList_t::QC_HBOX)
				continue;

			sorted[sortIdx] = temp[tmpIdx];
			sortIdx++;
		}

		qsort(sorted, sortIdx, sizeof(intptr_t), CompareCommand);

		for (uint32_t tmpIdx = 0u; sortIdx < numSorted; tmpIdx++)
		{
			const CommandList_t cmd = temp[tmpIdx]->GetCmd();
			if (cmd != CommandList_t::QC_HBOXSET && cmd != CommandList_t::QC_HBOX)
				continue;

			sorted[sortIdx] = temp[tmpIdx];
			sortIdx++;
		}

		return numSorted;
	}
}