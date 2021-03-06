#pragma once

#include <graphics/math.h>

#include "obj_structs.h"
#include "d20_defs.h"
#include "temple_enums.h"

#define macAsmProl {\
	__asm push ecx\
	__asm push esi\
	__asm push ebx\
	__asm push edi};

#define macAsmEpil {\
	__asm pop edi\
	__asm pop ebx\
	__asm pop esi\
	__asm pop ecx}\

#define macRebase(funName , b) rebase(funName, 0x##b); 
#define macReplaceFun(b, funName) replaceFunction(0x##b, funName); 
#define macAsmThis(funName) {\
	__asm mov ecx, this\
	__asm mov esi, [ecx]._##funName }

#define BonusListMax 40


// this is the number of tiles per sector in each direction (so the total is this squared i.e. 4096 in toee)
#define SECTOR_SIDE_SIZE 64

# pragma region Standard Structs

#include <infrastructure/location.h>

#pragma pack(push, 1)

typedef XMFLOAT2 vector2f;
typedef XMFLOAT3 vector3f;


struct Subtile // every tile is subdivided into 3x3 subtiles
{
	int32_t x;
	int32_t y;

	static Subtile fromField(int64_t location) {
		return *(Subtile*)&location;
	}

	int64_t ToField() {
		return *((int64_t*)this);
	}

	operator int64_t() const {
		return *(int64_t*)this;
	}

	operator Subtile() const
	{
		return *(Subtile*)this;
	}
};

struct TileRect
{
	int64_t x1;
	int64_t y1;
	int64_t x2;
	int64_t y2;
};


struct GroupArray {
	objHndl GroupMembers[32];
	uint32_t GroupSize;
	int (__cdecl*sortFunc)(void*, void*); // used for comparing two items (e.g. alphabetic sorting)

	void Clear(); // clears members
	void Reset(); // clears members and sorter func
};

struct JumpPointPacket {
	int jmpPntId;
	char * pMapName;
	_mapNum mapNum;
	uint32_t field_C;
	locXY location;
};



#pragma pack(pop)


struct BonusEntry
{
	int32_t bonValue;
	int32_t bonType; // types 0, 8 and 21 can stack ( 100E6490 ); use negative number for modifier
	const char * bonusMesString; // parsable string for the help system e.g. "~Item~[TAG_ITEM]"
	char * bonusDescr; // e.g. "Magic Full Plate +1"
};

struct BonusCap
{
	int capValue;
	int bonType;
	char *bonCapperString;
	const char * bonCapDescr;
};

struct BonusList
{
	BonusEntry bonusEntries[40];
	uint32_t bonCount;
	BonusCap bonCaps[10];
	uint32_t bonCapperCount;
	uint32_t zeroBonusReasonMesLine[10]; // a line from the bonus.mes that is auto assigned a 0 value (I think it will print ---). Probably for overrides like racial immunity and stuff.
	uint32_t zeroBonusCount;
	BonusEntry overallCapHigh; // init to largest  positive int; controlls what the sum of all the modifiers of various types cannot exceed
	BonusEntry overallCapLow; //init to most negative int
	uint32_t bonFlags; // init 0; 0x1 - overallCapHigh set; 0x2 - overallCapLow set; 0x4 - force cap override (otherwise it can only impose restrictions i.e. it will only change the cap if it's lower than the current one)

	BonusList()	{
		this->bonCount = 0;
		this->bonCapperCount = 0;
		this->zeroBonusCount = 0;
		this->bonFlags = 0;
		this->overallCapHigh.bonValue = 0x7fffFFFF;
		this->overallCapLow.bonValue = 0x80000001;
	}

	int GetEffectiveBonusSum() const;

	/**
	 * Returns true if the given bonus is suppressed by another bonus (i.e. of the same type and
	 * the bonus type does not stack). If true is returned, the idx of the supressing bonus is returned
	 * in suppressedByIdx.
	 */
	bool IsBonusSuppressed(size_t bonusIdx, size_t* suppressedByIdx) const;
	
	/**
	 * Returns true if the given bonus is capped by one of the caps in this bonus list and returns the
	 * index of the lowest cap in cappedByIdx, if the pointer is not null.
	 */
	bool IsBonusCapped(size_t bonusIdx, size_t* cappedByIdx) const;
	/*
		Adds a bonus of a particular type.
		Will register in the D20 roll history using the specified line from bonus.mes
	*/
	int AddBonus(int value, int bonType, int mesline);
	BOOL AddBonus(int value, int bonType, std::string & text);
	int AddBonusWithDesc(int value, int bonType, int mesline, char* descr);
	int AddBonusWithDesc(int value, int bonType, std::string &text, char* descr);
	int AddBonusFromFeat(int value, int bonType, int mesline, feat_enums feat); // same as the above, but it also gets the feat name automatically
	int AddBonusFromFeat(int value, int bonType, int mesline, std::string &feat); 
	int ModifyBonus(int value, int bonType, int meslineIdentifier); // directly modifies a bonus. For non-stacking bonus types that you want to stack anyway. Note: the mesline is used as an identifier here!
	BOOL ZeroBonusSetMeslineNum(int mesline);

