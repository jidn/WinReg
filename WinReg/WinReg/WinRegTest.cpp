//////////////////////////////////////////////////////////////////////////
//
// WinRegTest.cpp -- by Giovanni Dicanio
// 
// Test some of the code in WinReg.hpp
// 
// NOTE --- Test Preparation ---
// In the folder containing this source file, there should be also a file 
// "GioTest.reg". This REG file contains some initial data to load into 
// the registry for this test.
// 
////////////////////////////////////////////////////////////////////////// 

#include "WinReg.hpp"   // Module to test
#include <exception>
#include <iostream>
#include <vector>
using namespace std;
using namespace winreg;

bool set_privilege( HANDLE hToken, LPCTSTR lpszPrivilege, bool enable ) 
{
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if ( !LookupPrivilegeValue( NULL, lpszPrivilege, &luid ) )
		return false; 

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

	if ( !AdjustTokenPrivileges( hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES) NULL, (PDWORD) NULL) )
		return false; 

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
		return false;

	return true;
}

void adjust_privileges()
{
	HANDLE hToken;
	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
	if (!set_privilege(hToken, SE_BACKUP_NAME, TRUE)
		|| !set_privilege(hToken, SE_RESTORE_NAME, TRUE))
		throw RegException { "Unable to adjust privileges.", 0 };
}


