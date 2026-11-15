#include "Backend.h"
#include <Windows.h>
#include <winhttp.h>
#include <wincrypt.h>
#include <string>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "crypt32.lib")

namespace Backend
{
	static std::string g_lastError = "Ingresa tu key.";

	std::string LastError()
	{
		return g_lastError;
	}

	std::string MakeDeviceFingerprint()
	{
		wchar_t computer[256]{};
		DWORD computerSize = static_cast<DWORD>(std::size(computer));
		GetComputerNameW(computer, &computerSize);
		wchar_t windowsDir[MAX_PATH]{};
		GetWindowsDirectoryW(windowsDir, static_cast<UINT>(std::size(windowsDir)));
		std::wstring source = std::wstring(computer) + L"|" + windowsDir + L"|FORNITE";

		HCRYPTPROV provider = 0;
		HCRYPTHASH hash = 0;
		BYTE digest[32]{};
		DWORD digestSize = sizeof(digest);
		if (!CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) ||
			!CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash) ||
			!CryptHashData(hash, reinterpret_cast<const BYTE*>(source.data()), static_cast<DWORD>(source.size() * sizeof(wchar_t)), 0) ||
			!CryptGetHashParam(hash, HP_HASHVAL, digest, &digestSize, 0))
		{
			if (hash) CryptDestroyHash(hash);
			if (provider) CryptReleaseContext(provider, 0);
			return "fornite-" + std::to_string(GetCurrentProcessId());
		}

		std::string result;
		static const char hex[] = "0123456789abcdef";
		result.reserve(64);
		for (BYTE b : digest) {
			result.push_back(hex[b >> 4]);
			result.push_back(hex[b & 0x0f]);
		}
		CryptDestroyHash(hash);
		CryptReleaseContext(provider, 0);
		return result;
	}

	bool ValidateKeyOnline(const std::string& key)
	{
		if (key.size() < 12 || key.size() > 64) {
			g_lastError = "La key no tiene un formato valido.";
			return false;
		}

		HINTERNET session = WinHttpOpen(L"UNKNOWN-TEAM-FORNITE/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, nullptr, nullptr, 0);
		if (!session) {
			g_lastError = "No se pudo iniciar la conexion segura.";
			return false;
		}
		WinHttpSetTimeouts(session, 5000, 5000, 8000, 8000);

		HINTERNET connection = WinHttpConnect(session, L"key-admin-console.onrender.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
		HINTERNET request = connection
			? WinHttpOpenRequest(connection, L"POST", L"/api/validate", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE)
			: nullptr;

		const std::string fingerprint = MakeDeviceFingerprint();
		std::string body = std::string("{\"product\":\"FORNITE\",\"keyCode\":\"") + key +
			"\",\"deviceFingerprint\":\"" + fingerprint + "\",\"deviceLabel\":\"FORNITE Windows\"}";

		bool valid = false;
		if (request)
		{
			const wchar_t headers[] = L"Content-Type: application/json\r\nAccept: application/json\r\n";
			if (WinHttpSendRequest(request, headers, static_cast<DWORD>(-1L), (LPVOID)body.data(), static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0) &&
				WinHttpReceiveResponse(request, nullptr))
			{
				std::string response;
				DWORD available = 0;
				while (WinHttpQueryDataAvailable(request, &available) && available)
				{
					std::string chunk(available, '\0');
					DWORD read = 0;
					if (!WinHttpReadData(request, chunk.data(), available, &read))
						break;
					response.append(chunk.data(), read);
				}
				valid = response.find("\"valid\":true") != std::string::npos;
				if (!valid) {
					if (response.find("device_mismatch") != std::string::npos)
						g_lastError = "Esta key ya esta vinculada a otro dispositivo.";
					else if (response.find("expired") != std::string::npos)
						g_lastError = "Tu key esta vencida.";
					else
						g_lastError = "Key invalida, suspendida o vencida.";
				}
			}
			else {
				g_lastError = "No se pudo validar la key. Revisa tu conexion.";
			}
		}
		else {
			g_lastError = "No se pudo abrir la conexion segura al panel.";
		}

		if (request) WinHttpCloseHandle(request);
		if (connection) WinHttpCloseHandle(connection);
		WinHttpCloseHandle(session);
		return valid;
	}
}
