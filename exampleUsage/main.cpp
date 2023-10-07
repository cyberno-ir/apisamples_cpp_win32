#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <sstream>
#include <vector>
#include <string>
#include <Shlwapi.h>
#include "..\CPPLib\Header.h"

//clear screen:
void cls() {
    COORD topLeft = {0, 0};
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO screen;
    DWORD written;
    GetConsoleScreenBufferInfo(console, &screen);
    FillConsoleOutputCharacterA(console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written);
    FillConsoleOutputAttribute(console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
                               screen.dwSize.X * screen.dwSize.Y, topLeft, &written);
    SetConsoleCursorPosition(console, topLeft);
}

// for string delimiter
std::vector<std::string> split(std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}

void main() {
    // Print banner
    std::cout << R"( ______ ____    ____ .______    _______. ______      .__   __.   ______   
/      |\   \  /   / |   _  \  |   ____||   _  \     |  \ |  |  /  __  \  
| ,----' \   \/   /  |  |_)  | |  |__   |  |_)  |    |   \|  | |  |  |  | 
| |       \_    _/   |   _  <  |   __|  |      /     |. ` |  | |  |  |  |
| `----.    |  |     |  |_)  | |  |____ |  |\  \----.|  |\   | |  `--'  | 
\______|    |__|     |______/  |_______|| _| `._____||__| \__|  \______/  
)";

	std::string identifier = "", password = "", serveraddress = "", file_path = "", avs = "", guid;
    json::JSON scan_init_response, login_response, password_extract_response, scan_response, scan_result_response;
    std::cout << "Please insert API server address [Default=https://multiscannerdemo.cyberno.ir]: ";
    std::getline(std::cin, serveraddress);
    if (serveraddress.length() == 0)
        serveraddress = "https://multiscannerdemo.cyberno.ir";
    std::cout << "Please insert identifier (email): ";
    std::getline(std::cin, identifier);
    std::cout << "Please insert your password: ";
    std::getline(std::cin, password);
    CyUtils *cyutils = new CyUtils(serveraddress);
    json::JSON input_json;
    input_json["email"] = identifier;
    input_json["password"] = password;
    login_response = cyutils->call_with_json_input("user/login", input_json);
    if (login_response["success"].ToBool() == true)
        std::cout << "You are logged in successfully." << std::endl;
    else {
        std::cout << cyutils->get_error(login_response);
        _getch();
        return;
    }
    std::string apikey = login_response["data"].ToString();
    int index;
    std::cout << "Please select scan mode:\n1- Scan local folder\n2- Scan file\nEnter Number=";
    std::cin >> index;
    if (index == 1) {

        // Prompt user to enter file paths
        std::cout << "Please enter the paths of file to scan (with spaces): ";
        std::getchar();
        std::getline(std::cin, file_path);
        std::string delimiter = " ";
        std::vector<std::string> file_path_array = split(file_path, delimiter);


        // Prompt user to enter antivirus names
        std::cout << "Enter the name of the selected antivirus (with spaces): ";
        std::getline(std::cin, avs);
        std::vector<std::string> avs_array = split(avs, delimiter);

        // Make Json item
        json::JSON scan_item_json;
        scan_item_json["token"] = apikey;
        for (int i = 0; i < avs_array.size(); i++) {
            scan_item_json["avs"][i] = avs_array[i];
        }
        for (int i = 0; i < file_path_array.size(); i++) {
            scan_item_json["paths"][i] = file_path_array[i];
        }
        scan_init_response = cyutils->call_with_json_input("scan/init", scan_item_json);
    } else {
        // Initialize scan
        json::JSON scan_item_json;
        std::cout << "Please enter the path of file to scan:\n";
        std::getchar();
        std::getline(std::cin, file_path);
        std::cout << "Enter the name of the selected antivirus (with spaces): ";
        std::getline(std::cin, avs);
        scan_item_json["token"] = apikey;
        scan_item_json["avs"] = avs;
        scan_init_response = cyutils->call_with_form_input("scan/multiscanner/init", scan_item_json, "file", file_path);
		
    }
	guid = scan_init_response["guid"].ToString();
    if (scan_init_response["success"].ToBool() == true) {
        // Get scan response
        int password_protected = scan_init_response["password_protected"].length();
        // Check if password-protected
        if (password_protected > 0) {
            json::JSON password_item_json;
			for (int i = 0; i <scan_init_response["password_protected"].length(); i++) {
				std::string password = "";
				std::cout << "|Enter the Password file -> " << scan_init_response["password_protected"][i] << " |: ";
				std::cin >> password;
				password_item_json["password"] = password;
				password_item_json["token"] = apikey;
				password_item_json["path"] = scan_init_response["password_protected"][i];
				cyutils->call_with_json_input("scan/extract/" + guid, password_item_json);
			}
		}
        // Start scan
        std::cout << "=========  Start Scan ===========" << std::endl;
        json::JSON scan_json;
		scan_json["token"] = apikey;
		json::JSON scan_response = cyutils->call_with_json_input("scan/start/" + guid, scan_json);
        // Wait for scan results
        if (scan_response["success"].ToBool() == true) {
            bool is_finished = false;
            while (!is_finished) {
                std::cout << "Waiting for result...\n";
				input_json["token"] = apikey;
                scan_result_response = cyutils->call_with_json_input("scan/result/" + guid, input_json);
                if (scan_result_response["data"]["finished_at"].ToInt() != 0) {
                    is_finished = true;
                    std::cout << scan_result_response["data"] << std::endl;
                }
                Sleep(5000);
            }
        } else {
            std::cout << cyutils->get_error(scan_response) << std::endl;
        }
        return;
    } else {
        std::cout << cyutils->get_error(scan_init_response);
    }
}