#include <Windows.h>
#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp"
#include "skCrypter.hpp"
#include "ttspeak.hpp"

extern "C" 
{
	#include "logitech/mouse.h"
}

namespace Global
{
	constexpr double Sensitivity{ 0.16 };
	constexpr double FOV{ 90.0 };
	double TimerResolution{ 0.0 };
}

class CVector2
{
public:
	double X, Y;

	CVector2 operator - (auto other)
	{
		return {
			X - other,
			Y - other
		};
	}

	CVector2 operator + (auto other)
	{
		return {
			X + other,
			Y + other
		};
	}

	CVector2 operator * (auto other)
	{
		return {
			X * other,
			Y * other
		};
	}

	CVector2 operator / (auto other)
	{
		return {
			X / other,
			Y / other
		};
	}

	CVector2 operator - (CVector2 other)
	{
		return {
			X - other.X,
			Y - other.Y
		};
	}

	CVector2 operator + (CVector2 other)
	{
		return {
			X + other.X,
			Y + other.Y
		};
	}

	CVector2 operator * (CVector2 other)
	{
		return {
			X * other.X,
			Y * other.Y
		};
	}

	CVector2 operator / (CVector2 other)
	{
		return {
			X / other.X,
			Y / other.Y
		};
	}

	void operator += (CVector2 other)
	{
		// Update this vector's values
		X += other.X;
		Y += other.Y;
	}

	void operator -= (CVector2 other)
	{
		// Update this vector's values
		X -= other.X;
		Y -= other.Y;
	}

	void operator *= (CVector2 other)
	{
		// Update this vector's values
		X *= other.X;
		Y *= other.Y;
	}

	void operator /= (CVector2 other)
	{
		// Update this vector's values
		X /= other.X;
		Y /= other.Y;
	}
};

class CTimer
{
private:
	LARGE_INTEGER StartTime;

public:
	CTimer()
	{
		QueryPerformanceCounter(&StartTime);
	}

	void Update()
	{
		QueryPerformanceCounter(&StartTime);
	}

	double Elapsed()
	{
		LARGE_INTEGER CurrentTime, ElapsedQPC, Frequency;
		QueryPerformanceFrequency(&Frequency);
		QueryPerformanceCounter(&CurrentTime);
		ElapsedQPC.QuadPart = CurrentTime.QuadPart - StartTime.QuadPart;

		double ElapsedTime = ((static_cast<double>(ElapsedQPC.QuadPart) * 1000.0) /
			(static_cast<double>(Frequency.QuadPart)));

		return ElapsedTime;
	}
};

class CWeapon
{
public:
	double RepeatDelay{ };
	int MagazineCapacity{ };
	double StancePenalty{ };
	std::string Name{ };
	double FovOffset{ };
	double ZoomFactor{ };
	int Automatic{ };
	std::vector<CVector2> Pattern{ };
	bool Curves{ };
}; std::vector<CWeapon> Weapon;

class CAttachment
{
public:
	std::string Name{ };
	double FovOffset{ };
	double ZoomFactor{ };
	double PatternMultiplier{ };
	double RepeatDelayMultiplier{ };
}; std::vector<CAttachment> Attachment;

int Dialog(LPCWSTR Message)
{
	return MessageBox(GetConsoleWindow(), Message, skCrypt(L"Error"), MB_OK | MB_ICONHAND);
}

bool SetTimerResolution()
{
	#define STATUS_SUCCESS 0x00000000
	constexpr double NanoToMilli{ 0.0001 };
	static NTSTATUS(__stdcall * NtQueryTimerResolution)(OUT PULONG MinimumResolution, OUT PULONG MaximumResolution, OUT PULONG CurrentResolution) = (NTSTATUS(__stdcall*)(PULONG, PULONG, PULONG))GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtQueryTimerResolution");
	static NTSTATUS(__stdcall * NtSetTimerResolution)(IN ULONG RequestedResolution, IN BOOLEAN Set, OUT PULONG ActualResolution) = (NTSTATUS(__stdcall*)(ULONG, BOOLEAN, PULONG))GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtSetTimerResolution");

	if ((NtQueryTimerResolution == nullptr) || (NtSetTimerResolution == nullptr))
		return false;

	ULONG MinimumResolution{}, MaximumResolution{}, CurrentResolution{};
	if (NtQueryTimerResolution(&MinimumResolution, &MaximumResolution, &CurrentResolution) != STATUS_SUCCESS)
		return false;

	if (NtSetTimerResolution(min(MinimumResolution, MaximumResolution), TRUE, &CurrentResolution) != STATUS_SUCCESS)
		return false;

	Global::TimerResolution = min(MinimumResolution, MaximumResolution) * NanoToMilli;

	return true;
}