	int AddCap(int capType, int capValue, uint32_t bonMesLineNum);
	int AddCapWithDescr(int capType, int capValue, uint32_t bonMesLineNum, char* capDescr);
	int AddCapWithCustomDescr(int capType, int capValue, uint32_t bonMesLineNum, std::string &textArg);

	BOOL SetOverallCap(int BonFlags, int newCap, int newCapType, int newCapMesLineNum, char *capDescr = nullptr);
	static const char* GetBonusMesLine(int lineNum);
};

const int TestSizeOfBonusList = sizeof(BonusList); // should be 888 (0x378)


struct AttackPacket{

	objHndl attacker;
	objHndl victim;
	D20ActionType d20ActnType;
	int dispKey;
	D20CAF flags;
	int field_1C;
	objHndl weaponUsed;
	objHndl ammoItem;
	
	objHndl GetWeaponUsed() const;
	bool IsOffhandAttack();
	AttackPacket();
};

enum ScreenDirections : char {
	Top = 0,
	TopRight = 1,
	Right,
	BottomRight,
	Bottom,
	BottomLeft,
	Left,
	TopLeft,
	DirectionsNum // 8
};


inline std::ostream& operator<<(std::ostream& os, const ScreenDirections & direc) {
	switch(direc)
	{
	case ScreenDirections::Top:
		return os << "U";
	case ScreenDirections::TopRight:
		return os << "UR";
	case ScreenDirections::Right:
		return os << "R";
	case ScreenDirections::BottomRight:
		return os << "DR";
	case ScreenDirections::Bottom:
		return os << "D";
	case ScreenDirections::BottomLeft :
		return os << "DL";
	case ScreenDirections::Left:
		return os << "L";
	case ScreenDirections::TopLeft:
		return os << "UL";
	default:
		return os << "NA";
	}
}


struct PointNode
{
	float absX; // world X goes here
	float absZ; // always 0 in toee
	float absY; // world Y goes here
	PointNode();
	PointNode(float x, float y, float z);
};


/*
note that the first byte denotes the "basic" targeting type
*/
enum class UiPickerType : uint64_t {
	None = 0,
	Single,
	Multi,
	Cone,
	Area,
	Location,
	Personal,
	InventoryItem,
	Ray = 8,
	Wall = 9, // NEW!
	BecomeTouch = 0x100,
	AreaOrObj = 0x200,
	OnceMulti = 0x400,
	Any30Feet = 0x800,
	Primary30Feet = 0x1000,
	EndEarlyMulti = 0x2000,
	LocIsClear = 0x4000,
	PickOrigin = 0x8000 // New! denotes that the spell's point of origin can be freely chosen
};

struct ObjListResultItem {
	objHndl handle;
	ObjListResultItem *next;

	void FreeRecursive(){
		if (next)
			next->FreeRecursive();
		free(this);
	}
	
};

struct ObjListResult
{
	int numSectorObjects;
	int field_4;
	int field_8;
	int field_C;
	int field_10;
	int field_14;
	int field_18;
	int field_1C;
	int field_20;
	int field_24;
	int field_28;
	int field_2C;
	int field_30;
	int field_34;
	int field_38;
	int field_3C;
	int field_40;
	int field_44;
	int field_48;
	int field_4C;
	int field_50;
	int field_54;
	int field_58;
	int field_5C;
	int field_60;
	int field_64;
	int field_68;
	int field_6C;
	int field_70;
	int field_74;
	int field_78;
	int field_7C;
	int field_80;
	int field_84;
	ObjListResultItem *objects;
	int field_8C;
	int field_90;
	int field_94;

	void Init();
	int Free();
	void PrependHandle(objHndl handle);
	void IncreaseObjListCount();
	int CountResults();
	void ListRadius(LocAndOffsets origin, float rangeInches, float angleMin, float angleMax, int filter);
	void ListRaycast(LocAndOffsets &origin, LocAndOffsets &endPt, float rangeInches, float radiusInches);

};

struct PickerResult {
	int flags; // see PickerResultFlags
	int field4;
	objHndl handle;
	ObjListResult objList;
	LocAndOffsets location;
	float offsetz;
	int fieldbc;

	void FreeObjlist();

};


enum EquipSlot : uint32_t {
	Helmet = 0,
	Necklace = 1,
	Gloves = 2,
	WeaponPrimary = 3,
	WeaponSecondary = 4,
	Armor = 5,
	RingPrimary = 6,
	RingSecondary = 7,
	Boots = 8,
	Ammo = 9,
	Cloak = 10,
	Shield = 11,
	Robes = 12,
	Bracers = 13,
	BardicItem = 14,
	Lockpicks = 15,
	Count = 16,
	Invalid = 17
};

//struct ActionCostPacket
//{
//	int hourglassCost;
//	int chargeAfterPicker; // flag I think; is only set at stuff that requires using the picker it seems
//	float moveDistCost;
//
//	ActionCostPacket() { hourglassCost = 0; chargeAfterPicker = 0; moveDistCost = 0.0f; }
//};
////const auto TestSizeOfActionCostPacket = sizeof(ActionCostPacket); // should be 12 (0xC)
#pragma endregion