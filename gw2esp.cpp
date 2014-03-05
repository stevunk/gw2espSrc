/*
Compile to a DLL and inject into your running gw2.exe process
*/
#include <locale>
#include <iomanip>
#include <sstream>
#include <Windows.h>
#include <boost/circular_buffer.hpp> // Boost v1.55 Library
#include <numeric>
#include <assert.h>
#include "gw2lib.h"

using namespace GW2LIB;

// ESP Elements
bool Help = false;

bool DpsMeter = true;
bool DpsDebug = false;
bool AllowNegativeDps = false;
bool DpsLock = false;

int  FloatersRange = 6000;
bool Floaters = false;
bool FloatersType = true;
bool EnemyFloaters = true;
bool AllyFloaters = true;
bool EnemyPlayerFloaters = true;
bool AllyPlayerFloaters = true;

bool SelectedHealth = true;
bool SelectedHealthPercent = true;
bool DistanceToSelected = false;
bool SelectedDebug = false;

bool SelfHealth = false;
bool SelfHealthPercent = true;

bool KillTime = false;

// Global Vars
Font font;
Font fontHelp;
static const DWORD fontColor = 0xffffffff;
boost::circular_buffer<int> dpsBuffer(50);
int dpsThis = NULL;

// Global Functions
float dist(Vector3 p1, Vector3 p2)
{
	return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2) + pow(p1.z - p2.z, 2));
}
template<class T>
std::string FormatWithCommas(T value)
{
	std::stringstream ss;
	ss.imbue(std::locale(""));
	ss << std::fixed << value;
	return ss.str();
}

