//////////////////////////////////////////////////////////////////////////
//
// WinRegTest.cpp -- by Clinton James origional by Giovanni Dicanio
// 
// Test some of the code in WinReg.hpp
// 
////////////////////////////////////////////////////////////////////////// 

#include "WinReg.hpp"   // Module to test
#include <exception>
#include <iostream>
#include <vector>
using namespace std;
using namespace winreg;

const int kExitError = -1;
const wstring testSubKey = L"SOFTWARE\\WinRegTest";
const DWORD testDw = 0x1234ABCD;
const ULONGLONG testQw = 0xAABBCCDD11223344;
const DWORD nonExistDw = 101038;
const wstring testSz = L"CiaoTestSz";
const wstring testExpandSz = L"%PATH%";
const wstring nonExistSz = L"Jidn";
const vector<BYTE> testBinary{ 0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33 };
const vector<wstring> testMultiSz{ L"Hi", L"Hello", L"Ciao" };
const wstring guidKey = L"{BB82215E-7E92-4156-8D22-E2331878CAFB}";

bool set_privilege(HANDLE hToken, LPCTSTR lpszPrivilege, bool enable)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
		return false;

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
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
		throw RegException{ "Unable to adjust privileges.", 0 };
}



//
// Test SetXxxValue and GetXxxValue methods
// 
const int createdValueCount = 6;
void TestSet(RegKey &key)
{
	key.SetDwordValue(L"TestValueDword", testDw);
	key.SetQwordValue(guidKey, testQw);
	key.SetStringValue(L"TestValueString", testSz);
	key.SetExpandStringValue(L"TestValueExpandString", testExpandSz);
	key.SetMultiStringValue(L"TestValueMultiString", testMultiSz);
	key.SetBinaryValue(L"TestValueBinary", testBinary);
}

int TestGet(RegKey &key)
{
	int failed_tests = 0;
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

	ULONGLONG testQw1 = key.GetQwordValue(guidKey);
	if (testQw1 != testQw)
	{
		wcout << L"RegKey::GetQwordValue failed.\n";
		failed_tests++;
	}

	typeId = key.QueryValueType(guidKey);
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
	// Default value tests
	//

	testDw1 = key.GetDwordValue(L"AbsentValueDword", nonExistDw);
	if (testDw1 != nonExistDw)
	{
		wcout << L"RegKey::GetDwordValue not using defaultValue" << endl;
		failed_tests++;
	}
	testSz1 = key.GetStringValue(L"AbsentString", nonExistSz);
	if (testSz1.compare(nonExistSz) != 0)
	{
		wcout << L"RegKey::GetDwordValue not using defaultValue" << endl;
		failed_tests++;
	}

	return failed_tests;
}

int TestEnumeration(RegKey& key, int expectedSubkeys, int expectedValues)
{
	int failed_tests = 0;
	vector<wstring> subkeys = key.EnumSubKeys();
	if (subkeys.size() != expectedSubkeys)
	{
		wcout << "Found " << subkeys.size() << " subkeys. Expected " << expectedSubkeys << endl;
		failed_tests++;
	}

	vector<ValueInfo> values = key.EnumValues();
	if (values.size() != expectedValues)
	{
		wcout << "Failed to find values";
		wcout << "Found " << values.size() << " values. Expected " << expectedValues << endl;
		failed_tests++;
	}
	return failed_tests;
}

void TestDelete(RegKey& key)
{
	key.DeleteValue(L"TestValueDword");
	key.DeleteValue(guidKey);
	key.DeleteValue(L"TestValueString");
	key.DeleteValue(L"TestValueExpandString");
	key.DeleteValue(L"TestValueMultiString");
	key.DeleteValue(L"TestValueBinary");
}

int main()
{
	constexpr int kExitOk = 0;
	const DWORD totalSetValues = 6;
	int failed_tests = 0;

	RegKey key{ HKEY_CURRENT_USER, testSubKey };
	TestSet(key);
	failed_tests += TestGet(key);

	// Make multiple subkeys
	DWORD total_subkeys = 3;
	for (DWORD i = 0; i < total_subkeys; i++)
	{
		wstring subkeyStr = testSubKey;
		subkeyStr.append(L"\\");
		if (i == 0)
			subkeyStr.append(guidKey);
		else {
			subkeyStr.append(L"key");
			subkeyStr.append(to_wstring(i + 1));
		}
		RegKey subkey{ HKEY_CURRENT_USER, subkeyStr };
		TestSet(subkey);
		failed_tests += TestGet(subkey);
		failed_tests += TestEnumeration(subkey, 0, totalSetValues);
	}
	failed_tests += TestEnumeration(key, total_subkeys, totalSetValues);

	// Query counts of subkeys and values
	DWORD found_values, found_subkeys{ 0 };
	FILETIME filetime;
	key.QueryInfoKey(found_subkeys, found_values, filetime);
	if (found_subkeys != total_subkeys || found_values != totalSetValues)
	{
		wcout << "QueryInfoKey failed" << endl;
		failed_tests++;
	}

	// Recursing down enumeration
	{
		auto subkeys = key.EnumSubKeys();
		for (auto it = subkeys.begin(); it != subkeys.end(); it++) {
			RegKey ekey{ key.Get(), *it };
			failed_tests += TestGet(ekey);
			failed_tests += TestEnumeration(ekey, 0, totalSetValues);
			TestDelete(ekey);
			failed_tests += TestEnumeration(ekey, 0, 0);
			key.DeleteKey(*it, 0);
		}
	}
	key.QueryInfoKey(found_subkeys, found_values, filetime);
	if (found_subkeys != 0 || found_values != totalSetValues)
	{
		wcout << "QueryInfoKey failed after removing subkeys" << endl;
		failed_tests++;
	}

	TestDelete(key);
	key.Close();
	if (key.Get() != nullptr || key.IsValid() || key)
	{
		wcout << "Close failed" << endl;
		failed_tests++;
	}
	//key.Open(HKEY_CURRENT_USER, L"SOFTWARE");
	//key.DeleteKey(L"GioTest", 0);
	if (!failed_tests)
		cout << "Passed" << endl;
	return failed_tests;
}

int TestMounting()
{
	int failed_tests = 0;
	try
	{
		//
		// Test loading and unloading keys
		//
#ifdef SUPPORT_REGGETVALUE
		if (true)
		{
			// TODO save the GioTest to temp file and use to load.
			RegKey key{ HKEY_CURRENT_USER };
			key.LoadPrivate(L"../../curr-config");
			auto allKeys = key.EnumSubKeys();
			if (allKeys.empty())
			{
				wcout << "Unable to enumerate Private" << endl;
				failed_tests++;
			}
		}
#endif
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
			catch (const RegException&) { throw; }
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