int main()
{
    constexpr int kExitOk = 0;
    constexpr int kExitError = 1;
	int failed_tests = 0;

    try 
    {
        wcout << L"=========================================\n";
        wcout << L"*** Testing Giovanni Dicanio's WinReg ***\n";
        wcout << L"=========================================\n\n";

        //
        // Test subkey and value enumeration
        // 
		
        const wstring testSubKey = L"SOFTWARE\\GioTest";
        RegKey key{ HKEY_CURRENT_USER, testSubKey };    
        
        vector<wstring> subKeyNames = key.EnumSubKeys();
		if (subKeyNames.empty())
		{
			wcout << "Failed to find subkeys";
			failed_tests++;
		}
        //wcout << L"Subkeys:\n";
        //for (const auto& s : subKeyNames)
        //{
        //    wcout << L"  [" << s << L"]\n";
        //}
        //wcout << L'\n';

        vector<ValueInfo> values = key.EnumValues();
		if (values.empty())
		{
			wcout << "Failed to find values";
			failed_tests++;
		}
        //wcout << L"Values:\n";
        //for (const auto& v : values)
        //{
        //    wcout << L"  [" << v.first << L"](" << RegKey::RegTypeToString(v.second) << L")\n";
        //}
        //wcout << L'\n';
        key.Close();


        //
        // Test SetXxxValue and GetXxxValue methods
        // 
        
        key.Open(HKEY_CURRENT_USER, testSubKey);
        
        const DWORD testDw = 0x1234ABCD;
        const ULONGLONG testQw = 0xAABBCCDD11223344;
        const wstring testSz = L"CiaoTestSz";
        const wstring testExpandSz = L"%PATH%";
        const vector<BYTE> testBinary{0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33};
        const vector<wstring> testMultiSz{ L"Hi", L"Hello", L"Ciao" };

        key.SetDwordValue(L"TestValueDword", testDw);
        key.SetQwordValue(L"TestValueQword", testQw);
        key.SetStringValue(L"TestValueString", testSz);
        key.SetExpandStringValue(L"TestValueExpandString", testExpandSz);
        key.SetMultiStringValue(L"TestValueMultiString", testMultiSz);
        key.SetBinaryValue(L"TestValueBinary", testBinary);

        DWORD testDw1 = key.GetDwordValue(L"TestValueDword");
        if (testDw1 != testDw) 
        {
            wcout << L"RegKey::GetDwordValue failed.\n";
			failed_tests++;
        }

        DWORD typeId = key.QueryValueType(L"TestValueDword");
        if (typeId != REG_DWORD)
        {
            wcout << L"RegKey::QueryValueType failed for REG_DWORD.\n";
			failed_tests++;
        }

        ULONGLONG testQw1 = key.GetQwordValue(L"TestValueQword");
        if (testQw1 != testQw)
        {
            wcout << L"RegKey::GetQwordValue failed.\n";
			failed_tests++;
        }

        typeId = key.QueryValueType(L"TestValueQword");
        if (typeId != REG_QWORD)
        {
            wcout << L"RegKey::QueryValueType failed for REG_QWORD.\n";
			failed_tests++;
        }

        wstring testSz1 = key.GetStringValue(L"TestValueString");
        if (testSz1 != testSz) 
        {
            wcout << L"RegKey::GetStringValue failed.\n";
			failed_tests++;
        }

        typeId = key.QueryValueType(L"TestValueString");
        if (typeId != REG_SZ)
        {
            wcout << L"RegKey::QueryValueType failed for REG_SZ.\n";
			failed_tests++;
        }

        wstring testExpandSz1 = key.GetExpandStringValue(L"TestValueExpandString");
        if (testExpandSz1 != testExpandSz)
        {
            wcout << L"RegKey::GetExpandStringValue failed.\n";
			failed_tests++;
        }

        typeId = key.QueryValueType(L"TestValueExpandString");
        if (typeId != REG_EXPAND_SZ)
        {
            wcout << L"RegKey::QueryValueType failed for REG_EXPAND_SZ.\n";
			failed_tests++;
        }

        vector<wstring> testMultiSz1 = key.GetMultiStringValue(L"TestValueMultiString");
        if (testMultiSz1 != testMultiSz)
        {
            wcout << L"RegKey::GetMultiStringValue failed.\n";
			failed_tests++;
        }

        typeId = key.QueryValueType(L"TestValueMultiString");
        if (typeId != REG_MULTI_SZ)
        {
            wcout << L"RegKey::QueryValueType failed for REG_MULTI_SZ.\n";
			failed_tests++;
        }

        vector<BYTE> testBinary1 = key.GetBinaryValue(L"TestValueBinary");
        if (testBinary1 != testBinary)
        {
            wcout << L"RegKey::GetBinaryValue failed.\n";
			failed_tests++;
        }

        typeId = key.QueryValueType(L"TestValueBinary");
        if (typeId != REG_BINARY)
        {
            wcout << L"RegKey::QueryValueType failed for REG_BINARY." << endl;
			failed_tests++;
        }


        //
        // Remove some test values
        //
        
        key.DeleteValue(L"TestValueDword");
        key.DeleteValue(L"TestValueQword");
        key.DeleteValue(L"TestValueString");
        key.DeleteValue(L"TestValueExpandString");
        key.DeleteValue(L"TestValueMultiString");
        key.DeleteValue(L"TestValueBinary");

		//
		// Test loading and unloading keys
		//

		if (true)
		{
			RegKey key{ HKEY_CURRENT_USER };
			key.LoadPrivate(L"../../curr-config");
			auto allKeys = key.EnumSubKeys();
			if (allKeys.empty())
			{
				wcout << "Unable to enumerate Private" << endl;
				failed_tests++;
			}
		}
		if (true)
		{
			adjust_privileges();

			const wstring testSubKey = L"swimage-worker";
			try {
				RegKey key{ HKEY_USERS };

				key.LoadKey(testSubKey, L"../../test");
				auto allKeys = key.EnumSubKeys();
				if (allKeys.empty())
				{
					wcout << "Enumerate subkeys on loaded key" << endl;
					failed_tests++;
				}
				auto allValues = key.EnumValues();
				if (allValues.empty())
				{
					wcout << "Enumerate values on loaded key" << endl;
					failed_tests++;
				}
			} 
			catch (const RegException& ) { throw; }
			try {
				// This loaded registry should have been removed.
				RegKey key{ HKEY_USERS, testSubKey };
				wcout << "Loaded key [" << testSubKey << "] persists after object destruction." << endl;
				failed_tests++;
			}
			catch (const RegException&)
			{
				// Exception expected.  The loaded keys was removed in the destructor.
			}
		}


		if (failed_tests == 0)
			wcout << L"All right!! :)" << endl;
    }
    catch (const RegException& e)
    {
        cout << "\n*** Registry Exception: " << e.what();
        cout << "\n*** [Windows API error code = " << e.ErrorCode() << endl << endl;
        return kExitError;
    }
    catch (const exception& e)
    {
        cout << "\n*** ERROR: " << e.what() << endl;
        return kExitError;
    }

    return failed_tests;
}