void cbESP()
{
	Character me = GetOwnCharacter();
	Vector3 mypos = me.GetAgent().GetPos();
	Agent agLocked = GetLockedSelection();

	if (Help)
	{
		fontHelp.Draw(760, 150 + 15 * 1, fontColor, "[%i] [Alt /] Toggle this Help screen", SelectedHealth);

		fontHelp.Draw(760, 150 + 15 * 3, fontColor, "[%i] [Alt D] DPS Meter", DpsMeter);
		fontHelp.Draw(760, 150 + 15 * 4, fontColor, "[%i] [Alt B] DPS Meter Debug", DpsDebug);
		fontHelp.Draw(760, 150 + 15 * 5, fontColor, "[%i] [Alt N] DPS Meter AllowNegativeDPS", AllowNegativeDps);
		fontHelp.Draw(760, 150 + 15 * 6, fontColor, "[%i] [Alt L] DPS Meter LockOnCurrentlySelected", DpsLock);

		fontHelp.Draw(760, 150 + 15 * 8, fontColor, "[%i] [Alt F] Floaters", Floaters);
		fontHelp.Draw(760, 150 + 15 * 9, fontColor, "[%i] [Alt 0] Floaters Type (Distance or Health)", FloatersType);
		fontHelp.Draw(760, 150 + 15 * 10, fontColor, "[%i] [Alt 1] Floaters on Ally NPC", AllyFloaters);
		fontHelp.Draw(760, 150 + 15 * 11, fontColor, "[%i] [Alt 2] Floaters on Ally Players", AllyPlayerFloaters);
		fontHelp.Draw(760, 150 + 15 * 12, fontColor, "[%i] [Alt 3] Floaters on Enemy NPC", EnemyFloaters);
		fontHelp.Draw(760, 150 + 15 * 13, fontColor, "[%i] [Alt 4] Floaters on Enemy Players", EnemyPlayerFloaters);

		fontHelp.Draw(760, 150 + 15 * 15, fontColor, "[%i] [Alt S] Selected Health/Percent", SelectedHealth);
		fontHelp.Draw(760, 150 + 15 * 16, fontColor, "[%i] [Alt R] Selected Range", DistanceToSelected);
		fontHelp.Draw(760, 150 + 15 * 17, fontColor, "[%i] [Alt I] Selected Debug", SelectedDebug);

		fontHelp.Draw(760, 150 + 15 * 19, fontColor, "[%i] [Alt H] Self Health", SelfHealth);
		fontHelp.Draw(760, 150 + 15 * 20, fontColor, "[%i] [Alt P] Self Health Percent", SelfHealthPercent);

		fontHelp.Draw(760, 150 + 15 * 22, fontColor, "[%i] [Alt T] Kill Timer", KillTime);
	}

	if (DpsMeter)
	{
		if (!DpsLock)
		{
			if (agLocked.IsValid())
			{
				dpsThis = agLocked.GetAgentId();
			}
			else
			{
				dpsThis = NULL;
			}
		}
		
			
					
		int dp1s = 0;
		std::stringstream dp1S;
		dp1S << "DPS 1s: ";
		if (!dpsBuffer.empty())
		{
			for (int i = 0; i < 10; i++)
				dp1s += dpsBuffer[i];
			dp1S << FormatWithCommas(dp1s);
		}
		else
			dp1S << "...";

		int dp5s = 0;
		std::stringstream dp5S;
		dp5S << "DPS 5s: ";
		if (!dpsBuffer.empty())
		{
			for (int i = 0; i < 50; i++)
				dp5s += dpsBuffer[i];
			dp5s = int(dp5s / 5);
			dp5S << FormatWithCommas(dp5s);
		}
		else
			dp5S << "...";
		
		font.Draw(1250, 8 + 15 * 0, fontColor, dp1S.str());
		font.Draw(1250, 8 + 15 * 1, fontColor, dp5S.str());

		if (DpsDebug)
		{
			for (int i = 0; i < 50; i++)
				font.Draw(1250, 8 + 15 * (float(i) + 3), fontColor, "DMG/100ms: %i", dpsBuffer[i]);
		}
		
	}

	if (KillTime)
	{
		Agent agLocked = GetLockedSelection();
		Character chrLocked = agLocked.GetCharacter();

		//font.Draw(1250, 8 + 15 * 2, fontColor, "Timer: %i", KillTimeHistory[0]);
	}

	if (agLocked.m_ptr)
	{
		Character chrLocked = agLocked.GetCharacter();

		if (SelectedHealth | SelectedHealthPercent)
		{
			std::stringstream health;

			if (SelectedHealth)
				health << "Target: " << FormatWithCommas(int(chrLocked.GetCurrentHealth())) << " / " << FormatWithCommas(int(chrLocked.GetMaxHealth()));
			if (SelectedHealthPercent && int(chrLocked.GetMaxHealth()) > 0)
				health << " [" << int(chrLocked.GetCurrentHealth() / chrLocked.GetMaxHealth() * 100) << "%%]";

			font.Draw(400, 8, fontColor, health.str());
		}

		if (DistanceToSelected)
		{
			Vector3 pos = agLocked.GetPos();
			std::stringstream distance;
			distance << "Distance: " << FormatWithCommas(int(dist(mypos, pos)));
			font.Draw(400, 8 + 15 * 1, fontColor, distance.str());
		}

		if (SelectedDebug)
		{
			font.Draw(400, 8 + 15 * 2, fontColor, "agPtr: %p", *(void**)agLocked.m_ptr);
			if (chrLocked.m_ptr)
				font.Draw(400, 8 + 15 * 3, fontColor, "chPtr: %p", *(void**)chrLocked.m_ptr);
			
			font.Draw(400, 8 + 15 * 4, fontColor, "agentId: %i / 0x%04X", agLocked.GetAgentId(), agLocked.GetAgentId());
			
			font.Draw(400, 8 + 15 * 5, fontColor, "cat: %i / type: %i", agLocked.GetCategory(), agLocked.GetType());

			//font.Draw(400, 8 + 15 * 6, fontColor, "dpsBuffer: %i", dpsBuffer);
		}
	}

	if (SelfHealth | SelfHealthPercent)
	{
		std::stringstream health;
		if (SelfHealth) health << "Self: " << FormatWithCommas(int(me.GetCurrentHealth())) << " / " << FormatWithCommas(int(me.GetMaxHealth()));
		font.Draw(775, 8, fontColor, health.str());

		if (SelfHealthPercent) font.Draw(943, 973, fontColor, "%i", int(me.GetCurrentHealth() / me.GetMaxHealth() * 100));
	}

	if (Floaters)
	{
		Agent ag;
		while (ag.BeNext())
		{
			Character chr = ag.GetCharacter();
			Vector3 pos = ag.GetPos();
			
			if (chr.IsControlled())
				continue;

			if (dist(mypos, pos) >= FloatersRange)
				continue;

			float x, y;
			if (WorldToScreen(pos, &x, &y))
			{
				if (ag.GetCategory() == GW2::AGENT_CATEGORY_CHAR)
				{
					if (!chr.IsPlayer() && int(chr.GetCurrentHealth()) > 0 && int(chr.GetMaxHealth()) > 1)
					{
						if (EnemyFloaters && (chr.GetAttitude() == GW2::ATTITUDE_HOSTILE || chr.GetAttitude() == GW2::ATTITUDE_INDIFFERENT))
						{
							if (FloatersType) // Health
							{
								std::stringstream cHealth;
								cHealth << FormatWithCommas(int(chr.GetCurrentHealth()));
								font.Draw(x - 30, y - 15, fontColor, cHealth.str());
							}
							else // Distance
							{
								Vector3 pos = ag.GetPos();
								std::stringstream distance;
								distance << FormatWithCommas(int(dist(mypos, pos)));
								font.Draw(x - 30, y - 15, fontColor, distance.str());
							}

							DWORD color;
							color = 0x44ff3300;
							DrawCircleProjected(pos, 20.0f, color);
							DrawCircleFilledProjected(pos, 20.0f, color - 0x30000000);

						}

						if (AllyFloaters && (chr.GetAttitude() == GW2::ATTITUDE_FRIENDLY || chr.GetAttitude() == GW2::ATTITUDE_NEUTRAL))
						{
							if (FloatersType) // Health
							{
								std::stringstream cHealth;
								cHealth << FormatWithCommas(int(chr.GetCurrentHealth()));
								font.Draw(x - 30, y - 15, fontColor, cHealth.str());
							}
							else // Distance
							{
								Vector3 pos = ag.GetPos();
								std::stringstream distance;
								distance << FormatWithCommas(int(dist(mypos, pos)));
								font.Draw(x - 30, y - 15, fontColor, distance.str());
							}

							DWORD color;
							color = 0x4433ff00;
							DrawCircleProjected(pos, 20.0f, color);
							DrawCircleFilledProjected(pos, 20.0f, color - 0x30000000);
						}
					}
					
					if (chr.IsPlayer() && int(chr.GetCurrentHealth()) > 0)
					{
						if (AllyPlayerFloaters && chr.GetAttitude() == GW2::ATTITUDE_FRIENDLY)
						{
							if (FloatersType) // Health
							{
								std::stringstream cHealth;
								cHealth << FormatWithCommas(int(chr.GetCurrentHealth()));
								font.Draw(x - 30, y - 15, fontColor, cHealth.str());
							}
							else // Distance
							{
								Vector3 pos = ag.GetPos();
								std::stringstream distance;
								distance << FormatWithCommas(int(dist(mypos, pos)));
								font.Draw(x - 30, y - 15, fontColor, distance.str());
							}

							DWORD color;
							color = 0x4433ff00;
							DrawCircleProjected(pos, 20.0f, color);
							DrawCircleFilledProjected(pos, 20.0f, color - 0x30000000);
						}

						if (EnemyPlayerFloaters && chr.GetAttitude() == GW2::ATTITUDE_HOSTILE)
						{
							if (FloatersType) // Health
							{
								std::stringstream cHealth;
								cHealth << FormatWithCommas(int(chr.GetCurrentHealth()));
								font.Draw(x - 30, y - 15, fontColor, cHealth.str());
							}
							else // Distance
							{
								Vector3 pos = ag.GetPos();
								std::stringstream distance;
								distance << FormatWithCommas(int(dist(mypos, pos)));
								font.Draw(x - 30, y - 15, fontColor, distance.str());
							}
							
							DWORD color;
							color = 0x44ff3300;
							DrawCircleProjected(pos, 20.0f, color);
							DrawCircleFilledProjected(pos, 20.0f, color - 0x30000000);
						}
					}
				}
			}
		}
	}
}

