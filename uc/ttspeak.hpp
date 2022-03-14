#ifndef TTSPEAK_HPP
#define TTSPEAK_HPP

#pragma warning(disable : 4996)
#pragma warning(disable : 5054)

#include <sphelper.h>
#include <atlbase.h>
#include <atlcom.h>
#include <sapi.h>
#include <string>
extern CComModule _Module;

class CTTSpeak
{
private:
	unsigned int MAX_ARR_SIZE{};
	wchar_t* LocalText = NULL;
	HRESULT Result = NULL;
	ISpVoice* Voice = nullptr;
	CComPtr<ISpObjectToken> ObjectToken;
	void CheckOut(const HRESULT&);
public:
	CTTSpeak();
	~CTTSpeak();
	void LoadComSpeak(int);
	void ComSpeak(LPCWSTR, long SpeechRate = 2);
};

CTTSpeak::CTTSpeak()
{
	if (FAILED(::CoInitialize(NULL)))
		throw std::exception("Unable to Initilize COM object.");
}

CTTSpeak::~CTTSpeak(void)
{
	::CoUninitialize();
	if (Voice != NULL)
		Voice->Release();
	Voice = NULL;
	delete[] LocalText;
	return;
}

void CTTSpeak::LoadComSpeak(int ArraySize)
{
	// MAX_ARR_SIZE is defined as the maximum input string literal your
	// users can input. Users will be unable to exceed this length.
	MAX_ARR_SIZE = ArraySize;
	LocalText = new wchar_t[MAX_ARR_SIZE];

	// Instance creation for SVoice (SAPI).
	Result = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL,
		IID_ISpVoice, (void**)&Voice);
	CheckOut(Result);

	SpFindBestToken(SPCAT_VOICES, L"gender = female", L" ", &ObjectToken);

	Result = Voice->SetVoice(ObjectToken);
	CheckOut(Result);
}

void CTTSpeak::ComSpeak(LPCWSTR Input, long SpeechRate)
{
	if (wcslen(Input) >= MAX_ARR_SIZE)
	{
		printf("Input too large for MAX_ARR_SIZE: %i", MAX_ARR_SIZE);
		return;
	}

	// Set speech rate
	Voice->SetRate(SpeechRate);

	// Make next speech instant
	Voice->SetPriority(SPVPRI_ALERT);

	Result = Voice->Speak(Input, SPF_IS_XML | SPF_ASYNC, NULL);
	CheckOut(Result);

	return;
}

void CTTSpeak::CheckOut(const HRESULT& hrVal)
{
	std::string retVal = "";
	switch (hrVal)
	{
	case S_OK: return;

	case E_ABORT: retVal = "Process was aborted."; break;

	case E_ACCESSDENIED: retVal = "Access was denied."; break;

	case E_FAIL: retVal = "Process failed."; break;

	case E_HANDLE: retVal = "Handle was invalid."; break;

	case E_INVALIDARG: retVal = "Argument was invalid."; break;

	case E_NOINTERFACE:	retVal = "No such interface supported."; break;

	case E_NOTIMPL:	retVal = "Function not implemented."; break;

	case E_OUTOFMEMORY:retVal = "Out of memory."; break;

	case E_POINTER:	retVal = "Invlaid pointer."; break;

	case E_UNEXPECTED:retVal = "Unexpected failure."; break;

	default: retVal = "Unknown Error: " + std::to_string(hrVal);
	}
	throw std::exception(retVal.c_str());
	return;
}

#endif