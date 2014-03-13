/*
Compile to a DLL and inject into your running gw2.exe process
*/

#include <locale>
#include <iomanip>
#include <sstream>
#include <numeric>
#include <assert.h>
#include <Windows.h>
#include <boost/circular_buffer.hpp> // Boost v1.55 library
#include "gw2lib.h" // GW2Lib library by Rafi

using namespace GW2LIB;

// ESP Elements
bool Help = false;

bool DpsMeter = true;
bool DpsDebug = false;
bool AllowNegativeDps = false;
bool DpsLock = false;

int  FloatersRange = 3000;
bool Floaters = false;
bool FloaterStats = false;
bool FloaterClasses = false;
bool FloatersType = true;
bool EnemyFloaters = true;
bool AllyFloaters = true;
bool EnemyPlayerFloaters = true;
bool AllyPlayerFloaters = true;

bool SelectedHealth = true;
bool SelectedHealthPercent = true;
bool DistanceToSelected = false;
bool SelectedDebug = false;

bool SelfHealthPercent = true;

bool KillTime = false;
bool MeasureDistance = false;
bool AllyPlayers = false;
bool AllyPlayersRange = false;

// Global Vars
HWND hwnd = FindWindowEx(NULL, NULL, L"Guild Wars 2", NULL);
Font font;
double fontW = 10;
static const DWORD fontColor = 0xffffffff;
static const DWORD backColor = 0xff000000;
boost::circular_buffer<int> dpsBuffer(50);
int dpsThis = NULL;
Vector3 MeasureDistanceStart = { 0, 0, 0 };

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
SIZE ssSize(std::string ss, int height)
{
	HDC hdc = GetDC(hwnd);
	HFONT hFont = CreateFont(height, 0, 0, 0, 600, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_RASTER_PRECIS, CLIP_TT_ALWAYS, NONANTIALIASED_QUALITY,
		DEFAULT_PITCH, L"Verdana");
	HFONT hFontOld = (HFONT)SelectObject(hdc, hFont);
	try
	{
		ss.replace(ss.find("%"), 1, "");
	}
	catch (const std::out_of_range& oor) {
		// pass
	}

	SIZE size;
	GetTextExtentPoint32A(hdc, ss.c_str(), ss.length(), &size);

	if (0) // draw debug
	{
		RECT rcFont = { 500 + 0, 50 + 0, 500 + size.cx, 50 + size.cy };
		DrawTextA(hdc, ss.c_str(), ss.length(), &rcFont, DT_LEFT);
	}
	
	DeleteObject(hFont);
	ReleaseDC(hwnd, hdc);

	size_t count;
	count = std::count(ss.begin(), ss.end(), ':'); size.cx -= count * 2;
	count = std::count(ss.begin(), ss.end(), ' '); size.cx -= count * 2;
	count = std::count(ss.begin(), ss.end(), '['); size.cx -= count * 1;
	count = std::count(ss.begin(), ss.end(), ']'); size.cx -= count * 1;

	return size;
}