void DpsBuffer()
{
	int dpsCurrent = NULL;
	float previousHealth = NULL;

	while (true){
		if (dpsThis)
		{
			// new Agent, wipe buffer
			if (dpsCurrent != dpsThis){
				previousHealth = NULL;
				for (int i = 0; i < 50; i++)
					dpsBuffer.push_front(0);
				dpsBuffer.clear();
			}

			Agent ag;
			while (ag.BeNext())
			{
				Character ch = ag.GetCharacter();
				if (!ch.IsValid())
					continue;

				if (dpsThis == 0 || ag.GetAgentId() != dpsThis)
					continue;

				if (!previousHealth)
					previousHealth = ch.GetCurrentHealth();
				else
				{
					if (AllowNegativeDps || previousHealth > ch.GetCurrentHealth())
						dpsBuffer.push_front(int(previousHealth - ch.GetCurrentHealth()));
					else
						dpsBuffer.push_front(0);

					previousHealth = ch.GetCurrentHealth();
				}

				break;
			}
			dpsCurrent = dpsThis;
		}
		else
		{
			previousHealth = NULL;
			for (int i = 0; i < 50; i++)
				dpsBuffer.push_front(0);
			dpsBuffer.clear();
		}

		Sleep(100);
	}
}

void HotKey()
{
	// Help
	RegisterHotKey(NULL, 0, MOD_ALT | MOD_NOREPEAT, VK_OEM_2); // Help

	// DPS Meter
	RegisterHotKey(NULL, 10, MOD_ALT | MOD_NOREPEAT, 0x44); // DPS Meter
	RegisterHotKey(NULL, 11, MOD_ALT | MOD_NOREPEAT, 0x42); // DPS Meter Debug
	RegisterHotKey(NULL, 12, MOD_ALT | MOD_NOREPEAT, 0x4E); // DPS Meter Allow Negative DPS
	RegisterHotKey(NULL, 13, MOD_ALT | MOD_NOREPEAT, 0x4C); // DPS Meter LockToCurrentlySelected

	// Floaters
	RegisterHotKey(NULL, 20, MOD_ALT | MOD_NOREPEAT, 0x46); // Floaters
	RegisterHotKey(NULL, 21, MOD_ALT | MOD_NOREPEAT, 0x30); // Floaters Type (Health/Distance)
	RegisterHotKey(NULL, 22, MOD_ALT | MOD_NOREPEAT, 0x31); // Floaters Ally NPC
	RegisterHotKey(NULL, 23, MOD_ALT | MOD_NOREPEAT, 0x32); // Floaters Ally Players
	RegisterHotKey(NULL, 24, MOD_ALT | MOD_NOREPEAT, 0x33); // Floaters Enemy NPC
	RegisterHotKey(NULL, 25, MOD_ALT | MOD_NOREPEAT, 0x34); // Flaoters Enemy Players

	// Selected
	RegisterHotKey(NULL, 30, MOD_ALT | MOD_NOREPEAT, 0x53); // Selected Health/Percent
	RegisterHotKey(NULL, 31, MOD_ALT | MOD_NOREPEAT, 0x52); // Selected Range
	RegisterHotKey(NULL, 32, MOD_ALT | MOD_NOREPEAT, 0x49); // Selected Debug
	
	// Self
	RegisterHotKey(NULL, 40, MOD_ALT | MOD_NOREPEAT, 0x48); // Self Health
	RegisterHotKey(NULL, 41, MOD_ALT | MOD_NOREPEAT, 0x50); // Self Health Percent

	// Kill Timer
	RegisterHotKey(NULL, 50, MOD_ALT | MOD_NOREPEAT, 0x4B); // Kill Timer

	MSG msg;
	while (GetMessage(&msg, 0, 0, 0))
	{
		PeekMessage(&msg, 0, 0, 0, 0x0001);
		switch (msg.message)
		{
		case WM_HOTKEY:
			// Help
			if (msg.wParam == 0) Help = !Help;

			// DPS Meter
			if (msg.wParam == 10) DpsMeter = !DpsMeter;
			if (msg.wParam == 11) DpsDebug = !DpsDebug;
			if (msg.wParam == 12) AllowNegativeDps = !AllowNegativeDps;
			if (msg.wParam == 13) DpsLock = !DpsLock;

			// Floaters
			if (msg.wParam == 20) Floaters = !Floaters;
			if (msg.wParam == 21) FloatersType = !FloatersType;
			if (msg.wParam == 22) AllyFloaters = !AllyFloaters;
			if (msg.wParam == 23) AllyPlayerFloaters = !AllyPlayerFloaters;
			if (msg.wParam == 24) EnemyFloaters = !EnemyFloaters;
			if (msg.wParam == 25) EnemyPlayerFloaters = !EnemyPlayerFloaters;

			// Selected
			if (msg.wParam == 30) { SelectedHealth = !SelectedHealth; SelectedHealthPercent = !SelectedHealthPercent; }
			if (msg.wParam == 31) DistanceToSelected = !DistanceToSelected;
			if (msg.wParam == 32) SelectedDebug = !SelectedDebug;

			// Self
			if (msg.wParam == 40) SelfHealth = !SelfHealth;
			if (msg.wParam == 41) SelfHealthPercent = !SelfHealthPercent;

			// Kill Timer
			if (msg.wParam == 50) KillTime = !KillTime;

			
		}
	}
}
void CodeMain(){
	NewThread(DpsBuffer);
	NewThread(HotKey);
	EnableEsp(cbESP);
	
	if (!font.Init(18, "Verdana"))
		DbgOut("could not init Verdana font");

	if (!fontHelp.Init(18, "Courier New"))
		DbgOut("could not init Courier New font");
}

GW2LIBInit(CodeMain);
