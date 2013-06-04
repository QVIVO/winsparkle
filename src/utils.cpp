#include "utils.h"

#include <string>


#include <Windows.h>






namespace winsparkle
{




	std::string ConvertWideCharToMultiByte(const wchar_t* wstr)
	{
        int size = WideCharToMultiByte( CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL );

        char *buffer = new char[ size+1];
        WideCharToMultiByte( CP_UTF8, 0, wstr, -1, buffer, size, NULL, NULL );
		buffer[size] = '\0';

        std::string str( buffer);
        delete [] buffer;

        return str;
	}

	std::wstring ConvertMultiByteToWideChar(const std::string& str)
	{
		int wlen = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), strlen(str.c_str()) + 1, NULL, 0);
		wchar_t* wbuffer = new wchar_t[wlen];
		::MultiByteToWideChar(CP_UTF8, 0, str.c_str(),  strlen(str.c_str()) + 1, wbuffer, wlen);

		std::wstring wstr(wbuffer);
		delete[] wbuffer;

		return wstr;
	}


}