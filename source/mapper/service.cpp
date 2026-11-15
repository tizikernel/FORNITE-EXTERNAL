#include "includes/service.hpp"
#include <Windows.h>
#include <string>
#include <iostream>

#include "includes/utils.hpp"
#include "includes/nt.hpp"

bool service::RegisterAndStart(const std::wstring& driver_path, const std::wstring& serviceName) {
	const static DWORD ServiceTypeKernel = 1;
	const std::wstring servicesPath = L"SYSTEM\\CurrentControlSet\\Services\\" + serviceName;
	const std::wstring nPath = L"\\??\\" + driver_path;

	HKEY dservice;
	LSTATUS status = RegCreateKeyW(HKEY_LOCAL_MACHINE, servicesPath.c_str(), &dservice); 
	if (status != ERROR_SUCCESS) {
		Log("[-] Can't create service key" << std::endl);
		return false;
	}

	status = RegSetKeyValueW(dservice, NULL, L"ImagePath", REG_EXPAND_SZ, nPath.c_str(), (DWORD)(nPath.size()*sizeof(wchar_t)));
	if (status != ERROR_SUCCESS) {
		RegCloseKey(dservice);
		Log("[-] Can't create 'ImagePath' registry value" << std::endl);
		return false;
	}
	
	status = RegSetKeyValueW(dservice, NULL, L"Type", REG_DWORD, &ServiceTypeKernel, sizeof(DWORD));
	if (status != ERROR_SUCCESS) {
		RegCloseKey(dservice);
		Log("[-] Can't create 'Type' registry value" << std::endl);
		return false;
	}
	
	RegCloseKey(dservice);

	HMODULE ntdll = GetModuleHandleA("ntdll.dll");
	if (ntdll == NULL) {
		return false;
	}

	ULONG SE_LOAD_DRIVER_PRIVILEGE = 10UL;
	BOOLEAN SeLoadDriverWasEnabled;
	NTSTATUS Status = nt::RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE, TRUE, FALSE, &SeLoadDriverWasEnabled);
	if (!NT_SUCCESS(Status)) {
		Log("Fatal error: failed to acquire SE_LOAD_DRIVER_PRIVILEGE. Make sure you are running as administrator." << std::endl);
		return false;
	}

	std::wstring wdriver_reg_path = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\" + serviceName;
	UNICODE_STRING serviceStr;
	RtlInitUnicodeString(&serviceStr, wdriver_reg_path.c_str());

	// Disable Vulnerable Driver Blocklist for the load, then restore
	HKEY hKey = NULL;
	DWORD origVal = 1;
	DWORD zero = 0;
	DWORD sz = sizeof(DWORD);
	bool patched = false;

	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
		L"SYSTEM\\CurrentControlSet\\Control\\CI\\Config",
		0, KEY_READ | KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
		if (RegQueryValueExW(hKey, L"VulnerableDriverBlocklistEnable",
			nullptr, nullptr, (BYTE*)&origVal, &sz) == ERROR_SUCCESS) {
			if (origVal != 0) {
				if (RegSetValueExW(hKey, L"VulnerableDriverBlocklistEnable",
					0, REG_DWORD, (BYTE*)&zero, sizeof(DWORD)) == ERROR_SUCCESS) {
					patched = true;
					Log("[+] Temporarily disabled VulnerableDriverBlocklistEnable" << std::endl);
				}
			}
		}
		RegCloseKey(hKey);
	}

	Status = nt::NtLoadDriver(&serviceStr);

	// Restore original value
	if (patched) {
		if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
			L"SYSTEM\\CurrentControlSet\\Control\\CI\\Config",
			0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
			RegSetValueExW(hKey, L"VulnerableDriverBlocklistEnable",
				0, REG_DWORD, (BYTE*)&origVal, sizeof(DWORD));
			RegCloseKey(hKey);
			Log("[+] Restored VulnerableDriverBlocklistEnable" << std::endl);
		}
	}

	Log("[+] NtLoadDriver Status 0x" << std::hex << Status << std::endl);

	if (Status == STATUS_IMAGE_CERT_REVOKED) {
		Log("[-] Your vulnerable driver list is enabled and have blocked the driver loading, you must disable vulnerable driver list to use kdmapper with intel driver" << std::endl);
		Log("[-] Registry path to disable vulnerable driver list: HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\CI\\Config" << std::endl);
		Log("[-] Set 'VulnerableDriverBlocklistEnable' as dword to 0" << std::endl);
	}
	else if (Status == STATUS_ACCESS_DENIED || Status == STATUS_INSUFFICIENT_RESOURCES) {
		Log("[-] Access Denied or Insufficient Resources (0x" << std::hex << Status << "), Probably some anticheat or antivirus running blocking the load of vulnerable driver" << std::endl);
	}

	if (Status == STATUS_IMAGE_ALREADY_LOADED) {
		return true;
	}
	
	return NT_SUCCESS(Status);
}

bool service::StopAndRemove(const std::wstring& serviceName) {
	HMODULE ntdll = GetModuleHandleA("ntdll.dll");
	if (ntdll == NULL)
		return false;

	std::wstring wdriver_reg_path = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\" + serviceName;
	UNICODE_STRING serviceStr;
	RtlInitUnicodeString(&serviceStr, wdriver_reg_path.c_str());

	HKEY driver_service;
	std::wstring servicesPath = L"SYSTEM\\CurrentControlSet\\Services\\" + serviceName;
	LSTATUS status = RegOpenKeyW(HKEY_LOCAL_MACHINE, servicesPath.c_str(), &driver_service);
	if (status != ERROR_SUCCESS) {
		if (status == ERROR_FILE_NOT_FOUND) {
			return true;
		}
		return false;
	}
	RegCloseKey(driver_service);

	NTSTATUS st = nt::NtUnloadDriver(&serviceStr);
	Log("[+] NtUnloadDriver Status 0x" << std::hex << st << std::endl);
	if (st != ERROR_SUCCESS) {
		Log("[-] Driver Unload Failed!!" << std::endl);
		status = RegDeleteTreeW(HKEY_LOCAL_MACHINE, servicesPath.c_str());
		return false; 
	}

	status = RegDeleteTreeW(HKEY_LOCAL_MACHINE, servicesPath.c_str());
	if (status != ERROR_SUCCESS) {
		return false;
	}
	return true;
}
