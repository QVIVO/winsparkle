/*
 *  This file is part of WinSparkle (http://winsparkle.org)
 *
 *  Copyright (C) 2009-2010 Vaclav Slavik
 *  Copyright (C) 2007 Andy Matuschak
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 */

#include "UpdateFile.h"
#include "appcast.h"
#include "ui.h"
#include "error.h"
#include "settings.h"
#include "download.h"
#include "utils.h"

//#include <ctime>
//#include <vector>
//#include <cstdlib>
//#include <algorithm>
#include <tchar.h>

#include "unzip.h"
#include "download.h"



namespace winsparkle
{

/*--------------------------------------------------------------------------*
                             UpdateChecker::Run()
 *--------------------------------------------------------------------------*/

UpdateFile::UpdateFile(const char* filename)
: Thread("WinSparkle updates app")
, m_zipFileUrl(filename)
{
}

bool UpdateFile::DownloadZipFromFeedUrl()
{
	StringDownloadSink downloadZip;
	DownloadFile(m_zipFileUrl, &downloadZip, 1);

	std::string baseUrl("http://localhost:47688/updates/win32/");
	std::string downloadFilename = m_zipFileUrl.substr(baseUrl.length(), m_zipFileUrl.length() - baseUrl.length());

	FILE * pFile;
	fopen_s ( &pFile, downloadFilename.c_str() , "wb" );

	size_t size_write = fwrite(downloadZip.data.c_str(), 1, downloadZip.data.size(), pFile);
	fclose(pFile);

	return true;
}

void UpdateFile::CloseAppFromFeedAppId()
{
    const std::string appId = Settings::GetAppId();
    
	if ( appId.empty() )
        throw std::runtime_error("App ID not specified.");

	int nLen = strlen(appId.c_str()) + 1; 
    int nwLen = MultiByteToWideChar(CP_ACP, 0, appId.c_str(), nLen, NULL, 0);

	TCHAR lpszFile[256]; 
    MultiByteToWideChar(CP_ACP, 0, appId.c_str(), nLen, lpszFile, nwLen); 

	HWND qWin = ::FindWindow( lpszFile, 0);
	UINT uMessage = WM_CLOSE;
	WPARAM wParam = 0;
	LPARAM lParam = 0;

	SendMessage( qWin, uMessage, wParam, lParam );
}

std::wstring GetExecutablePath()
{
	static TCHAR buffer[MAX_PATH];
	DWORD res = GetModuleFileName(NULL, buffer, MAX_PATH-1);

	std::wstring str(buffer);
	size_t pos = str.find_last_of('\\');

	if (pos != std::string::npos)
	{
		return str.substr(0, pos);
	}
	else
	{
		return str;
	}
}

void UpdateFile::UpdateApp()
{
	int ret = 0;

	std::string baseUrl("http://localhost:47688/updates/win32/");
	std::string downloadFilename = m_zipFileUrl.substr(baseUrl.length(), m_zipFileUrl.length() - baseUrl.length());

	unzFile zip = unzOpen( downloadFilename.c_str() );

	if ( zip == NULL ) 
	{
		printf("Error opening unzOpen\n");
		return;
	}

	std::string exeuteFilename;


	ret = unzGoToFirstFile(zip);

	while( ret == UNZ_OK )
	{
		unz_file_info FileInfo;   
		char *szName=NULL;   
		memset(&FileInfo,0,sizeof(FileInfo));   
	   
		int nRet=unzGetCurrentFileInfo(zip,&FileInfo,NULL,0,NULL,0,NULL,0); 
		if (nRet!=UNZ_OK) continue;   
	   
		// Allocate space for the filename    
		szName=(char *)malloc(FileInfo.size_filename+1); if (szName==NULL) continue;   
		nRet=unzGetCurrentFileInfo(zip,&FileInfo,szName,FileInfo.size_filename+1,NULL,0,NULL,0);   

		FILE * file;
		fopen_s ( &file, szName , "wb" );

		nRet=unzOpenCurrentFile(zip); 
		if (nRet!=UNZ_OK) return;   

		unsigned int len = FileInfo.uncompressed_size;
		char* buf = NULL;
		buf=(char *)malloc(len);

		nRet=unzReadCurrentFile(zip, buf, len);   

		fwrite(buf, 1, len, file);
		fclose(file);

		ret = unzGoToNextFile(zip);

		exeuteFilename = szName;

		delete szName;
		szName = NULL;

		delete buf;
		buf = NULL;
	}

	unzClose(zip);

	int nLen = strlen(exeuteFilename.c_str()) + 1; 
    int nwLen = MultiByteToWideChar(CP_ACP, 0, exeuteFilename.c_str(), nLen, NULL, 0);

	TCHAR lpszFile[256]; 
    MultiByteToWideChar(CP_ACP, 0, exeuteFilename.c_str(), nLen, lpszFile, nwLen); 

	TCHAR path[MAX_PATH] = {0};

	wsprintf(path, L"%s\\%s", GetExecutablePath().c_str(), lpszFile);
	//wsprintf(path, L"%s\\QVIVO_2.0.31.exe", GetExecutablePath().c_str());

	ShellExecute(NULL, _T("open"), path, NULL, NULL, SW_SHOWNORMAL);
}

void UpdateFile::Run()
{
    // no initialization to do, so signal readiness immediately

    SignalReady();

    try
    {
		if(DownloadZipFromFeedUrl())
		{
			UpdateApp();
			CloseAppFromFeedAppId();
		}
    }
    catch ( ... )
    {
        UI::NotifyUpdateError();
        throw;
    }
}

} // namespace winsparkle