void cbESP()
{
	Character me = GetOwnCharacter();
	Vector3 mypos = me.GetAgent().GetPos();
	Agent agLocked = GetLockedSelection();

	if (Help)
	{
		SIZE size = ssSize("[%i] [Alt L] DPS Meter LockOnCurrentlySelected", 16);
		float x = int(GetWindowWidth() / 2 - 45 * fontW / 2);
		float y = 150;
		int xPad = 5; int yPad = 2;

		DrawRectFilled(x - xPad, y + 15 - yPad, size.cx + xPad * 2, size.cy*27 + yPad * 4, backColor - 0x44000000);
			  DrawRect(x - xPad, y + 15 - yPad, size.cx + xPad * 2, size.cy*27 + yPad * 4, 0xff444444);

		font.Draw(x, y + 15 * 1, fontColor, "[%i] [Alt /] Toggle this Help screen", SelectedHealth);

		font.Draw(x, y + 15 * 3, fontColor,  "[%i] [Alt D] DPS Meter", DpsMeter);
		font.Draw(x, y + 15 * 4, fontColor,  "[%i] [Alt B] DPS Meter Debug", DpsDebug);
		font.Draw(x, y + 15 * 5, fontColor,  "[%i] [Alt N] DPS Meter AllowNegativeDPS", AllowNegativeDps);
		font.Draw(x, y + 15 * 6, fontColor,  "[%i] [Alt L] DPS Meter LockOnCurrentlySelected", DpsLock);

		font.Draw(x, y + 15 * 8, fontColor,  "[%i] [Alt F] Floaters", Floaters);
		font.Draw(x, y + 15 * 9, fontColor,  "[-] [Alt +] Floaters Range + (%i)", FloatersRange);
		font.Draw(x, y + 15 * 10, fontColor, "[-] [Alt -] Floaters Range - (%i)", FloatersRange);
		font.Draw(x, y + 15 * 11, fontColor, "[%i] [Alt 9] Floater Count", FloaterStats);
		font.Draw(x, y + 15 * 12, fontColor, "[%i] [Alt 8] Floater Classes (ally players)", FloaterClasses);
		font.Draw(x, y + 15 * 13, fontColor, "[%i] [Alt 0] Floaters Type (Distance or Health)", FloatersType);

		font.Draw(x, y + 15 * 15, fontColor, "[%i] [Alt 1] Floaters on Ally NPC", AllyFloaters);
		font.Draw(x, y + 15 * 16, fontColor, "[%i] [Alt 2] Floaters on Enemy NPC", EnemyFloaters);
		font.Draw(x, y + 15 * 17, fontColor, "[%i] [Alt 3] Floaters on Ally Players", AllyPlayerFloaters);
		font.Draw(x, y + 15 * 18, fontColor, "[%i] [Alt 4] Floaters on Enemy Players", EnemyPlayerFloaters);

		font.Draw(x, y + 15 * 20, fontColor, "[%i] [Alt S] Selected Health/Percent", SelectedHealth);
		font.Draw(x, y + 15 * 21, fontColor, "[%i] [Alt R] Selected Range", DistanceToSelected);
		font.Draw(x, y + 15 * 22, fontColor, "[%i] [Alt I] Selected Debug", SelectedDebug);

		font.Draw(x, y + 15 * 24, fontColor, "[%i] [Alt P] Self Health Percent", SelfHealthPercent);

		font.Draw(x, y + 15 * 26, fontColor, "[%i] [Alt T] Kill Timer", KillTime);
		font.Draw(x, y + 15 * 27, fontColor, "[%i] [Alt M] Measure Distance", MeasureDistance);
		font.Draw(x, y + 15 * 28, fontColor, "[%i] [Alt C] Ally Player Info", AllyPlayers);
		font.Draw(x, y + 15 * 29, fontColor, "[%i] [Alt V] Ally Player Info (show distance)", AllyPlayersRange);
	}

	if (DpsMeter)
	{
		if (!DpsLock)
		{
			if (agLocked.IsValid())
				dpsThis = agLocked.GetAgentId();
			else
				dpsThis = NULL;
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
		{
			dp1S << "...";
		}

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
		{
			dp5S << "...";
		}
			
		SIZE sizeA = ssSize(dp1S.str(), 16);
		SIZE sizeB = ssSize(dp5S.str(), 16);
		SIZE size;
		if (sizeA.cx >= sizeB.cx)
			size = sizeA;			
		else
			size = sizeB;
			
		int xPad = 5; int yPad = 2;
		int x = int(GetWindowWidth() / 4*3); int y = 15 + size.cy + yPad*2;
		x -= 120 / 2; y -= (size.cy*2+yPad*2) / 2;

		DrawRectFilled(x - xPad, y - yPad - size.cy/2 - yPad/2, size.cx + xPad * 2, size.cy*2 + yPad * 3, backColor - 0x44000000);
		DrawRect(x - xPad, y - yPad - size.cy / 2 - yPad / 2, size.cx + xPad * 2, size.cy * 2 + yPad * 3, 0xff444444);
		font.Draw(x, y - size.cy/2 - yPad/2, fontColor, dp1S.str());
		font.Draw(x, y + size.cy/2 + yPad/2, fontColor, dp5S.str());

		if (DpsDebug)
		{
			y += 45; size.cx = 150;
			DrawRectFilled(x - xPad, y - yPad - size.cy / 2 - yPad / 2, size.cx + xPad * 2, size.cy * 47 + yPad * 2, backColor - 0x44000000);
			DrawRect(x - xPad, y - yPad - size.cy / 2 - yPad / 2, size.cx + xPad * 2, size.cy * 47 + yPad * 2, 0xff444444);

			for (int i = 0; i < 50; i++)
			{
				std::stringstream dps;
				dps << "DMG/100ms: " << FormatWithCommas(dpsBuffer[i]);
				font.Draw(x, 8 + 15 * (float(i) + 3), fontColor, dps.str());
			}
		}
	}

	if (KillTime)
	{
		// too lazy to code atm
	}

	if (agLocked.m_ptr)
	{
		Character chrLocked = agLocked.GetCharacter();
		
		int leftAlignX;

		if (SelectedHealth | SelectedHealthPercent)
		{
			std::stringstream ss;

			if (SelectedHealth)
				ss << "Selected: " << FormatWithCommas(int(chrLocked.GetCurrentHealth())) << " / " << FormatWithCommas(int(chrLocked.GetMaxHealth()));
			if (SelectedHealthPercent && int(chrLocked.GetMaxHealth()) > 0)
				ss << " [" << int(chrLocked.GetCurrentHealth() / chrLocked.GetMaxHealth() * 100) << "%%]";

			SIZE size = ssSize(ss.str(), 16);
			int xPad = 5; int yPad = 2;
			int x = int(GetWindowWidth() / 4); int y = 15;
			x -= size.cx / 2; y -= size.cy / 2;
			
			leftAlignX = x;
			DrawRectFilled(x - xPad, y - yPad, size.cx + xPad * 2, size.cy + yPad * 2, backColor - 0x44000000);
			DrawRect(x - xPad, y - yPad, size.cx + xPad * 2, size.cy + yPad * 2, 0xff444444);
			font.Draw(x, y, fontColor, ss.str());
		}

		if (DistanceToSelected)
		{
			Vector3 pos = agLocked.GetPos();
			std::stringstream ss;
			ss << "Distance: " << FormatWithCommas(int(dist(mypos, pos)));

			SIZE size = ssSize(ss.str(), 16);
			int xPad = 5; int yPad = 2;
			int x = int(GetWindowWidth() / 4); int y = 15 + yPad * 2 + size.cy;
			x -= size.cx / 2; y -= size.cy / 2;
			
			if (leftAlignX)
			{
				x = leftAlignX;
			}
			else{
				y -= size.cy + yPad*2;
			}

			DrawRectFilled(x - xPad, y - yPad, size.cx + xPad * 2, size.cy + yPad * 2, backColor - 0x44000000);
			DrawRect(x - xPad, y - yPad, size.cx + xPad * 2, size.cy + yPad * 2, 0xff444444);

			font.Draw(x, y, fontColor, ss.str());
		}

		if (SelectedDebug)
		{
			int x = int(GetWindowWidth() / 4); int y = 15;	// center

			font.Draw(x, 8 + 15 * 2, fontColor, "agPtr: %p", *(void**)agLocked.m_ptr);
			if (chrLocked.m_ptr)
				font.Draw(x, 8 + 15 * 3, fontColor, "chPtr: %p", *(void**)chrLocked.m_ptr);
			
			font.Draw(x, 8 + 15 * 4, fontColor, "agentId: %i / 0x%04X", agLocked.GetAgentId(), agLocked.GetAgentId());
			
			font.Draw(x, 8 + 15 * 5, fontColor, "cat: %i / type: %i", agLocked.GetCategory(), agLocked.GetType());

			//font.Draw(x, 8 + 15 * 6, fontColor, "dpsBuffer: %i", dpsBuffer);
		}
	}

	if (SelfHealthPercent)
	{
		std::stringstream ss;
		ss << int(me.GetCurrentHealth() / me.GetMaxHealth() * 100);
		
		SIZE size = ssSize(ss.str(), 16);
		int x = int(GetWindowWidth() / 2); int y = int(GetWindowHeight() - 92);
		x -= size.cx / 2; y -= size.cy / 2;

		font.Draw(x, y, fontColor, ss.str());
	}

	if (MeasureDistance)
	{
		if (MeasureDistanceStart.x == 0 && MeasureDistanceStart.y == 0 && MeasureDistanceStart.z == 0)
			MeasureDistanceStart = mypos;

		std::stringstream ss;
		ss << "Displacement: " << FormatWithCommas(int(dist(mypos, MeasureDistanceStart)));
		
		int x = int(GetWindowWidth() / 2); int y = 15;
		SIZE size = ssSize(ss.str(), 16);
		x -= size.cx/2; y -= size.cy/2;
		int xPad = 5; int yPad = 2;

		DrawRectFilled(x - xPad, y - yPad, size.cx + xPad*2, size.cy + yPad*2, backColor - 0x44000000);
		DrawRect(x - xPad, y - yPad, size.cx + xPad*2, size.cy + yPad*2, 0xff444444);
		font.Draw(x, y, fontColor, ss.str());
	}
	else
	{
		MeasureDistanceStart = { 0, 0, 0 };
	}
	
	if (AllyPlayers)
	{
		float x = 320;
		float y = 20;
		int boxWidth = 400;
		int i = 4;
		Agent ag;
		Agent agCount;

		int count = 0;
		while (agCount.BeNext())
		{
			Character chr = agCount.GetCharacter();
			if (chr.IsControlled())
				continue;

			if (!chr.IsPlayer() || chr.GetAttitude() != GW2::ATTITUDE_FRIENDLY)
				continue;

			if (!chr.IsValid())
				continue;

			count++;
		}

		int xPad = 5; int yPad = 2;
		DrawRectFilled(x - xPad, y - yPad + 15 * 2, boxWidth + xPad * 2, count * 15 + 15 * 2 + yPad * 3, backColor - 0x44000000);
		      DrawRect(x - xPad, y - yPad + 15 * 2, boxWidth + xPad * 2, count * 15 + 15 * 2 + yPad * 3, 0xff444444);

		font.Draw(x, y + 15 * 2, fontColor, "Allies close by");
		font.Draw(x, y + 15 * 3, fontColor, "---------------");
		
		while (ag.BeNext())
		{
			Character chr = ag.GetCharacter();
			if (chr.IsControlled())
				continue;

			if (!chr.IsPlayer() || chr.GetAttitude() != GW2::ATTITUDE_FRIENDLY)
				continue;

			if (!chr.IsValid())
				continue;

			std::stringstream out;
			switch (chr.GetProfession())
			{
			case GW2::PROFESSION_GUARDIAN:
				out << "[Guard] ";
				break;
			case GW2::PROFESSION_WARRIOR:
				out << "[War] ";
				break;
			case GW2::PROFESSION_ENGINEER:
				out << "[Engi] ";
				break;
			case GW2::PROFESSION_RANGER:
				out << "[Rang] ";
				break;
			case GW2::PROFESSION_THIEF:
				out << "[Thief] ";
				break;
			case GW2::PROFESSION_ELEMENTALIST:
				out << "[Ele] ";
				break;
			case GW2::PROFESSION_MESMER:
				out << "[Mes] ";
				break;
			case GW2::PROFESSION_NECROMANCER:
				out << "[Necro] ";
				break;
			}
			out << " " << chr.GetName() << " [" << FormatWithCommas(int(chr.GetMaxHealth())) << " hp]";
			
			if (AllyPlayersRange)
			{
				Vector3 pos = ag.GetPos();
				out << " " << FormatWithCommas(int(dist(mypos, pos))) << " away";
			}

			font.Draw(x, y + 15 * i, fontColor, out.str());
			i++;
		}
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
						// Enemy NPC
						if (EnemyFloaters && (chr.GetAttitude() == GW2::ATTITUDE_HOSTILE || chr.GetAttitude() == GW2::ATTITUDE_INDIFFERENT))
						{
							if (FloatersType) // Health
							{
								std::stringstream health;
								health << FormatWithCommas(int(chr.GetMaxHealth()));
								int xOffset = int(health.str().size()*fontW / 2);
								font.Draw(x - xOffset, y - 15, fontColor, health.str());
							}
							else // Distance
							{
								Vector3 pos = ag.GetPos();
								std::stringstream distance;
								distance << FormatWithCommas(int(dist(mypos, pos)));
								int xOffset = int(distance.str().size()*fontW / 2);
								font.Draw(x - xOffset, y - 15, fontColor, distance.str());
							}

							DWORD color;
							color = 0x44ff3300;
							DrawCircleProjected(pos, 20.0f, color);
							DrawCircleFilledProjected(pos, 20.0f, color - 0x30000000);

						}

						// Ally NPC
						if (AllyFloaters && (chr.GetAttitude() == GW2::ATTITUDE_FRIENDLY || chr.GetAttitude() == GW2::ATTITUDE_NEUTRAL))
						{
							if (FloatersType) // Health
							{
								std::stringstream health;
								health << FormatWithCommas(int(chr.GetMaxHealth()));
								int xOffset = int(health.str().size()*fontW / 2);
								font.Draw(x - xOffset, y - 15, fontColor, health.str());
							}
							else // Distance
							{
								Vector3 pos = ag.GetPos();
								std::stringstream distance;
								distance << FormatWithCommas(int(dist(mypos, pos)));
								int xOffset = int(distance.str().size()*fontW / 2);
								font.Draw(x - xOffset, y - 15, fontColor, distance.str());
							}

							DWORD color;
							color = 0x4433ff00;
							DrawCircleProjected(pos, 20.0f, color);
							DrawCircleFilledProjected(pos, 20.0f, color - 0x30000000);
						}
					}
					
					if (chr.IsPlayer() && int(chr.GetCurrentHealth()) > 0)
					{
						// Ally Player
						if (AllyPlayerFloaters && chr.GetAttitude() == GW2::ATTITUDE_FRIENDLY)
						{
							if (FloatersType) // Health
							{
								std::stringstream health;
								health << FormatWithCommas(int(chr.GetMaxHealth()));
								int xOffset = int(health.str().size()*fontW / 2);
								font.Draw(x - xOffset, y - 15, fontColor, health.str());
							}
							else // Distance
							{
								Vector3 pos = ag.GetPos();
								std::stringstream distance;
								distance << FormatWithCommas(int(dist(mypos, pos)));
								//distance << chr.GetWvwSupply();
								int xOffset = int(distance.str().size()*fontW / 2);
								font.Draw(x - xOffset, y - 15, fontColor, distance.str());
							}

							DWORD color;
							color = 0x4433ff00;
							DrawCircleProjected(pos, 20.0f, color);
							DrawCircleFilledProjected(pos, 20.0f, color - 0x30000000);
						}

						// Enemy Player
						if (EnemyPlayerFloaters && chr.GetAttitude() == GW2::ATTITUDE_HOSTILE)
						{
							if (FloatersType) // Health
							{
								std::stringstream health;
								health << FormatWithCommas(int(chr.GetMaxHealth()));
								int xOffset = int(health.str().size()*fontW / 2);
								font.Draw(x - xOffset, y - 15, fontColor, health.str());
							}
							else // Distance
							{
								Vector3 pos = ag.GetPos();
								std::stringstream distance;
								distance << FormatWithCommas(int(dist(mypos, pos)));
								int xOffset = int(distance.str().size()*fontW / 2);
								font.Draw(x - xOffset, y - 15, fontColor, distance.str());
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
	if (FloaterStats)
	{
		int allyNpc = 0;
		int foeNpc = 0;
		int allyPlayer = 0;
		int foePlayer = 0;

		int tallyGuardian = 0;
		int tallyWarrior = 0;
		int tallyEngineer = 0;
		int tallyRanger = 0;
		int tallyThief = 0;
		int tallyElementalist = 0;
		int tallyMesmer = 0;
		int tallyNecromancer = 0;

		Agent ag;
		while (ag.BeNext())
		{
			Character chr = ag.GetCharacter();
			Vector3 pos = ag.GetPos();

			if (chr.IsControlled())
				continue;

			if (dist(mypos, pos) >= FloatersRange)
				continue;

			if (ag.GetCategory() == GW2::AGENT_CATEGORY_CHAR)
			{
				if (!chr.IsPlayer() && int(chr.GetCurrentHealth()) > 0 && int(chr.GetMaxHealth()) > 1)
				{
					if (EnemyFloaters && (chr.GetAttitude() == GW2::ATTITUDE_HOSTILE || chr.GetAttitude() == GW2::ATTITUDE_INDIFFERENT))
						foeNpc++;
					if (AllyFloaters && (chr.GetAttitude() == GW2::ATTITUDE_FRIENDLY || chr.GetAttitude() == GW2::ATTITUDE_NEUTRAL))
						allyNpc++;
				}
				if (chr.IsPlayer() && int(chr.GetCurrentHealth()) > 0)
				{
					if (AllyPlayerFloaters && chr.GetAttitude() == GW2::ATTITUDE_FRIENDLY)
						allyPlayer++;
					if (EnemyPlayerFloaters && chr.GetAttitude() == GW2::ATTITUDE_HOSTILE)
						foePlayer++;

					switch (chr.GetProfession())
					{
					case GW2::PROFESSION_GUARDIAN:
						tallyGuardian++;
						break;
					case GW2::PROFESSION_WARRIOR:
						tallyWarrior++;
						break;
					case GW2::PROFESSION_ENGINEER:
						tallyEngineer++;
						break;
					case GW2::PROFESSION_RANGER:
						tallyRanger++;
						break;
					case GW2::PROFESSION_THIEF:
						tallyThief++;
						break;
					case GW2::PROFESSION_ELEMENTALIST:
						tallyElementalist++;
						break;
					case GW2::PROFESSION_MESMER:
						tallyMesmer++;
						break;
					case GW2::PROFESSION_NECROMANCER:
						tallyNecromancer++;
						break;
					}
				}
			}
		}
		
		std::stringstream ss;
		ss << "R:" << FormatWithCommas(FloatersRange);
		if (AllyFloaters) ss << " | AllyNpc: " << allyNpc;
		if (EnemyFloaters) ss << " | FoeNpc: " << foeNpc;
		if (AllyPlayerFloaters) ss << " | Allies: " << allyPlayer;
		if (EnemyPlayerFloaters) ss << " | Foes: " << foePlayer;
		
		
		SIZE size = ssSize(ss.str(), 16);
		int x = int(GetWindowWidth() / 2); int y = 45;
		x -= size.cx / 2; y -= size.cy / 2;
		int xPad = 5; int yPad = 2;

		DrawRectFilled(x - xPad, y - yPad, size.cx + xPad * 2, size.cy + yPad * 2, backColor - 0x44000000);
		DrawRect(x - xPad, y - yPad, size.cx + xPad * 2, size.cy + yPad * 2, 0xff444444);
		font.Draw(x, y, fontColor, ss.str());
		
		ss.str("");
		if (FloaterClasses)
		{
			size = ssSize("Ranger: 00", 16);
			x = int(GetWindowWidth() / 2); y = 45 + 30;
			x -= size.cx / 2; y -= size.cy / 2;
			int xPad = 5; int yPad = 2;

			DrawRectFilled(x - xPad, y - yPad, size.cx + xPad * 2, size.cy * 7 + yPad * 7, backColor - 0x44000000);
			DrawRect(x - xPad, y - yPad, size.cx + xPad * 2, size.cy * 7 + yPad * 7, 0xff444444);

			ss << "Guard: " << tallyGuardian << " ";
			font.Draw(x, y, fontColor, ss.str());
			ss.str("");

			y += 15;
			ss << "War: " << tallyWarrior << " ";
			font.Draw(x, y, fontColor, ss.str());
			ss.str("");
			
			y += 15;
			ss << "Ele: " << tallyElementalist << " ";
			font.Draw(x, y, fontColor, ss.str());
			ss.str("");

			y += 15;
			ss << "Mes: " << tallyMesmer << " ";
			font.Draw(x, y, fontColor, ss.str());
			ss.str("");

			y += 15;
			ss << "Thief: " << tallyThief << " ";
			font.Draw(x, y, fontColor, ss.str());
			ss.str("");

			y += 15;
			ss << "Necro: " << tallyNecromancer << " ";
			font.Draw(x, y, fontColor, ss.str());
			ss.str("");

			y += 15;
			ss << "Ranger: " << tallyRanger << " ";
			font.Draw(x, y, fontColor, ss.str());
			ss.str("");

			y += 15;
			ss << "Engi: " << tallyEngineer << " ";
			font.Draw(x, y, fontColor, ss.str());
			ss.str("");
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
	RegisterHotKey(NULL, 23, MOD_ALT | MOD_NOREPEAT, 0x32); // Floaters Enemy NPC
	RegisterHotKey(NULL, 24, MOD_ALT | MOD_NOREPEAT, 0x33); // Floaters Ally Players
	RegisterHotKey(NULL, 25, MOD_ALT | MOD_NOREPEAT, 0x34); // Flaoters Enemy Players
	RegisterHotKey(NULL, 26, MOD_ALT | MOD_NOREPEAT, 0x39); // Floater Stats
	RegisterHotKey(NULL, 27, MOD_ALT | MOD_NOREPEAT, 0x38); // Floater Classes

	RegisterHotKey(NULL, 28, MOD_ALT, 0x6B); // Floaters Range +
	RegisterHotKey(NULL, 29, MOD_ALT, 0x6D); // Flaoters Range -

	// Selected
	RegisterHotKey(NULL, 30, MOD_ALT | MOD_NOREPEAT, 0x53); // Selected Health/Percent
	RegisterHotKey(NULL, 31, MOD_ALT | MOD_NOREPEAT, 0x52); // Selected Range
	RegisterHotKey(NULL, 32, MOD_ALT | MOD_NOREPEAT, 0x49); // Selected Debug
	
	// Self
	RegisterHotKey(NULL, 40, MOD_ALT | MOD_NOREPEAT, 0x50); // Self Health Percent

	// Misc
	RegisterHotKey(NULL, 50, MOD_ALT | MOD_NOREPEAT, 0x4B); // Kill Timer
	RegisterHotKey(NULL, 51, MOD_ALT | MOD_NOREPEAT, 0x4D); // Measure Distance
	RegisterHotKey(NULL, 52, MOD_ALT | MOD_NOREPEAT, 0x43); // Ally Player list
	RegisterHotKey(NULL, 53, MOD_ALT | MOD_NOREPEAT, 0x56); // Ally Player list distance

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
			if (msg.wParam == 23) EnemyFloaters = !EnemyFloaters;
			if (msg.wParam == 24) AllyPlayerFloaters = !AllyPlayerFloaters;
			if (msg.wParam == 25) EnemyPlayerFloaters = !EnemyPlayerFloaters;
			if (msg.wParam == 26) FloaterStats = !FloaterStats;
			if (msg.wParam == 27) FloaterClasses = !FloaterClasses;
			
			if (msg.wParam == 28) if (FloatersRange < 15000) FloatersRange += 100;
			if (msg.wParam == 29) if (FloatersRange > 100) FloatersRange -= 100;
			

			// Selected
			if (msg.wParam == 30) { SelectedHealth = !SelectedHealth; SelectedHealthPercent = !SelectedHealthPercent; }
			if (msg.wParam == 31) DistanceToSelected = !DistanceToSelected;
			if (msg.wParam == 32) SelectedDebug = !SelectedDebug;

			// Self
			if (msg.wParam == 40) SelfHealthPercent = !SelfHealthPercent;

			// Kill Timer
			if (msg.wParam == 50) KillTime = !KillTime;
			if (msg.wParam == 51) MeasureDistance = !MeasureDistance;
			if (msg.wParam == 52) AllyPlayers = !AllyPlayers;
			if (msg.wParam == 53) AllyPlayersRange = !AllyPlayersRange;
		}
	}
}
void CodeMain(){
	NewThread(DpsBuffer);
	NewThread(HotKey);
	EnableEsp(cbESP);
	
	if (!font.Init(16, "Verdana"))
		DbgOut("could not init Font");
}

GW2LIBInit(CodeMain);
