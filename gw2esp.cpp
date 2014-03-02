/*
Compile to a DLL and inject into your running gw2.exe process
*/
#include <locale>
#include <iomanip>
#include <sstream>
#include "gw2lib.h"

using namespace GW2LIB;

// ESP Elements
const bool SelectedHealth = true;
const bool SelectedHealthPercent = true;
const bool DistanceToSelected = false;
const bool SelectedDebug = false;

const bool SelfHealth = false;
const bool SelfHealthPercent = true;
const bool DpsMeter = true;
const bool KillTime = false;

const bool Floaters = true;
const int  FloatersRange = 6000;
const bool EnemyFloaters = false;
const bool AllyFloaters = false;
const bool EnemyPlayerFloaters = true;
const bool AllyPlayerFloaters = false;


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

Font font;
static const DWORD fontColor = 0xffffffff;
int HealthHistory[23] {};
int KillTimeHistory[4] {};

void cbESP()
{
	Character me = GetOwnCharacter();
	Vector3 mypos = me.GetAgent().GetPos();

	Agent agLocked = GetLockedSelection();
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

			//font.Draw(400, 8 + 15 * 6, fontColor, "data: %08X", (void**)agLocked.m_ptr);
		}


		
		if (DpsMeter)
		{
			if (HealthHistory[23] != agLocked.GetAgentId())
				memset(HealthHistory, NULL, sizeof(HealthHistory));

			int seconds;
			seconds = int(GetTickCount() / 250);

			if (HealthHistory[22] < seconds)
			{
				for (int i = 20; i > 0; i--)
					HealthHistory[i] = HealthHistory[i - 1];	

				HealthHistory[0] = int(chrLocked.GetCurrentHealth());
				HealthHistory[22] = seconds;
				HealthHistory[23] = agLocked.GetAgentId();
			}

			int dps;
			std::stringstream Dps;

			// DPS over 1s
			if (HealthHistory[4] != NULL) {
				dps = int((HealthHistory[4] - HealthHistory[1]));
				if (dps < 0) dps = 0;
				Dps << "DPS1s: " << FormatWithCommas(dps);
			}
			else{ Dps << "DPS1s: ..."; }
			font.Draw(1250, 8 + 15 * 0, fontColor, Dps.str());
			Dps.str("");

			// DPS over 5s
			if (HealthHistory[4 * 5] != NULL) {
				dps = int((HealthHistory[4 * 5] - HealthHistory[1]) / 5);
				if (dps < 0) dps = 0;
				Dps << "DPS5s: " << FormatWithCommas(dps);
			}else{ Dps << "DPS5s: ..."; }
			font.Draw(1250, 8 + 15 * 1, fontColor, Dps.str());
			Dps.str("");
		}
	}

	if (KillTime)
	{
		Agent agLocked = GetLockedSelection();
		Character chrLocked = agLocked.GetCharacter();
		
		
		if (chrLocked.m_ptr)
		{
			if (KillTimeHistory[3] && KillTimeHistory[3] != agLocked.GetAgentId())
				memset(KillTimeHistory, 0, sizeof(KillTimeHistory));

			if (chrLocked.GetCurrentHealth() == chrLocked.GetMaxHealth())
				KillTimeHistory[0] = GetTickCount();

			if (chrLocked.GetCurrentHealth() > 0)
				KillTimeHistory[1] = GetTickCount();

			if (KillTimeHistory[0] && KillTimeHistory[1])
				KillTimeHistory[2] = KillTimeHistory[1] - KillTimeHistory[0];

			KillTimeHistory[3] = agLocked.GetAgentId();
		}
		
		font.Draw(1250, 8 + 15 * 2, fontColor, "Timer: %i", KillTimeHistory[0]);
	}

	if (SelfHealth | SelfHealthPercent)
	{
		std::stringstream health;
		if (SelfHealth) health << "Self: " << FormatWithCommas(int(me.GetCurrentHealth())) << " / " << FormatWithCommas(int(me.GetMaxHealth()));
		font.Draw(1200, 8, fontColor, health.str());

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
							std::stringstream cHealth;
							cHealth << FormatWithCommas(int(chr.GetCurrentHealth()));
							font.Draw(x - 30, y - 15, fontColor, cHealth.str());

							DWORD color;
							color = 0x44ff3300;
							DrawCircleProjected(pos, 20.0f, color);
							DrawCircleFilledProjected(pos, 20.0f, color - 0x30000000);

						}

						if (AllyFloaters && (chr.GetAttitude() == GW2::ATTITUDE_FRIENDLY || chr.GetAttitude() == GW2::ATTITUDE_NEUTRAL))
						{
							std::stringstream cHealth;
							cHealth << FormatWithCommas(int(chr.GetCurrentHealth()));
							font.Draw(x - 30, y - 15, fontColor, cHealth.str());

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
							std::stringstream cHealth;
							cHealth << FormatWithCommas(int(chr.GetCurrentHealth()));
							font.Draw(x - 30, y - 15, fontColor, cHealth.str());

							DWORD color;
							color = 0x4433ff00;
							DrawCircleProjected(pos, 20.0f, color);
							DrawCircleFilledProjected(pos, 20.0f, color - 0x30000000);
						}

						if (EnemyPlayerFloaters && chr.GetAttitude() == GW2::ATTITUDE_HOSTILE)
						{
							if (0) // Health
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




void CodeMain(){

	EnableEsp(cbESP);
	
	if (!font.Init(18, "Verdana"))
		DbgOut("could not create font");
}

GW2LIBInit(CodeMain);
