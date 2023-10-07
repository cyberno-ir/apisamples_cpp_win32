#include "Header.h"


std::string __stdcall CyUtils::get_sha256(std::string file_path)
{
	std::string out_file_sha256 = "";
	BOOL bResult = FALSE;
	HANDLE hFile = NULL;
	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	BYTE rgbFile[1024];
	DWORD cbRead = 0;
	DWORD cbHash = 0;
	CHAR rgbDigits[] = "0123456789abcdef";
	CHAR temp_sha256[68] = { 0 };

	hFile = CreateFileA(file_path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
		return "";
	// Get handle to the crypto provider
	if (!CryptAcquireContext(&hProv, NULL, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, 0))
	{
		CloseHandle(hFile);
		return "";
	}
	if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
	{
		DWORD err = GetLastError();
		CryptReleaseContext(hProv, 0);
		CloseHandle(hFile);
		return "";
	}
	while (bResult = ReadFile(hFile, rgbFile, 1024, &cbRead, NULL))
	{
		if (0 == cbRead)
			break;
		if (!CryptHashData(hHash, rgbFile, cbRead, 0))
		{
			CryptReleaseContext(hProv, 0);
			CryptDestroyHash(hHash);
			CloseHandle(hFile);
			return "";
		}
	}
	if (!bResult)
	{
		CryptReleaseContext(hProv, 0);
		CryptDestroyHash(hHash);
		CloseHandle(hFile);
		return "";
	}
	cbHash = 64;
	if (!CryptGetHashParam(hHash, HP_HASHVAL, (BYTE*)temp_sha256, &cbHash, 0))
	{
		CloseHandle(hFile);
		return "";
	}
	else
	{
		for (DWORD i = 0; i < cbHash; i++)
		{
			out_file_sha256 += (char)rgbDigits[((unsigned char)temp_sha256[i]) >> 4];
			out_file_sha256 += (char)rgbDigits[((unsigned char)temp_sha256[i]) & 0xf];
		}
	}
	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);
	CloseHandle(hFile);
	return out_file_sha256;
}

std::string __stdcall CyUtils::get_error(json::JSON json_input) {
	std::string returnValue = "Error!\n";
	if (json_input.hasKey("error_code"))
		returnValue += (std::string("Error code: ") + std::to_string(json_input["error_code"].ToInt()) + "\n");
	if (json_input.hasKey("error_desc"))
		returnValue += (std::string("Error description: ") + json_input["error_desc"].ToString() + "\n");
	return returnValue;
}

