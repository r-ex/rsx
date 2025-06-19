#pragma once

class CCommandLine;

void HandleLoadFromCommandLine(const CCommandLine* const cli);
void HandlePakLoad(std::vector<std::string> filePaths);
void HandleMBNKLoad(std::vector<std::string> filePaths);
void HandleMDLLoad(std::vector<std::string> filePaths);
void HandleBPKLoad(std::vector<std::string> filePaths);