static NTSTATUS(__stdcall* NtDelayExecution)(IN BOOLEAN Alertable, IN PLARGE_INTEGER DelayInterval) = (NTSTATUS(__stdcall*)(BOOLEAN, PLARGE_INTEGER))GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtDelayExecution");
void AccurateSleep(double Delay, CTimer& Excess)
{
	CTimer Timer;
	constexpr double MilliToNano{ 5000.0 };
	LARGE_INTEGER DelayInterval{};

	if (Delay > Excess.Elapsed())
		Delay -= Excess.Elapsed();

	DelayInterval.QuadPart = static_cast<LONGLONG>(-Delay * MilliToNano);
	NtDelayExecution(FALSE, &DelayInterval);

	while (Delay >= Timer.Elapsed());
	Excess.Update();
}

bool GetData(LPCWSTR File)
{
	nlohmann::json Data;
	std::ifstream DataFile(File);

	if (!DataFile)
		return Dialog(skCrypt(L"Opening File Failed"));

	DataFile >> Data;

	for (nlohmann::json & Object : Data["weapons"]) {
		CWeapon Temp;
		Temp.Name = Object["name"];
		Temp.RepeatDelay = Object["repeat-delay"];
		Temp.MagazineCapacity = Object["mag"]["capacity"];
		Temp.StancePenalty = Object["stance-penalty"];
		Temp.FovOffset = Object["fov-offset"];
		Temp.ZoomFactor = Object["zoom-factor"];
		Temp.Automatic = Object["automatic"];
		Temp.Curves = Object["recoil"]["curves"];

		double LastX{ 0.0 }, LastY{ 0.0 };
		for (int Bullet{}; Bullet < Temp.MagazineCapacity; Bullet++)
		{
			double X = Object["recoil"]["values"]["x"][Bullet];
			double Y = Object["recoil"]["values"]["y"][Bullet];
			Temp.Pattern.emplace_back(X - LastX, Y - LastY);
			LastX = X;
			LastY = Y;
		}		

		Weapon.emplace_back(Temp);
	}

	for (nlohmann::json& Object : Data["attachments"]) {
		CAttachment Temp;

		Temp.Name = Object["name"];
		Temp.FovOffset = Object["fov-offset"];
		Temp.ZoomFactor = Object["zoom-factor"];
		Temp.PatternMultiplier = Object["modifiers"]["recoil"]["scalar"];
		Temp.RepeatDelayMultiplier = Object["modifiers"]["repeat-delay"]["scalar"];

		Attachment.emplace_back(Temp);
	}

	return true;
}

namespace WeaponID
{
	constexpr unsigned int MP5{ 0 };
	constexpr unsigned int BoltRifle{ 1 };
	constexpr unsigned int LR300{ 2 };
	constexpr unsigned int M249{ 3 };
	constexpr unsigned int Thompson{ 4 };
	constexpr unsigned int ShotgunWaterpipe{ 5 };
	constexpr unsigned int ShotgunPump{ 6 };
	constexpr unsigned int SemiRifle{ 7 };
	constexpr unsigned int Nailgun{ 8 };
	constexpr unsigned int Python{ 9 };
	constexpr unsigned int Smg{ 10 };
	constexpr unsigned int Spas12{ 11 };
	constexpr unsigned int Revolver{ 12 };
	constexpr unsigned int SemiPistol{ 13 };
	constexpr unsigned int ShotgunDouble{ 14 };
	constexpr unsigned int L96{ 15 };
	constexpr unsigned int AK47{ 16 };
	constexpr unsigned int M39{ 17 };
	constexpr unsigned int M92{ 18 };
	constexpr unsigned int AK47Ice{ 19 };
}