json::JSON __stdcall CyUtils::call_with_json_input(std::string api, json::JSON json_input)
{
	bool use_https = FALSE;
	json::JSON return_json;
	std::string server_domain = "";
	if (current_server_address.find("http://") == 0)
	{
		server_domain = current_server_address;
		server_domain.erase(0, 7);
	}
	else if (current_server_address.find("https://") == 0)
	{
		use_https = TRUE;
		server_domain = current_server_address;
		server_domain.erase(0, 8);
	}
	else {
		return_json["success"] = false;
		return_json["error_code"] = 900; //unknown error code
		return return_json;
	}
	HINTERNET internet_handle = InternetOpen(TEXT("Cyberno-API-Sample-CPP"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (internet_handle == NULL)
	{
		return_json["success"] = false;
		return_json["error_code"] = 900; //unknown error code
		return return_json;
	}
	HINTERNET http_connection = InternetConnectA(internet_handle, server_domain.c_str(), use_https == TRUE ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, INTERNET_NO_CALLBACK, 0);
	if (http_connection == NULL)
	{
		InternetCloseHandle(internet_handle);
		return_json["success"] = false;
		return_json["error_code"] = 900; //unknown error code
		return return_json;
	}
	int open_request_flags = INTERNET_FLAG_RELOAD;
	if (use_https == TRUE)
		open_request_flags += INTERNET_FLAG_SECURE;
	HINTERNET site_connection = HttpOpenRequestA(http_connection, "POST", ("/" + api).c_str(), NULL, NULL, NULL, open_request_flags, 0);
	if (site_connection == NULL)
	{
		InternetCloseHandle(http_connection);
		InternetCloseHandle(internet_handle);
		return_json["success"] = false;
		return_json["error_code"] = 900; //unknown error code
		return return_json;
	}
	std::string hdrs = "Content-Type: application/json";
	std::string frmdata = json_input.dump();
	if (HttpSendRequestA(site_connection, hdrs.c_str(), (DWORD)hdrs.length(), (PVOID)frmdata.c_str(), (DWORD)frmdata.length()) == FALSE)
	{
		InternetCloseHandle(site_connection);
		InternetCloseHandle(http_connection);
		InternetCloseHandle(internet_handle);
		return_json["success"] = false;
		return_json["error_code"] = 900; //unknown error code
		return return_json;
	}
	DWORD readed_data_count = 0;
	DWORD data_available = 0;
	PCHAR data_response = NULL;
	do {
		InternetQueryDataAvailable(site_connection, &data_available, 0, 0);
		readed_data_count = data_available + readed_data_count;
		if (readed_data_count == 0)
		{
			InternetCloseHandle(site_connection);
			InternetCloseHandle(http_connection);
			InternetCloseHandle(internet_handle);
			return_json["success"] = false;
			return_json["error_code"] = 900; //unknown error code
			return return_json;
		}
		if (data_available != 0)
		{
			if (data_response == NULL)
			{
				data_response = (PCHAR)malloc(readed_data_count);
				ZeroMemory(data_response, readed_data_count);
			}
			else
			{
				data_response = (PCHAR)realloc(data_response, readed_data_count);
			}
			DWORD data_read = 0;
			InternetReadFile(site_connection, data_response + readed_data_count - data_available, data_available, &data_read);
			if (data_read == 0)
			{
				InternetCloseHandle(site_connection);
				InternetCloseHandle(http_connection);
				InternetCloseHandle(internet_handle);
				return_json["success"] = false;
				return_json["error_code"] = 900; //unknown error code
				return return_json;
			}
		}
	} while (data_available != 0);
	json::JSON obj;
	obj = json::JSON::Load(data_response);
	free(data_response);
	InternetCloseHandle(site_connection);
	InternetCloseHandle(http_connection);
	InternetCloseHandle(internet_handle);
	if (obj.hasKey("success") == true)
		return obj;	
	else
	{

		return_json["success"] = false;
		return_json["error_code"] = 900; //unknown error code
		return return_json;
	}
}

json::JSON __stdcall CyUtils::call_with_form_input(std::string api, json::JSON data_input, std::string file_param_name, std::string file_path)
{
	bool use_https = FALSE;
	json::JSON return_json;
	std::string server_domain = "";
	if (current_server_address.find("http://") == 0)
	{
		server_domain = current_server_address;
		server_domain.erase(0, 7);
	}
	else if (current_server_address.find("https://") == 0)
	{
		use_https = TRUE;
		server_domain = current_server_address;
		server_domain.erase(0, 8);
	}
	else {
		return_json["success"] = false;
		return_json["error_code"] = 900; //unknown error code
		return return_json;
	}
	HINTERNET internet_handle = InternetOpen(TEXT("Cyberno-API-Sample-CPP"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (internet_handle == NULL)
	{
		return_json["success"] = false;
		return_json["error_code"] = 900; //unknown error code
		return return_json;
	}
	HINTERNET http_connection = InternetConnectA(internet_handle, server_domain.c_str(), use_https == TRUE ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, INTERNET_NO_CALLBACK, 0);
	if (http_connection == NULL)
	{
		InternetCloseHandle(internet_handle);
		return_json["success"] = false;
		return_json["error_code"] = 900; //unknown error code
		return return_json;
	}
	int open_request_flags = INTERNET_FLAG_RELOAD;
	if (use_https == TRUE)
		open_request_flags += INTERNET_FLAG_SECURE;
	HINTERNET site_connection = HttpOpenRequestA(http_connection, "POST", ("/" + api).c_str(), NULL, NULL, NULL, open_request_flags, 0);
	if (site_connection == NULL)
	{
		InternetCloseHandle(http_connection);
		InternetCloseHandle(internet_handle);
		return_json["success"] = false;
		return_json["error_code"] = 900; //unknown error code
		return return_json;
	}
	std::string boundary = "---------------------------293582696224464";
	std::string hdrs = "Content-Type: multipart/form-data; boundary=" + boundary;
	std::string frmdata_begin = "", frmdata_end = "";
	auto data_all = data_input.ObjectRange();
	for (auto iterator = data_all.begin(), e = data_all.end(); iterator != e; ++iterator)
	{
		frmdata_begin += ("--" + boundary + "\r\nContent-Disposition: form-data; name=\"");
		frmdata_begin += iterator->first;
		frmdata_begin += "\"\r\n\r\n";
		frmdata_begin += iterator->second.ToString() + "\r\n";
	}
	frmdata_begin += ("--" + boundary + "\r\nContent-Disposition: form-data; name=\"" + file_param_name + "\"; filename=\"file\"\r\n\r\n");
	frmdata_end = "\r\n--" + boundary + "--\r\n";
	//read file data:
	HANDLE hFile = CreateFileA(file_path.c_str(), GENERIC_READ, FILE_SHARE_READ + FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		InternetCloseHandle(http_connection);
		InternetCloseHandle(internet_handle);
		return_json["success"] = false;
		return_json["error_code"] = 900; //unknown error code
		return return_json;
	}
	DWORD file_size = GetFileSize(hFile, NULL);
	PBYTE file_data = (PBYTE)malloc(file_size);
	ZeroMemory(file_data, file_size);
	DWORD BytesRead = 0;
	ReadFile(hFile, file_data, file_size, &BytesRead, NULL);
	CloseHandle(hFile);
	if (BytesRead != file_size)
	{
		InternetCloseHandle(http_connection);
		InternetCloseHandle(internet_handle);
		return_json["success"] = false;
		return_json["error_code"] = 900; //unknown error code
		return return_json;
	}
	DWORD data_to_send_size = (DWORD)frmdata_begin.length() + (DWORD)frmdata_end.length() + file_size;
	PBYTE data_to_send = (PBYTE)malloc(data_to_send_size + 1);
	ZeroMemory(data_to_send, data_to_send_size + 1);
	memcpy_s(data_to_send, data_to_send_size, frmdata_begin.c_str(), frmdata_begin.length());
	memcpy_s(data_to_send + frmdata_begin.length(), data_to_send_size - frmdata_begin.length(), file_data, file_size);
	memcpy_s(data_to_send + data_to_send_size - frmdata_end.length(), frmdata_end.length(), frmdata_end.c_str(), frmdata_end.length());
	free(file_data);
	if (HttpSendRequestA(site_connection, hdrs.c_str(), (DWORD)hdrs.length(), data_to_send, (DWORD)data_to_send_size) == FALSE)
	{
		free(data_to_send);
		InternetCloseHandle(site_connection);
		InternetCloseHandle(http_connection);
		InternetCloseHandle(internet_handle);
		return_json["success"] = false;
		return_json["error_code"] = 900; //unknown error code
		return return_json;
	}
	free(data_to_send);
	DWORD readed_data_count = 0;
	DWORD data_available = 0;
	PCHAR data_response = NULL;
	do {
		InternetQueryDataAvailable(site_connection, &data_available, 0, 0);
		readed_data_count = data_available + readed_data_count;
		if (readed_data_count == 0)
		{
			InternetCloseHandle(site_connection);
			InternetCloseHandle(http_connection);
			InternetCloseHandle(internet_handle);
			return_json["success"] = false;
			return_json["error_code"] = 900; //unknown error code
			return return_json;
		}
		if (data_available != 0)
		{
			if (data_response == NULL)
			{
				data_response = (PCHAR)malloc(readed_data_count);
				ZeroMemory(data_response, readed_data_count);
			}
			else
			{
				data_response = (PCHAR)realloc(data_response, readed_data_count);
			}
			DWORD data_read = 0;
			InternetReadFile(site_connection, data_response + readed_data_count - data_available, data_available, &data_read);
			if (data_read == 0)
			{
				InternetCloseHandle(site_connection);
				InternetCloseHandle(http_connection);
				InternetCloseHandle(internet_handle);
				return_json["success"] = false;
				return_json["error_code"] = 900; //unknown error code
				return return_json;
			}
		}
	} while (data_available != 0);
	json::JSON obj;
	obj = json::JSON::Load(data_response);
	free(data_response);
	InternetCloseHandle(site_connection);
	InternetCloseHandle(http_connection);
	InternetCloseHandle(internet_handle);
	if (obj.hasKey("success") == true)
		return obj;
	else
	{
		return_json["success"] = false;
		return_json["error_code"] = 900; //unknown error code
		return return_json;
	}

}

