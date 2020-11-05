﻿// ConsoleApplication79.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <pdh.h>

PDH_STATUS GetRegistrySize(LPTSTR szMachineName,
    LPDWORD lpdwCurrentSize, LPDWORD lpdwMaximumSize);

//*******************************************************************
// 
//  Entry point for the program. This function demonstrates how to
//  use the GetRegistrySize function implemented below.
// 
//  It will use the first argument, if present, as the name of the
//  computer whose registry you wish to determine. If unspecified,
//  it will use the local computer.
//*******************************************************************

int _tmain(int argc, TCHAR* argv[])
{

    LPTSTR      szMachineName = NULL;
    PDH_STATUS  pdhStatus = 0;
    DWORD       dwCurrent = 0;
    DWORD       dwMaximum = 0;

    // Allow a computer name to be specified on the command line.
    if (argc > 1)
        szMachineName = argv[1];

    // Get the registry size.
    pdhStatus = GetRegistrySize(szMachineName, &dwCurrent, &dwMaximum);

    // Print the results.
    if (pdhStatus == ERROR_SUCCESS)
    {
        _tprintf(TEXT("Registry size: %ld bytes\n"), dwCurrent);
        _tprintf(TEXT("Max registry size: %ld bytes\n"), dwMaximum);

    }
    else
    {
        // If the operation failed, print the PDH error message.
        LPTSTR szMessage = NULL;

        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_HMODULE,
            GetModuleHandle(TEXT("PDH.DLL")), pdhStatus,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            szMessage, 0, NULL);

        _tprintf(TEXT("GetRegistrySize failed:  %s"), szMessage);

        LocalFree(szMessage);
    }

    return 0;
}

//*******************************************************************
// 
//  Retrieves the current and maximum registry size. It gets this
//  information from the raw counter values for the "% Registry Quota 
//  In Use" performance counter within the System object.
// 
//  PARAMETERS:   
//      szMachineName - Null-terminated string that specifies the
//          name of the computer whose registry you wish to query.
//          If this parameter is NULL, the local computer is used.
// 
//      lpdwCurrentSize - Receives the current registry size.
// 
//      lpdwMaximumSize - Receives the maximum registry size.
// 
//  RETURN VALUE: 
//      ERROR_SUCCESS if successful. Otherwise, the function
//      returns a PDH error code. These error codes can be
//      found in PDHMSG.H. A textual error message can be
//      retrieved from PDH.DLL using the FormatMessage function.
// 
//******************************************************************

PDH_STATUS GetRegistrySize(LPTSTR szMachineName,
    LPDWORD lpdwCurrentSize, LPDWORD lpdwMaximumSize)
{
    PDH_STATUS  pdhResult = 0;
    TCHAR       szCounterPath[1024];
    DWORD       dwPathSize = 1024;
    PDH_COUNTER_PATH_ELEMENTS pe;
    PDH_RAW_COUNTER pdhRawValues;
    HQUERY      hQuery = NULL;
    HCOUNTER    hCounter = NULL;
    DWORD       dwType = 0;

    // Open PDH query
    pdhResult = PdhOpenQuery(NULL, 0, &hQuery);
    if (pdhResult != ERROR_SUCCESS)
        return pdhResult;

    __try
    {
        // Create counter path
        pe.szMachineName = szMachineName;
        pe.szObjectName = (LPWSTR)TEXT("System");
        pe.szInstanceName = NULL;
        pe.szParentInstance = NULL;
        pe.dwInstanceIndex = 1;
        pe.szCounterName = (LPWSTR)TEXT("% Registry Quota In Use");       //在此处两个地方添加了强制转换，否则出现报错："const wchar_t *" 类型的值分配到 "LPWSTR" 类型的实体

        pdhResult = PdhMakeCounterPath(&pe, szCounterPath,
            &dwPathSize, 0);
        if (pdhResult != ERROR_SUCCESS)
            __leave;

        // Add the counter to the query
        pdhResult = PdhAddCounter(hQuery, szCounterPath, 0, &hCounter);
        if (pdhResult != ERROR_SUCCESS)
            __leave;

        // Run the query to collect the performance data
        pdhResult = PdhCollectQueryData(hQuery);
        if (pdhResult != ERROR_SUCCESS)
            __leave;

        // Retrieve the raw counter data:
        //    The dividend (FirstValue) is the current registry size
        //    The divisor (SecondValue) is the maximum registry size
        ZeroMemory(&pdhRawValues, sizeof(pdhRawValues));
        pdhResult = PdhGetRawCounterValue(hCounter, &dwType,
            &pdhRawValues);
        if (pdhResult != ERROR_SUCCESS)
            __leave;

        // Store the sizes in variables.
        if (lpdwCurrentSize)
            *lpdwCurrentSize = (DWORD)pdhRawValues.FirstValue;

        if (lpdwMaximumSize)
            *lpdwMaximumSize = (DWORD)pdhRawValues.SecondValue;

    }
    __finally
    {
        // Close the query
        PdhCloseQuery(hQuery);
    }

    return 0;
}