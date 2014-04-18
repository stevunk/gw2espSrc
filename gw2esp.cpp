/*
Compile to a DLL and inject into your running gw2.exe process
*/

#include <locale>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <numeric>
#include <math.h>
#include <sys/timeb.h>
#include <assert.h>
#include <Windows.h>
#include <boost/circular_buffer.hpp> // Boost v1.55 library
#include "gw2lib.h" // GW2Lib library by Rafi

using namespace GW2LIB;

// ESP Elements
bool Help = false;

bool DpsMeter = true;
bool DpsDebug = false;
bool DpsToFile = false;
bool DpsToFile100ms = false;
bool DpsToFileIgnoreZero = false;
bool AllowNegativeDps = false;
bool DpsLock = false;

int  FloatersRange = 7000;
bool Floaters = false;
bool FloaterStats = false;
bool FloaterClasses = false;
bool FloatersType = true;
bool EnemyFloaters = true;
bool AllyFloaters = true;
bool EnemyPlayerFloaters = true;
bool AllyPlayerFloaters = true;
bool AllySupply = false;

bool SelectedHealth = true;
bool SelectedHealthPercent = true;
bool DistanceToSelected = false;
bool SelectedDebug = false;

bool SelfHealthPercent = true;

bool KillTime = false;
bool AttackRate = false;
float AttackRateMin = 0.5;
bool MeasureDistance = false;
bool Speedometer = false;
bool AllyPlayers = false;
bool AllyPlayersVit = false;
bool AllyPlayersSelf = false;

bool WorldBossDebug = false;

// Global Vars
HWND hwnd = FindWindowEx(NULL, NULL, L"Guild Wars 2", NULL);
Font font;
static const DWORD fontColor = 0xffffffff;
static const DWORD backColor = 0xff000000;
boost::circular_buffer<int> dpsBuffer(50);
boost::circular_buffer<float> speedBuffer(50);
boost::circular_buffer<float> attackrateBuffer(50);
float timer[3] {0, 0, 0};
int dpsThis = NULL;
Vector3 MeasureDistanceStart = { 0, 0, 0 };
int wvwBonus = 0;
struct ally {
	int level;
	int health;
	std::string name;
	short int supply;
};
struct allies {
	std::vector<ally> war;
	std::vector<ally> necro;

	std::vector<ally> mes;
	std::vector<ally> ranger;
	std::vector<ally> engi;

	std::vector<ally> guard;
	std::vector<ally> ele;
	std::vector<ally> thief;
};
struct AttackRateVar {
	float cHealth = 0;
	float mHealth = 0;
};
AttackRateVar attackRateVar;

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

	size_t i;
	i = count(ss.begin(), ss.end(), ':'); size.cx -= i * 2;
	i = count(ss.begin(), ss.end(), ' '); size.cx -= i * 2;
	i = count(ss.begin(), ss.end(), '['); size.cx -= i * 1;
	i = count(ss.begin(), ss.end(), ']'); size.cx -= i * 1;

	return size;
}
struct baseHpReturn {
	float health;
	float vitality;
};
baseHpReturn baseHp(int lvl, int profession)
{
	// base stats
	float hp = 0;
	float vit = 16;

	// calc base vit
	if (lvl >= 0 && lvl <= 9) vit += (lvl - 0) * 4; if (lvl >  9) vit += 10 * 4;
	if (lvl >= 10 && lvl <= 19) vit += (lvl - 9) * 6; if (lvl > 19) vit += 10 * 6;
	if (lvl >= 20 && lvl <= 29) vit += (lvl - 19) * 8; if (lvl > 29) vit += 10 * 8;
	if (lvl >= 30 && lvl <= 39) vit += (lvl - 29) * 10; if (lvl > 39) vit += 10 * 10;
	if (lvl >= 40 && lvl <= 49) vit += (lvl - 39) * 12; if (lvl > 49) vit += 10 * 12;
	if (lvl >= 50 && lvl <= 59) vit += (lvl - 49) * 14; if (lvl > 59) vit += 10 * 14;
	if (lvl >= 60 && lvl <= 69) vit += (lvl - 59) * 16; if (lvl > 69) vit += 10 * 16;
	if (lvl >= 70 && lvl <= 79) vit += (lvl - 69) * 18; if (lvl > 79) vit += 10 * 18;
	if (lvl >= 80 && lvl <= 89) vit += (lvl - 79) * 20; if (lvl > 89) vit += 10 * 20;

	// calc base hp
	switch (profession)
	{
	case GW2::PROFESSION_WARRIOR:
	case GW2::PROFESSION_NECROMANCER:
		hp = lvl * 28;
		if (lvl > 19) hp += (lvl - 19) * 42;
		if (lvl > 39) hp += (lvl - 39) * 70;
		if (lvl > 59) hp += (lvl - 59) * 70;
		if (lvl > 79) hp += (lvl - 79) * 70;
		hp += vit * 10;
		break;
	case GW2::PROFESSION_ENGINEER:
	case GW2::PROFESSION_RANGER:
	case GW2::PROFESSION_MESMER:
		hp = lvl * 18;
		if (lvl > 19) hp += (lvl - 19) * 27;
		if (lvl > 39) hp += (lvl - 39) * 45;
		if (lvl > 59) hp += (lvl - 59) * 45;
		if (lvl > 79) hp += (lvl - 79) * 45;
		hp += vit * 10;
		break;
	case GW2::PROFESSION_GUARDIAN:
	case GW2::PROFESSION_ELEMENTALIST:
	case GW2::PROFESSION_THIEF:
		hp = lvl * 5;
		if (lvl > 19) hp += (lvl - 19) * 7.5;
		if (lvl > 39) hp += (lvl - 39) * 12.5;
		if (lvl > 59) hp += (lvl - 59) * 12.5;
		if (lvl > 79) hp += (lvl - 79) * 12.5;
		hp += vit * 10;
		break;
	}

	baseHpReturn out;
	out.health = hp;
	out.vitality = vit;
	return out;
}
int getMilliCount(){
	timeb tb;
	ftime(&tb);
	int nCount = tb.millitm + (tb.time & 0xfffff) * 1000;
	return nCount;
}
int getMilliSpan(int nTimeStart){
	int nSpan = getMilliCount() - nTimeStart;
	if (nSpan < 0)
		nSpan += 0x100000 * 1000;
	return nSpan;
}

