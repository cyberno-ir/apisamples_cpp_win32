#pragma once
#include <windows.h>
#include <WinInet.h>
#include <tchar.h>
#include "json.hpp"

#define EXPORT __declspec (dllexport)

class EXPORT CyUtils {
	std::string current_server_address = "";

public:
	CyUtils(std::string server_address)
	{
		current_server_address = server_address;
	}

	std::string __stdcall get_sha256(std::string file_path);
	std::string __stdcall get_error(json::JSON json_input);
	json::JSON __stdcall call_with_json_input(std::string api, json::JSON json_input);
	json::JSON __stdcall call_with_form_input(std::string api, json::JSON data_input, std::string file_param_name, std::string file_path);

};