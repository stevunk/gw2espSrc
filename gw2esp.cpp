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
const bool SelfHealth = true;
const bool SelfHealthPercent = true;

const bool DistanceToSelected = true;
const bool SelectedDebug = false;

const bool Floaters = true;
const int  FloatersRange = 3000;
const bool EnemyFloaters = true;
const bool AllyFloaters = true;
const bool EnemyPlayerFloaters = true;
const bool AllyPlayerFloaters = true;


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

void cbESP()
{
	// Me and my position
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
				health << "Selected: " << FormatWithCommas(int(chrLocked.GetCurrentHealth())) << " / " << FormatWithCommas(int(chrLocked.GetMaxHealth()));
			if (SelectedHealthPercent && int(chrLocked.GetMaxHealth()) > 0)
				health << " [" << int(chrLocked.GetCurrentHealth() / chrLocked.GetMaxHealth() * 100) << "%%]";

			font.Draw(400, 8, fontColor, health.str());
		}

		if (DistanceToSelected)
		{
			Vector3 pos = agLocked.GetPos();
			std::stringstream distance;
			distance << "Distance: " << FormatWithCommas(int(dist(mypos, pos)));
			font.Draw(400, 23, fontColor, distance.str());
		}

		if (SelectedDebug)
		{
			font.Draw(400, 38, fontColor, "agPtr: %p", *(void**) agLocked.m_ptr);
			if (chrLocked.m_ptr)
				font.Draw(400, 53, fontColor, "chPtr: %p", *(void**)chrLocked.m_ptr);
			
			font.Draw(400, 68, fontColor, "agentId: %i / 0x%04X", agLocked.GetAgentId(), agLocked.GetAgentId());
			
			unsigned long agmetrics = *(unsigned long*)(*(unsigned long*)agLocked.m_ptr + 0x1c);
			unsigned long long tok = *(unsigned long long*)(agmetrics + 0x98);
			unsigned long long seq = *(unsigned long long*)(agmetrics + 0xa0);
			
			unsigned long long* tokP = &*(unsigned long long*)(*(unsigned long*)(*(unsigned long*)agLocked.m_ptr + 0x1c) + 0x98);
			unsigned long long* seqP = &*(unsigned long long*)(*(unsigned long*)(*(unsigned long*)agLocked.m_ptr + 0x1c) + 0xa0);
			
			std::stringstream tokaddr;
			std::stringstream seqaddr;
			std::stringstream math;
			
			tokaddr << "addr: " << tokP << " token: " << tok;
			seqaddr << "addr: " << seqP << " seqen: " << seq;
			
			font.Draw(400, 83, fontColor, tokaddr.str());
			font.Draw(400, 98, fontColor, seqaddr.str());
		}
	}

	if (SelfHealth | SelfHealthPercent)
	{
		std::stringstream health;
		health << "Self:";
		if (SelfHealth) health << " " << FormatWithCommas(int(me.GetCurrentHealth())) << " / " << FormatWithCommas(int(me.GetMaxHealth()));
		if (SelfHealthPercent) health << " [" << int(me.GetCurrentHealth() / me.GetMaxHealth() * 100) << "%%]";
		font.Draw(1200, 8, fontColor, health.str());
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
							std::stringstream cHealth;
							cHealth << FormatWithCommas(int(chr.GetCurrentHealth()));
							font.Draw(x - 30, y - 15, fontColor, cHealth.str());

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

void CodeMain()
{
	EnableEsp(cbESP);
	if (!font.Init(18, "Verdana"))
		DbgOut("could not create font");
}

GW2LIBInit(CodeMain)