void cbESP()
{
	Character me = GetOwnCharacter();
	Vector3 mypos = me.GetAgent().GetPos();
	Agent agLocked = GetLockedSelection();

	// WORLD BOSS DEBUGGING [Alt B]
	if (WorldBossDebug)
	{
		struct wboss {
			int id;
			unsigned long * pointer;
			float cHealth;
			float mHealth;
		};
		boost::circular_buffer<wboss> wbosses(100);

		Agent all;
		int allCount = 0;
		while (all.BeNext())
		{
			if (all.GetType() == GW2::AGENT_TYPE_GADGET_ATTACK_TARGET)
			{
				float cHealth, mHealth;

				Vector3 pos = all.GetPos();
				float x, y;
				if (WorldToScreen(pos, &x, &y))
				{
					unsigned long shift = *(unsigned long*)all.m_ptr;
					shift = *(unsigned long*)(shift + 0x30);
					shift = *(unsigned long*)(shift + 0x28);
					shift = *(unsigned long*)(shift + 0x178);

					cHealth = *(float*)(shift + 0x8);
					mHealth = *(float*)(shift + 0xC);

					std::stringstream ss;
					ss << FormatWithCommas(int(mHealth)) << " hp | " << FormatWithCommas(int(dist(mypos, pos))) << " in";
					
					SIZE size = ssSize(ss.str(), 16);
					int xPad = 5; int yPad = 2;
					y -= size.cy;

					DrawRectFilled(floor(x - xPad - size.cx / 2), floor(y - yPad), floor(size.cx + xPad * 2), floor(size.cy + yPad * 2), backColor - 0x44000000);
					DrawRect(floor(x - xPad - size.cx / 2), floor(y - yPad), floor(size.cx + xPad * 2), floor(size.cy + yPad * 2), 0xff444444);
					font.Draw(floor(x - size.cx / 2), floor(y), fontColor, ss.str());

					DWORD color;
					color = 0x44ff3300;
					DrawCircleProjected(pos, 20.0f, color);
					DrawCircleFilledProjected(pos, 20.0f, color - 0x30000000);
				}

				wboss push;
				push.id = all.GetAgentId();
				push.pointer = (unsigned long*)all.m_ptr;
				push.cHealth = cHealth;
				push.mHealth = mHealth;

				wbosses.push_front(push);
				allCount++;
			}
		}

		std::stringstream ss;
		ss << "WBoss Agents: " << allCount;
		SIZE size = ssSize(ss.str(), 16);
		int x = 10; int y = 30;
		int xPad = 5; int yPad = 2;

		DrawRectFilled(x - xPad, y - yPad, size.cx + xPad * 2, size.cy + yPad * 2, backColor - 0x44000000);
		DrawRect(x - xPad, y - yPad, size.cx + xPad * 2, size.cy + yPad * 2, 0xff444444);
		font.Draw(x, y, fontColor, "WBoss Agents: %i", allCount);
	}
	

	if (Help)
	{
		SIZE size = ssSize("[0] [Alt V] Ally Player Vitality (Alt PgUp/PgDown adjust wvwBonus)", 16);
		float x = int(GetWindowWidth() / 2 - size.cx / 2);
		float y = 150;
		int xPad = 5; int yPad = 2;

		DrawRectFilled(x - xPad, y + 15 - yPad, size.cx + xPad * 2, size.cy*33 + yPad * 4, backColor - 0x44000000);
			  DrawRect(x - xPad, y + 15 - yPad, size.cx + xPad * 2, size.cy*33 + yPad * 4, 0xff444444);

		font.Draw(x, y + 15 * 1, fontColor,  "[%i] [Alt /] Toggle this Help screen", SelectedHealth);

		font.Draw(x, y + 15 * 3, fontColor,  "[%i] [Alt D] DPS Meter", DpsMeter);
		font.Draw(x, y + 15 * 4, fontColor,  "[%i] [Alt H] DPS Meter Debug", DpsDebug);
		font.Draw(x, y + 15 * 5, fontColor,  "[%i] [Alt N] DPS Meter AllowNegativeDPS", AllowNegativeDps);
		font.Draw(x, y + 15 * 6, fontColor,  "[%i] [Alt L] DPS Meter/Timer LockOnCurrentlySelected", DpsLock);
		font.Draw(x, y + 15 * 7, fontColor,  "[%i] [Alt W] Log DPS To File (GW2's folder)", DpsToFile);
		font.Draw(x, y + 15 * 8, fontColor,  "[%i] [Alt E] Log DPS To File (ignore 0 dps)", DpsToFileIgnoreZero);
		font.Draw(x, y + 15 * 9, fontColor,  "[%i] [Alt Q] Log DPS Faster (dmg per 100ms)", DpsToFile100ms);

		font.Draw(x, y + 15 * 11, fontColor, "[%i] [Alt F] Floaters", Floaters);
		font.Draw(x, y + 15 * 12, fontColor, "[-] [Alt +] Floaters Range + (%i)", FloatersRange);
		font.Draw(x, y + 15 * 13, fontColor, "[-] [Alt -] Floaters Range - (%i)", FloatersRange);
		font.Draw(x, y + 15 * 14, fontColor, "[%i] [Alt 9] Floater Count", FloaterStats);
		font.Draw(x, y + 15 * 15, fontColor, "[%i] [Alt 8] Floater Classes (ally players)", FloaterClasses);
		font.Draw(x, y + 15 * 16, fontColor, "[%i] [Alt 0] Floaters Type (Distance or Health)", FloatersType);

		font.Draw(x, y + 15 * 18, fontColor, "[%i] [Alt 1] Floaters on Ally NPC", AllyFloaters);
		font.Draw(x, y + 15 * 19, fontColor, "[%i] [Alt 2] Floaters on Enemy NPC", EnemyFloaters);
		font.Draw(x, y + 15 * 20, fontColor, "[%i] [Alt 3] Floaters on Ally Players", AllyPlayerFloaters);
		font.Draw(x, y + 15 * 21, fontColor, "[%i] [Alt 4] Floaters on Enemy Players", EnemyPlayerFloaters);

		font.Draw(x, y + 15 * 23, fontColor, "[%i] [Alt S] Selected Health/Percent", SelectedHealth);
		font.Draw(x, y + 15 * 24, fontColor, "[%i] [Alt R] Selected Range", DistanceToSelected);
		font.Draw(x, y + 15 * 25, fontColor, "[%i] [Alt I] Selected Debug", SelectedDebug);

		font.Draw(x, y + 15 * 27, fontColor, "[%i] [Alt P] Self Health Percent", SelfHealthPercent);

		font.Draw(x, y + 15 * 29, fontColor, "[%i] [Alt T] Kill Timer", KillTime);
		font.Draw(x, y + 15 * 30, fontColor, "[%i] [Alt A] AttackRate Timer (Alt \"[\" & \"]\" to mod threshold)", AttackRate, AttackRateMin);
		font.Draw(x, y + 15 * 31, fontColor, "[%i] [Alt M] Measure Distance", MeasureDistance);
		font.Draw(x, y + 15 * 32, fontColor, "[%i] [Alt ,] Speedometer", Speedometer);
		font.Draw(x, y + 15 * 33, fontColor, "[%i] [Alt C] Ally Player Info", AllyPlayers);
		font.Draw(x, y + 15 * 34, fontColor, "[%i] [Alt V] Ally Player Vitality (Alt PgUp/PgDown adjust wvwBonus)", AllyPlayersVit);
		font.Draw(x, y + 15 * 35, fontColor, "[%i] [Alt B] World Boss Debug)", WorldBossDebug);
	}

	if (DpsMeter)
	{
		if (!DpsLock)
		{
			if (agLocked.IsValid())
			{
				Character chLocked = agLocked.GetCharacter();
				
				float cHealth = 0, mHealth = 0;
				if (agLocked.GetType() == GW2::AGENT_TYPE_GADGET) {
					unsigned long shift = *(unsigned long*)agLocked.m_ptr;
					shift = *(unsigned long*)(shift + 0x30);
					shift = *(unsigned long*)(shift + 0x164);

					if (shift)
					{
						cHealth = *(float*)(shift + 0x8);
						mHealth = *(float*)(shift + 0xC);
					}
				}
				else if (agLocked.GetType() == GW2::AGENT_TYPE_GADGET_ATTACK_TARGET)
				{
					unsigned long shift = *(unsigned long*)agLocked.m_ptr;
					shift = *(unsigned long*)(shift + 0x30);
					shift = *(unsigned long*)(shift + 0x28);
					shift = *(unsigned long*)(shift + 0x178);

					if (shift)
					{
						cHealth = *(float*)(shift + 0x8);
						mHealth = *(float*)(shift + 0xC);
					}
				}
				else if (agLocked.GetType() == GW2::AGENT_CATEGORY_CHAR)
				{
					cHealth = chLocked.GetCurrentHealth();
					mHealth = chLocked.GetMaxHealth();
				}

				attackRateVar.cHealth = cHealth;
				attackRateVar.mHealth = mHealth;
				
				dpsThis = agLocked.GetAgentId();
			}
				
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
		int x = int(GetWindowWidth() / 4 * 3); int y = 15 + size.cy + yPad * 2;
		x -= 120 / 2; y -= (size.cy * 2 + yPad * 2) / 2;

		DWORD backColorSpecial = 0xff000000;
		if (DpsToFile && dpsThis)
		{
			backColorSpecial = 0xff550000;
		}

		DrawRectFilled(x - xPad, y - yPad - size.cy / 2 - yPad / 2, size.cx + xPad * 2, size.cy * 2 + yPad * 3, backColorSpecial - 0x44000000);
		      DrawRect(x - xPad, y - yPad - size.cy / 2 - yPad / 2, size.cx + xPad * 2, size.cy * 2 + yPad * 3, 0xff444444);
		font.Draw(x, y - size.cy/2 - yPad/2, fontColor, dp1S.str());
		font.Draw(x, y + size.cy/2 + yPad/2, fontColor, dp5S.str());

		if (DpsDebug)
		{
			y += 45; size.cx = 170;
			DrawRectFilled(x - xPad, y - yPad - size.cy / 2 - yPad / 2, size.cx + xPad * 2, size.cy * 47 + yPad * 2, backColorSpecial - 0x44000000);
			DrawRect(x - xPad, y - yPad - size.cy / 2 - yPad / 2, size.cx + xPad * 2, size.cy * 47 + yPad * 2, 0xff444444);

			for (int i = 0; i < 50; i++)
			{
				std::stringstream dps;
				dps << "DMG/100ms: " << FormatWithCommas(dpsBuffer[i]);
				font.Draw(x, 8 + 15 * (float(i) + 3), fontColor, dps.str());
			}
		}
	}

	if (AttackRate && DpsMeter)
	{
		SIZE size = ssSize("Hit #50: 12.123s", 16);

		int xPad = 5; int yPad = 2;
		int x = int(GetWindowWidth() - size.cy + 6); int y = 15 + size.cy + yPad * 2;
		x -= 120 / 2; y -= (size.cy * 2 + yPad * 2) / 2;

		x -= 70;
		DrawRectFilled(x - xPad, y - yPad - size.cy / 2 - yPad / 2, size.cx + xPad * 2, size.cy * 53 + yPad * -1, backColor - 0x44000000);
		DrawRect(x - xPad, y - yPad - size.cy / 2 - yPad / 2, size.cx + xPad * 2, size.cy * 53 + yPad * -1, 0xff444444);

		float tickAverage = 0;
		float tickMin = 0;
		float tickMax = 0;
		int tickCount = 0;
		for (int i = 0; i < 50; i++)
		{
			if (attackrateBuffer[i] > 0)
				font.Draw(x, 8 + 15 * float(i), fontColor, "Hit #%02i: %0.3fs", i+1, attackrateBuffer[i]);
			else
				font.Draw(x, 8 + 15 * float(i), fontColor, "Hit #%02i: ...", i+1);

			if (attackrateBuffer[i] > 0)
			{
				tickAverage += attackrateBuffer[i];
				tickCount++;

				if (tickMin == 0)
					tickMin = attackrateBuffer[i];
				if (tickMax == 0)
					tickMax = attackrateBuffer[i];

				if (attackrateBuffer[i] < tickMin)
					tickMin = attackrateBuffer[i];
				if (attackrateBuffer[i] > tickMax)
					tickMax = attackrateBuffer[i];
			}
		}
		if (tickCount > 0)
			tickAverage = tickAverage / tickCount;

		int colPad = 40;

		font.Draw(x, 8 + 15 * float(51), fontColor, "Min:");
		font.Draw(x + colPad, 8 + 15 * float(51), fontColor, "%0.3fs", tickMin);

		font.Draw(x, 8 + 15 * float(52), fontColor, "Avg:");
		if (tickAverage > 0)
			font.Draw(x + colPad, 8 + 15 * float(52), fontColor, "%0.3fs", tickAverage);
		else
			font.Draw(x + colPad, 8 + 15 * float(52), fontColor, "0.000s");

		font.Draw(x, 8 + 15 * float(53), fontColor, "Max:");
		font.Draw(x + colPad, 8 + 15 * float(53), fontColor, "%0.3fs", tickMax);

		font.Draw(x, 8 + 15 * float(55), fontColor, "Threshold: %0.1fs", AttackRateMin);
	}

	if (KillTime)
	{
		int hr = floor(timer[2] / 60 / 60);
		int min = floor((timer[2] - hr * 60 * 60) / 60);
		float sec = timer[2] - hr * 60 * 60 - min * 60;

		std::stringstream ss;
		ss << "Timer:";
		if (hr > 0) ss << " " << hr << "h";
		if (min > 0) ss << " " << min << "m";
		ss << " " << std::fixed << std::setprecision(1) << sec << "s";

		SIZE size = ssSize(ss.str(), 16);
		int xPad = 5; int yPad = 2;
		int x = int(GetWindowWidth() / 4 * 3); int y = 15+1;
		x -= size.cx + 76; y -= size.cy / 2;

		DrawRectFilled(x - xPad, y - yPad, size.cx + xPad * 2, size.cy + yPad * 2, backColor - 0x44000000);
		DrawRect(x - xPad, y - yPad, size.cx + xPad * 2, size.cy + yPad * 2, 0xff444444);
		font.Draw(x, y, fontColor, ss.str());
	}

	if (agLocked.m_ptr)
	{
		Character chrLocked = agLocked.GetCharacter();
		
		int leftAlignX;

		if (SelectedHealth | SelectedHealthPercent)
		{
			if (agLocked.IsValid())
			{
				float cHealth = 0, mHealth = 0;
				if (agLocked.GetType() == GW2::AGENT_TYPE_GADGET) {
					unsigned long shift = *(unsigned long*)agLocked.m_ptr;
					shift = *(unsigned long*)(shift + 0x30);
					shift = *(unsigned long*)(shift + 0x164);
					
					if (shift)
					{
						cHealth = *(float*)(shift + 0x8);
						mHealth = *(float*)(shift + 0xC);
					}
				}
				else if (agLocked.GetType() == GW2::AGENT_TYPE_GADGET_ATTACK_TARGET)
				{
					unsigned long shift = *(unsigned long*)agLocked.m_ptr;
					shift = *(unsigned long*)(shift + 0x30);
					shift = *(unsigned long*)(shift + 0x28);
					shift = *(unsigned long*)(shift + 0x178);

					if (shift)
					{
						cHealth = *(float*)(shift + 0x8);
						mHealth = *(float*)(shift + 0xC);
					}
				}
				else if (agLocked.GetType() == GW2::AGENT_CATEGORY_CHAR)
				{
					cHealth = chrLocked.GetCurrentHealth();
					mHealth = chrLocked.GetMaxHealth();
				}
				
				std::stringstream ss;
				if (SelectedHealth)
					ss << "Selected: " << FormatWithCommas(int(cHealth)) << " / " << FormatWithCommas(int(mHealth));
				if (SelectedHealthPercent && int(mHealth) > 0)
					ss << " [" << int(cHealth / mHealth * 100) << "%%]";

				SIZE size = ssSize(ss.str(), 16);
				int xPad = 5; int yPad = 2;
				int x = int(GetWindowWidth() / 4); int y = 15;
				x -= size.cx / 2; y -= size.cy / 2;

				leftAlignX = x;
				DrawRectFilled(x - xPad, y - yPad, size.cx + xPad * 2, size.cy + yPad * 2, backColor - 0x44000000);
				DrawRect(x - xPad, y - yPad, size.cx + xPad * 2, size.cy + yPad * 2, 0xff444444);
				font.Draw(x, y, fontColor, ss.str());
			}
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
				//x = leftAlignX;
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
			

			SIZE size = ssSize("agPtr: AABBCCDD -> AABBCCDD", 16);
			int xPad = 5; int yPad = 2;

			x -= size.cx / 2;
			y += 15;

			DrawRectFilled(x - xPad, y + 20, size.cx + xPad * 2, size.cy * 5 + yPad * 2, backColor - 0x44000000);
			DrawRect(x - xPad, y + 20, size.cx + xPad * 2, size.cy * 5 + yPad * 2, 0xff444444);


			y += 8;
			font.Draw(x, y + 15 * 1, fontColor, "agPtr: %p <= %p", *(void**)agLocked.m_ptr, (void**)agLocked.m_ptr);
			if (chrLocked.m_ptr)
				font.Draw(x, y + 15 * 2, fontColor, "chPtr: %p", *(void**)chrLocked.m_ptr);
			
			font.Draw(x, y + 15 * 3, fontColor, "agentId: %i / 0x%04X", agLocked.GetAgentId(), agLocked.GetAgentId());
			
			// Lvl
			short int lvl = 0;
			if (agLocked.GetType() == GW2::AGENT_TYPE_CHAR) {
				unsigned long shift;
				shift = *(unsigned long*)agLocked.m_ptr;
				shift = *(unsigned long*)(shift + 0x30);
				shift = *(unsigned long*)(shift + 0x1c);
				shift = *(unsigned long*)(shift + 0xf0);
				lvl = *(short int*)(shift + 0xb0);
			}

			// Supply
			short int supply = 0;
			if (agLocked.GetType() == GW2::AGENT_TYPE_CHAR) {
				unsigned long shift;
				shift = *(unsigned long*)agLocked.m_ptr;
				shift = *(unsigned long*)(shift + 0x30);
				shift = *(unsigned long*)(shift + 0x134);
				supply = *(unsigned long*)(shift + 0x1d8);
			}
			
			font.Draw(x, y + 15 * 4, fontColor, "cat: %i / type: %i", agLocked.GetCategory(), agLocked.GetType());
			font.Draw(x, y + 15 * 5, fontColor, "lvl: %i / supply: %i", lvl, supply);
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
		
		int x = int(GetWindowWidth() / 2); int y = 33;
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

	if (Speedometer)
	{
		float speed1 = 0;
		for (int i = 0; i < 5; i++)
		{
			speed1 += speedBuffer[i];
		}

		float speed5 = 0;
		for (int i = 0; i < 25; i++)
		{
			speed5 += speedBuffer[i];
		}
		speed5 = speed5 / 5;

		std::stringstream ss;
		ss << "Speed: " << FormatWithCommas(int(speed1)) << " in/s | " << FormatWithCommas(int(speed5)) << " in/s";

		int x = int(GetWindowWidth() / 2); int y = 33;
		int xPad = 5; int yPad = 2;
		y += 15 + yPad * 2 + 4;

		SIZE size = ssSize(ss.str(), 16);
		x -= size.cx / 2; y -= size.cy / 2;

		DrawRectFilled(x - xPad, y - yPad, size.cx + xPad * 2, size.cy + yPad * 2, backColor - 0x44000000);
		DrawRect(x - xPad, y - yPad, size.cx + xPad * 2, size.cy + yPad * 2, 0xff444444);
		font.Draw(x, y, fontColor, ss.str());
	}
	
	if (AllyPlayers)
	{
		Agent ag;
		Agent agCount;

		allies allies;
		
		int allyCount = 0;
		while (agCount.BeNext())
		{
			Character chr = agCount.GetCharacter();
			if (!chr.IsValid())
				continue;

			if (chr.IsControlled() && !AllyPlayersSelf)
				continue;

			if (!chr.IsPlayer() || chr.GetAttitude() != GW2::ATTITUDE_FRIENDLY)
				continue;
			

			// read character level
			short int lvl = 0;
			unsigned long shift;
			shift = *(unsigned long*)agCount.m_ptr;
			shift = *(unsigned long*)(shift + 0x30);
			shift = *(unsigned long*)(shift + 0x1c);
			shift = *(unsigned long*)(shift + 0xf0);
			lvl = *(short int*)(shift + 0xb0);

			// compile
			ally ally;
			switch (chr.GetProfession())
			{
			case GW2::PROFESSION_GUARDIAN:
				ally.level = int(lvl);
				ally.health = round(chr.GetMaxHealth() / (100 + wvwBonus) * 100);
				ally.name = chr.GetName();
				allies.guard.push_back(ally);
				break;
			case GW2::PROFESSION_WARRIOR:
				ally.level = int(lvl);
				ally.health = round(chr.GetMaxHealth() / (100 + wvwBonus) * 100);
				ally.name = chr.GetName();
				allies.war.push_back(ally);
				break;
			case GW2::PROFESSION_ENGINEER:
				ally.level = int(lvl);
				ally.health = round(chr.GetMaxHealth() / (100 + wvwBonus) * 100);
				ally.name = chr.GetName();
				allies.engi.push_back(ally);
				break;
			case GW2::PROFESSION_RANGER:
				ally.level = int(lvl);
				ally.health = round(chr.GetMaxHealth() / (100 + wvwBonus) * 100);
				ally.name = chr.GetName();
				allies.ranger.push_back(ally);
				break;
			case GW2::PROFESSION_THIEF:
				ally.level = int(lvl);
				ally.health = round(chr.GetMaxHealth() / (100 + wvwBonus) * 100);
				ally.name = chr.GetName();
				allies.thief.push_back(ally);
				break;
			case GW2::PROFESSION_ELEMENTALIST:
				ally.level = int(lvl);
				ally.health = round(chr.GetMaxHealth() / (100 + wvwBonus) * 100);
				ally.name = chr.GetName();
				allies.ele.push_back(ally);
				break;
			case GW2::PROFESSION_MESMER:
				ally.level = int(lvl);
				ally.health = round(chr.GetMaxHealth() / (100 + wvwBonus) * 100);
				ally.name = chr.GetName();
				allies.mes.push_back(ally);
				break;
			case GW2::PROFESSION_NECROMANCER:
				ally.level = int(lvl);
				ally.health = round(chr.GetMaxHealth() / (100 + wvwBonus) * 100);
				ally.name = chr.GetName();
				allies.necro.push_back(ally);
				break;
			}
			allyCount++;
		}

		float x = 150;
		float y = 20;
		int i = 5;
		int lineHeight = 16;
		
		SIZE maxCharName = ssSize("WWWWWWWWWWWWWWWWWWW", 16);
		
		int col0 = 0;
		int col1 = col0 + 65; // name start
		int col2 = col1 + maxCharName.cx + 10; // hp start
		int col3 = col2 + 85; // +vit start
		int col4 = (AllyPlayersVit) ? col3 + 70 : col3 + 0; // +traits start
		int colx = (AllyPlayersVit) ? col4 + 50 : col4 + 0;

		int xPad = 5; int yPad = 2;
		if (allyCount > 0)
		{
			DrawRectFilled(x - xPad, y - yPad + lineHeight * 2, colx + xPad * 2, allyCount * lineHeight + lineHeight * 3 + yPad * 1, backColor - 0x44000000);
			      DrawRect(x - xPad, y - yPad + lineHeight * 2, colx + xPad * 2, allyCount * lineHeight + lineHeight * 3 + yPad * 1, 0xff444444);
		}
		else
		{
			DrawRectFilled(x - xPad, y - yPad + lineHeight * 2, colx + xPad * 2, allyCount * lineHeight + lineHeight * 4 + yPad * 1, backColor - 0x44000000);
			      DrawRect(x - xPad, y - yPad + lineHeight * 2, colx + xPad * 2, allyCount * lineHeight + lineHeight * 4 + yPad * 1, 0xff444444);
		}
		
		
		font.Draw(x + col0, y + lineHeight * 2, fontColor, "Allies Nearby (WvW HP Bonus: %i%%)", wvwBonus);

		font.Draw(x + col0, y + lineHeight * 4, fontColor, "Class");
		font.Draw(x + col1, y + lineHeight * 4, fontColor, "Name");
		font.Draw(x + col2, y + lineHeight * 4, fontColor, "Health");
		if (AllyPlayersVit) font.Draw(x + col3, y + lineHeight * 4, fontColor, "Vitality");
		if (AllyPlayersVit) font.Draw(x + col4, y + lineHeight * 4, fontColor, "Traits");

		if (allyCount == 0)
		{
			font.Draw(x + col0, y + lineHeight * i, fontColor, "...");
			font.Draw(x + col1, y + lineHeight * i, fontColor, "...");
			font.Draw(x + col2, y + lineHeight * i, fontColor, "...");
			if (AllyPlayersVit) font.Draw(x + col3, y + lineHeight * i, fontColor, "...");
			if (AllyPlayersVit) font.Draw(x + col4, y + lineHeight * i, fontColor, "...");
			DrawRect(x - xPad, y + lineHeight * i, colx + xPad * 2, 0, 0xff444444);
		}

		// warriors
		for (auto & ally : allies.war) {
			baseHpReturn base = baseHp(ally.level, GW2::PROFESSION_WARRIOR);
			std::string hp = FormatWithCommas(int(ally.health));
			
			font.Draw(x + col0, y + lineHeight * i, fontColor, "War:");
			font.Draw(x + col1, y + lineHeight * i, fontColor, ally.name);
			font.Draw(x + col2, y + lineHeight * i, fontColor, "%s hp", hp.c_str());
			if (AllyPlayersVit) font.Draw(x + col3, y + lineHeight * i, fontColor, "%+i", int(round((ally.health - base.health) / 10)));
			if (AllyPlayersVit) font.Draw(x + col4, y + lineHeight * i, fontColor, "%+i", int(round((916 / base.vitality) * ((ally.health - base.health) / 100))));
			DrawRect(x - xPad, y + lineHeight * i, colx + xPad * 2, 0, 0xff444444);

			i++;
		}
		// guards
		for (auto & ally : allies.guard) {
			baseHpReturn base = baseHp(ally.level, GW2::PROFESSION_GUARDIAN);
			std::string hp = FormatWithCommas(int(ally.health));

			font.Draw(x + col0, y + lineHeight * i, fontColor, "Guard:");
			font.Draw(x + col1, y + lineHeight * i, fontColor, ally.name);
			font.Draw(x + col2, y + lineHeight * i, fontColor, "%s hp", hp.c_str());
			if (AllyPlayersVit) font.Draw(x + col3, y + lineHeight * i, fontColor, "%+i", int(round((ally.health - base.health) / 10)));
			if (AllyPlayersVit) font.Draw(x + col4, y + lineHeight * i, fontColor, "%+i", int(round((916 / base.vitality) * ((ally.health - base.health) / 100))));
			DrawRect(x - xPad, y + lineHeight * i, colx + xPad * 2, 0, 0xff444444);

			i++;
		}

		// mes
		for (auto & ally : allies.mes) {
			baseHpReturn base = baseHp(ally.level, GW2::PROFESSION_MESMER);
			std::string hp = FormatWithCommas(int(ally.health));

			font.Draw(x + col0, y + lineHeight * i, fontColor, "Mes:");
			font.Draw(x + col1, y + lineHeight * i, fontColor, ally.name);
			font.Draw(x + col2, y + lineHeight * i, fontColor, "%s hp", hp.c_str());
			if (AllyPlayersVit) font.Draw(x + col3, y + lineHeight * i, fontColor, "%+i", int(round((ally.health - base.health) / 10)));
			if (AllyPlayersVit) font.Draw(x + col4, y + lineHeight * i, fontColor, "%+i", int(round((916 / base.vitality) * ((ally.health - base.health) / 100))));
			DrawRect(x - xPad, y + lineHeight * i, colx + xPad * 2, 0, 0xff444444);

			i++;
		}
		// ele
		for (auto & ally : allies.ele) {
			baseHpReturn base = baseHp(ally.level, GW2::PROFESSION_ELEMENTALIST);
			std::string hp = FormatWithCommas(int(ally.health));

			font.Draw(x + col0, y + lineHeight * i, fontColor, "Ele:");
			font.Draw(x + col1, y + lineHeight * i, fontColor, ally.name);
			font.Draw(x + col2, y + lineHeight * i, fontColor, "%s hp", hp.c_str());
			if (AllyPlayersVit) font.Draw(x + col3, y + lineHeight * i, fontColor, "%+i", int(round((ally.health - base.health) / 10)));
			if (AllyPlayersVit) font.Draw(x + col4, y + lineHeight * i, fontColor, "%+i", int(round((916 / base.vitality) * ((ally.health - base.health) / 100))));
			DrawRect(x - xPad, y + lineHeight * i, colx + xPad * 2, 0, 0xff444444);

			i++;
		}
		// thief
		for (auto & ally : allies.thief) {
			baseHpReturn base = baseHp(ally.level, GW2::PROFESSION_THIEF);
			std::string hp = FormatWithCommas(int(ally.health));

			font.Draw(x + col0, y + lineHeight * i, fontColor, "Thief:");
			font.Draw(x + col1, y + lineHeight * i, fontColor, ally.name);
			font.Draw(x + col2, y + lineHeight * i, fontColor, "%s hp", hp.c_str());
			if (AllyPlayersVit) font.Draw(x + col3, y + lineHeight * i, fontColor, "%+i", int(round((ally.health - base.health) / 10)));
			if (AllyPlayersVit) font.Draw(x + col4, y + lineHeight * i, fontColor, "%+i", int(round((916 / base.vitality) * ((ally.health - base.health) / 100))));
			DrawRect(x - xPad, y + lineHeight * i, colx + xPad * 2, 0, 0xff444444);

			i++;
		}
		// ranger
		for (auto & ally : allies.ranger) {
			baseHpReturn base = baseHp(ally.level, GW2::PROFESSION_RANGER);
			std::string hp = FormatWithCommas(int(ally.health));

			font.Draw(x + col0, y + lineHeight * i, fontColor, "Ranger:");
			font.Draw(x + col1, y + lineHeight * i, fontColor, ally.name);
			font.Draw(x + col2, y + lineHeight * i, fontColor, "%s hp", hp.c_str());
			if (AllyPlayersVit) font.Draw(x + col3, y + lineHeight * i, fontColor, "%+i", int(round((ally.health - base.health) / 10)));
			if (AllyPlayersVit) font.Draw(x + col4, y + lineHeight * i, fontColor, "%+i", int(round((916 / base.vitality) * ((ally.health - base.health) / 100))));
			DrawRect(x - xPad, y + lineHeight * i, colx + xPad * 2, 0, 0xff444444);

			i++;
		}
		// Engi
		for (auto & ally : allies.engi) {
			baseHpReturn base = baseHp(ally.level, GW2::PROFESSION_ENGINEER);
			std::string hp = FormatWithCommas(int(ally.health));

			font.Draw(x + col0, y + lineHeight * i, fontColor, "Engi:");
			font.Draw(x + col1, y + lineHeight * i, fontColor, ally.name);
			font.Draw(x + col2, y + lineHeight * i, fontColor, "%s hp", hp.c_str());
			if (AllyPlayersVit) font.Draw(x + col3, y + lineHeight * i, fontColor, "%+i", int(round((ally.health - base.health) / 10)));
			if (AllyPlayersVit) font.Draw(x + col4, y + lineHeight * i, fontColor, "%+i", int(round((916 / base.vitality) * ((ally.health - base.health) / 100))));
			DrawRect(x - xPad, y + lineHeight * i, colx + xPad * 2, 0, 0xff444444);

			i++;
		}
		// Necro
		for (auto & ally : allies.necro) {
			baseHpReturn base = baseHp(ally.level, GW2::PROFESSION_NECROMANCER);
			std::string hp = FormatWithCommas(int(ally.health));

			font.Draw(x + col0, y + lineHeight * i, fontColor, "Necro:");
			font.Draw(x + col1, y + lineHeight * i, fontColor, ally.name);
			font.Draw(x + col2, y + lineHeight * i, fontColor, "%s hp", hp.c_str());
			if (AllyPlayersVit) font.Draw(x + col3, y + lineHeight * i, fontColor, "%+i", int(round((ally.health - base.health) / 10)));
			if (AllyPlayersVit) font.Draw(x + col4, y + lineHeight * i, fontColor, "%+i", int(round((916 / base.vitality) * ((ally.health - base.health) / 100))));
			DrawRect(x - xPad, y + lineHeight * i, colx + xPad * 2, 0, 0xff444444);

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
								std::stringstream ss;
								ss << FormatWithCommas(int(chr.GetMaxHealth()));
								SIZE size = ssSize(ss.str(), 16);
								int xOffset = int(size.cx / 2);
								font.Draw(x - xOffset, y - 15, fontColor, ss.str());
							}
							else // Distance
							{
								Vector3 pos = ag.GetPos();
								std::stringstream ss;
								ss << FormatWithCommas(int(dist(mypos, pos)));
								SIZE size = ssSize(ss.str(), 16);
								int xOffset = int(size.cx / 2);
								font.Draw(x - xOffset, y - 15, fontColor, ss.str());
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
								std::stringstream ss;
								ss << FormatWithCommas(int(chr.GetMaxHealth()));
								SIZE size = ssSize(ss.str(), 16);
								int xOffset = int(size.cx / 2);
								font.Draw(x - xOffset, y - 15, fontColor, ss.str());
							}
							else // Distance
							{
								Vector3 pos = ag.GetPos();
								std::stringstream ss;
								ss << FormatWithCommas(int(dist(mypos, pos)));
								SIZE size = ssSize(ss.str(), 16);
								int xOffset = int(size.cx / 2);
								font.Draw(x - xOffset, y - 15, fontColor, ss.str());
							}

							DWORD color;
							color = 0x4433ff00;
							DrawCircleProjected(pos, 20.0f, color);
							DrawCircleFilledProjected(pos, 20.0f, color - 0x30000000);
						}
					}
					
					if (chr.IsPlayer() && int(chr.GetCurrentHealth()) > 0)
					{
						// Supply
						short int supply = 0;
						if (0) {
							unsigned long shift;
							shift = *(unsigned long*)ag.m_ptr;
							shift = *(unsigned long*)(shift + 0x30);
							shift = *(unsigned long*)(shift + 0x134);
							supply = *(unsigned long*)(shift + 0x1d8);
						}

						// Ally Player
						if (AllyPlayerFloaters && chr.GetAttitude() == GW2::ATTITUDE_FRIENDLY)
						{
							if (FloatersType) // Health
							{
								std::stringstream ss;
								ss << FormatWithCommas(int(chr.GetMaxHealth()));
								SIZE size = ssSize(ss.str(), 16);
								int xOffset = int(size.cx / 2);
								font.Draw(x - xOffset, y - 15, fontColor, ss.str());
							}
							else // Distance
							{
								Vector3 pos = ag.GetPos();
								std::stringstream ss;
								ss << FormatWithCommas(int(dist(mypos, pos)));
								//ss << supply;
								SIZE size = ssSize(ss.str(), 16);
								int xOffset = int(size.cx / 2);
								font.Draw(x - xOffset, y - 15, fontColor, ss.str());
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
								std::stringstream ss;
								ss << FormatWithCommas(int(chr.GetMaxHealth()));
								SIZE size = ssSize(ss.str(), 16);
								int xOffset = int(size.cx / 2);
								font.Draw(x - xOffset, y - 15, fontColor, ss.str());
							}
							else // Distance
							{
								Vector3 pos = ag.GetPos();
								std::stringstream ss;
								ss << FormatWithCommas(int(dist(mypos, pos)));
								//ss << supply;
								SIZE size = ssSize(ss.str(), 16);
								int xOffset = int(size.cx / 2);
								font.Draw(x - xOffset, y - 15, fontColor, ss.str());
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
		int supply = 0;

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
			{
				// Supply
				short int sup = 0;
				unsigned long shift;
				shift = *(unsigned long*)ag.m_ptr;
				shift = *(unsigned long*)(shift + 0x30);
				shift = *(unsigned long*)(shift + 0x134);
				sup = *(unsigned long*)(shift + 0x1d8);

				supply += int(sup);

				continue;
			}

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
					if (EnemyPlayerFloaters && chr.GetAttitude() == GW2::ATTITUDE_HOSTILE)
						foePlayer++;

					if (AllyPlayerFloaters && chr.GetAttitude() == GW2::ATTITUDE_FRIENDLY)
					{
						allyPlayer++;
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

						// Supply
						short int sup = 0;
						unsigned long shift;
						shift = *(unsigned long*)ag.m_ptr;
						shift = *(unsigned long*)(shift + 0x30);
						shift = *(unsigned long*)(shift + 0x134);
						sup = *(unsigned long*)(shift + 0x1d8);

						supply += int(sup);
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
		if (AllySupply) ss << " | Supply: " << supply;
		
		
		SIZE size = ssSize(ss.str(), 16);
		int x = int(GetWindowWidth() / 2); int y = 9;
		x -= size.cx / 2; y -= size.cy / 2;
		int xPad = 5; int yPad = 2;

		DrawRectFilled(x - xPad, y - yPad, size.cx + xPad * 2, size.cy + yPad * 2, backColor - 0x44000000);
		DrawRect(x - xPad, y - yPad, size.cx + xPad * 2, size.cy + yPad * 2, 0xff444444);
		font.Draw(x, y, fontColor, ss.str());
		
		ss.str("");
		if (FloaterClasses)
		{
			size = ssSize("Ranger: 00", 16);
			x = int(GetWindowWidth() / 2); y += 32;
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
		// timer not visible = reset it
		if (!KillTime && timer[0] != 0)
		{
			timer[0] = 0;
			timer[1] = 0;
			timer[2] = 0;
		}

		if (dpsThis)
		{
			// new Agent, wipe buffer
			if (dpsCurrent != dpsThis){
				previousHealth = NULL;
				for (int i = 0; i < 50; i++)
					dpsBuffer.push_front(0);
				dpsBuffer.clear();

				timer[0] = 0;
				timer[1] = 0;
				timer[2] = 0;
			}

			Agent ag;
			while (ag.BeNext())
			{
				Character ch = ag.GetCharacter();
				if (!ch.IsValid())
					continue;

				if (dpsThis == 0 || ag.GetAgentId() != dpsThis)
					continue;

				// timer
				if (KillTime)
				{
					if (ch.GetCurrentHealth() == ch.GetMaxHealth() || timer[0] == 0)
					{
						timer[0] = clock();
					}
					
					
					if (ch.IsAlive()){
						timer[1] = clock();
						timer[2] = (timer[1] - timer[0]) / 1000;
					}
				}

				// health tracker
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

void LogDps()
{
	using namespace std;

	bool initiated = false;
	bool fileEmpty = true;
	ofstream file;
	string filename = "gw2dpsLog.txt";

	while (1)
	{
		if (!DpsToFile && initiated)
		{
			initiated = false;
		}
		
		if (DpsToFile && DpsMeter && dpsThis)
		{
			if (!initiated)
			{
				file.open(filename);
				file.close();
				initiated = true;
				fileEmpty = true;
			}

			int dps = 0;
			if (DpsToFile100ms)
				dps = dpsBuffer[0];
			else
			{
				for (int i = 0; i < 10; i++)
					dps += dpsBuffer[i];
			}

			file.open(filename, ios::app);
			if (file.is_open())
			{
				if (!(DpsToFileIgnoreZero && dps == 0))
				{
					if (fileEmpty)
					{
						file << dps; fileEmpty = false;
					}
					else
					{
						file << endl << dps;
					}
				}

				if (file.bad())
					DbgOut("Failed writing gw2dpsLog.txt");
			}
			else DbgOut("Failed opening gw2dpsLog.txt");
			file.close();

			if (!DpsToFile100ms)
				Sleep(900);
		}
		
		Sleep(100);
	}
}

void SpeedBuffer()
{
	bool initiated = false;
	while (1)
	{
		if (Speedometer)
		{
			initiated = true;
			float speed;

			Agent me;
			me.BeSelf();

			Vector3 posOld, posNew;
			posOld = me.GetPos();
			
			for (int i = 0; i < 50; i++)
				speedBuffer[i] = 0;

			while (1)
			{
				if (!Speedometer)
					break;
				
				if (!me.IsValid())
					break;

				posNew = me.GetPos();
				speed = dist(posOld, posNew);
				speedBuffer.push_front(speed);
				posOld = posNew;

				Sleep(200);
			}
		}
		else if (initiated)
		{
			for (int i = 0; i < 50; i++)
				speedBuffer[i] = 0;
			initiated = false;
		}

		Sleep(100);
	}
	
}

void AttackrateBuffer() {
	int dpsCurrent = NULL;
	float previousHealth = NULL;
	int lastTick = 0;
	float tick = 0;

	while (true){
		if (AttackRate && dpsThis)
		{
			if (!dpsThis || dpsThis == 0)
				continue;

			// new Agent, wipe buffer
			if (dpsCurrent != dpsThis){
				previousHealth = NULL;
				for (int i = 0; i < 50; i++)
					attackrateBuffer.push_front(0);
				attackrateBuffer.clear();
			}

			
			// health tracker
			float cHealth = attackRateVar.cHealth;
			float mHealth = attackRateVar.mHealth;
			int tickNow = getMilliCount();

			if (!previousHealth)
			{
				previousHealth = cHealth;
			}
			else
			{
				if (cHealth < 0 || cHealth > mHealth)
					continue;

				if (previousHealth > cHealth)
				{
					if (!lastTick)
					{
						lastTick = tickNow;
					}
					else if (getMilliSpan(lastTick) > (AttackRateMin * 1000))
					{
						tick = getMilliSpan(lastTick);
						tick = tick / 1000;
						attackrateBuffer.push_front(tick);
						//attackrateBuffer.push_front(mHealth);

						lastTick = tickNow;
					}
				}
					
				previousHealth = cHealth;
			}
			dpsCurrent = dpsThis;
		}
		else
		{
			previousHealth = NULL;
			lastTick = 0;
			for (int i = 0; i < 50; i++)
				attackrateBuffer.push_front(0);
			attackrateBuffer.clear();
		}
		Sleep(1);
	}
}

void HotKey()
{
	// Help
	RegisterHotKey(NULL, 0, MOD_ALT | MOD_NOREPEAT, VK_OEM_2); // Help
	RegisterHotKey(NULL, 1, MOD_ALT | MOD_NOREPEAT, 0x42); // WorldBossDebug

	// DPS Meter
	RegisterHotKey(NULL, 10, MOD_ALT | MOD_NOREPEAT, 0x44); // DPS Meter
	RegisterHotKey(NULL, 11, MOD_ALT | MOD_NOREPEAT, 0x48); // DPS Meter Debug
	RegisterHotKey(NULL, 12, MOD_ALT | MOD_NOREPEAT, 0x4E); // DPS Meter Allow Negative DPS
	RegisterHotKey(NULL, 13, MOD_ALT | MOD_NOREPEAT, 0x4C); // DPS Meter LockToCurrentlySelected
	RegisterHotKey(NULL, 14, MOD_ALT | MOD_NOREPEAT, 0x57); // Log DPS to file
	RegisterHotKey(NULL, 15, MOD_ALT | MOD_NOREPEAT, 0x51); // Log DPS to file every 100ms
	RegisterHotKey(NULL, 16, MOD_ALT | MOD_NOREPEAT, 0x45); // Log DPS to file Ignore zero dmg

	// Floaters
	RegisterHotKey(NULL, 20, MOD_ALT | MOD_NOREPEAT, 0x46); // Floaters
	RegisterHotKey(NULL, 21, MOD_ALT | MOD_NOREPEAT, 0x30); // Floaters Type (Health/Distance)
	RegisterHotKey(NULL, 22, MOD_ALT | MOD_NOREPEAT, 0x31); // Floaters Ally NPC
	RegisterHotKey(NULL, 23, MOD_ALT | MOD_NOREPEAT, 0x32); // Floaters Enemy NPC
	RegisterHotKey(NULL, 24, MOD_ALT | MOD_NOREPEAT, 0x33); // Floaters Ally Players
	RegisterHotKey(NULL, 25, MOD_ALT | MOD_NOREPEAT, 0x34); // Floaters Enemy Players
	RegisterHotKey(NULL, 26, MOD_ALT | MOD_NOREPEAT, 0x39); // Floater Stats
	RegisterHotKey(NULL, 27, MOD_ALT | MOD_NOREPEAT, 0x38); // Floater Classes

	RegisterHotKey(NULL, 28, MOD_ALT, 0x6B); // Floaters Range +
	RegisterHotKey(NULL, 29, MOD_ALT, 0x6D); // Floaters Range -

	RegisterHotKey(NULL, 299, MOD_ALT | MOD_NOREPEAT, 0x35); // Floaters AllySupplyCount

	// Selected
	RegisterHotKey(NULL, 30, MOD_ALT | MOD_NOREPEAT, 0x53); // Selected Health/Percent
	RegisterHotKey(NULL, 31, MOD_ALT | MOD_NOREPEAT, 0x52); // Selected Range
	RegisterHotKey(NULL, 32, MOD_ALT | MOD_NOREPEAT, 0x49); // Selected Debug
	
	// Self
	RegisterHotKey(NULL, 40, MOD_ALT | MOD_NOREPEAT, 0x50); // Self Health Percent

	// Misc
	RegisterHotKey(NULL, 50, MOD_ALT | MOD_NOREPEAT, 0x54); // Kill Timer
	RegisterHotKey(NULL, 51, MOD_ALT | MOD_NOREPEAT, 0x4D); // Measure Distance
	RegisterHotKey(NULL, 52, MOD_ALT | MOD_NOREPEAT, 0x43); // Ally Player list
	RegisterHotKey(NULL, 53, MOD_ALT | MOD_NOREPEAT, 0x56); // Ally Player list +vit
	RegisterHotKey(NULL, 54, MOD_ALT | MOD_NOREPEAT, 0x58); // Ally Player list +self
	RegisterHotKey(NULL, 55, MOD_ALT | MOD_NOREPEAT, 0xBC); // Speedometer
	RegisterHotKey(NULL, 56, MOD_ALT | MOD_NOREPEAT, 0x41); // AttackRate
	RegisterHotKey(NULL, 57, MOD_ALT, 0xDB); // AttackRate -
	RegisterHotKey(NULL, 58, MOD_ALT, 0xDD); // AttackRate +

	RegisterHotKey(NULL, 60, MOD_ALT, 0x22); // WvWBonus -
	RegisterHotKey(NULL, 61, MOD_ALT, 0x21); // WvWBonus +

	MSG msg;
	while (GetMessage(&msg, 0, 0, 0))
	{
		PeekMessage(&msg, 0, 0, 0, 0x0001);
		switch (msg.message)
		{
		case WM_HOTKEY:
			// Help
			if (msg.wParam == 0) Help = !Help;
			if (msg.wParam == 1) WorldBossDebug = !WorldBossDebug;

			// DPS Meter
			if (msg.wParam == 10) DpsMeter = !DpsMeter;
			if (msg.wParam == 11) DpsDebug = !DpsDebug;
			if (msg.wParam == 12) AllowNegativeDps = !AllowNegativeDps;
			if (msg.wParam == 13) DpsLock = !DpsLock;
			if (msg.wParam == 14) DpsToFile = !DpsToFile;
			if (msg.wParam == 15) DpsToFile100ms = !DpsToFile100ms;
			if (msg.wParam == 16) DpsToFileIgnoreZero = !DpsToFileIgnoreZero;

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

			if (msg.wParam == 299) AllySupply = !AllySupply;

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
			if (msg.wParam == 53) AllyPlayersVit = !AllyPlayersVit;
			if (msg.wParam == 54) AllyPlayersSelf = !AllyPlayersSelf;
			if (msg.wParam == 55) Speedometer = !Speedometer;
			if (msg.wParam == 56) AttackRate = !AttackRate;
			if (msg.wParam == 57) if (AttackRateMin > 00.1) AttackRateMin -= 0.1;
			if (msg.wParam == 58) if (AttackRateMin < 20.1) AttackRateMin += 0.1;

			if (msg.wParam == 60) if (wvwBonus > 0) wvwBonus -= 1;
			if (msg.wParam == 61) if (wvwBonus < 10) wvwBonus += 1;
		}
	}
}

class TestDll : public GW2LIB::Main
{
public:
	bool init() override
	{
		NewThread(DpsBuffer);
		NewThread(LogDps);
		NewThread(SpeedBuffer);
		NewThread(AttackrateBuffer);
		NewThread(HotKey);
		EnableEsp(cbESP);
		if (!font.Init(16, "Verdana"))
		{
			DbgOut("could not init Font");
			return false;
		}

		return true;
	}
};

TestDll g_testDll;