namespace AttachmentID
{
	constexpr unsigned int Silencer{ 0 };
	constexpr unsigned int Scope16x{ 1 };
	constexpr unsigned int Simplesight{ 2 };
	constexpr unsigned int Flashlight{ 3 };
	constexpr unsigned int Muzzleboost{ 4 };
	constexpr unsigned int Scope8x{ 5 };
	constexpr unsigned int Muzzlebrake{ 6 };
	constexpr unsigned int Lasersight{ 7 };
	constexpr unsigned int Holosight{ 8 };
}

CVector2 AnglesToPixels(CVector2 Angles)
{
	return Angles / (-0.03 * Global::Sensitivity * 3.0 * (Global::FOV) * 0.01);
}

double AnimationToMillisecond(CVector2 Angles)
{
	return sqrt(pow(Angles.X, 2.0) + pow(Angles.Y, 2.0)) / 0.02;
}

void AddOverflow(double& Input, double& Overflow)
{
	Overflow = std::modf(Input, &Input) + Overflow;

	if (Overflow > 1.0)
	{
		double Integral{ 0.0 };
		Overflow = std::modf(Overflow, &Integral);
		Input += Integral;
	}
}

bool IsPressed(int Key)
{
	if (GetAsyncKeyState(Key) & 0x8000) 
		return true;

	return false;
}

void ControlRecoil(CWeapon& CurrentWeapon, size_t& Bullet, CTimer& Excess)
{
	double LastX{}, LastY{}, OverflowX{}, OverflowY{}, AnimationOverflow{}, RepeatDelayOverflow{};

	CVector2 Pos = AnglesToPixels(CurrentWeapon.Pattern[Bullet]);

	double Animation = AnimationToMillisecond(CurrentWeapon.Pattern[Bullet]);
	AddOverflow(Animation, AnimationOverflow);

	double RepeatDelay = CurrentWeapon.RepeatDelay;
	AddOverflow(RepeatDelay, RepeatDelayOverflow);

	double UX = Pos.X / Animation;
	double UY = Pos.Y / Animation;

	for (int Index{ 1 }; Index <= static_cast<int>(Animation); Index++)
	{
		double X = Index * UX;
		double Y = Index * UY;

		AddOverflow(X, OverflowX);
		AddOverflow(Y, OverflowY);
		MouseMove(0, static_cast<char>(X - LastX), static_cast<char>(Y - LastY), 0);
		AccurateSleep(1.0, Excess);

		LastX = X; LastY = Y;
	}

	if (RepeatDelay > Animation) AccurateSleep(RepeatDelay - Animation, Excess);
}

int main()
{
	if (!SetConsoleTitle(skCrypt(L"UnKnoWnCheaTs")))
		return Dialog(skCrypt(L"SetConsoleTitle Failed"));

	if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
		return Dialog(skCrypt(L"SetThreadPriority Failed"));

	if (!SetTimerResolution())
		return Dialog(skCrypt(L"NtSetTimerResolution Failed"));

	if (!MouseOpen())
		return Dialog(skCrypt(L"Please install Logitech G HUB"));

	GetData(skCrypt(L"dump.json"));

	CWeapon CurrentWeapon = Weapon[WeaponID::AK47];
	CTimer Excess;
	CTTSpeak Speech;

	Speech.LoadComSpeak(100);
	Speech.ComSpeak(skCrypt(L"Hello UnknownCheats"));

	while (true)
	{
		while (!IsPressed(VK_LBUTTON))
		{
			AccurateSleep(Global::TimerResolution, Excess);
		}

		for (size_t Bullet{ 1 }; Bullet < CurrentWeapon.Pattern.size(); ++Bullet)
		{
			if (!IsPressed(VK_LBUTTON) || !IsPressed(VK_RBUTTON)) break;
			ControlRecoil(CurrentWeapon, Bullet, Excess);
		}

		while (IsPressed(VK_LBUTTON))
		{
			AccurateSleep(Global::TimerResolution, Excess);
		}
	}

	MouseClose();

	return EXIT_SUCCESS;
}