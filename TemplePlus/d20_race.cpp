#include "stdafx.h"
#include "d20_race.h"
#include "temple_enums.h"
#include "python/python_integration_race.h"

class RaceSpec
{
public:
	std::vector<int> statModifiers;
	int effectiveLevel = 0; // modifier for Effective Character Level (determines XP requirement)
	std::string helpTopic;  // helpsystem id ("TAG_xxx")
	D20RaceSys::RaceDefinitionFlags flags;
	int protoId;            // protos.tab entry (male; female is +1)
	int materialOffset = 0; // offset for rules/materials.mes (or materials_ext.mes for non-vanilla races)

	int weightMale[2];
	int weightFemale[2];
	int heightMale[2];
	int heightFemale[2];


	// Main Race
	RaceBase baseRace = race_base_human;
	bool isBaseRace = true;
	std::vector<Race> subraces;

	// Subrace
	Subrace subrace = subrace_none;

	std::string conditionName;

	RaceSpec(){
		statModifiers = { 0,0,0,0,0,0 };
	}
};

D20RaceSys d20RaceSys;

D20RaceSys::D20RaceSys()
{
	int _charRaceEnums[VANILLA_NUM_RACES] =
	{ Race::race_human, Race::race_dwarf, Race::race_elf,
		Race::race_gnome, Race::race_halfelf, Race::race_half_orc,
		Race::race_halfling	 };
	memcpy(vanillaRaceEnums, _charRaceEnums, VANILLA_NUM_RACES * sizeof(uint32_t));

	for (auto it:vanillaRaceEnums){
		mRaceSpecs[(Race)it] = RaceSpec();
	}

	mRaceSpecs[Race::race_dwarf].statModifiers[Stat::stat_constitution] = 2;
	mRaceSpecs[Race::race_dwarf].statModifiers[Stat::stat_charisma] = -2;

	mRaceSpecs[Race::race_elf].statModifiers[Stat::stat_constitution] = -2;
	mRaceSpecs[Race::race_elf].statModifiers[Stat::stat_dexterity] = 2;

	mRaceSpecs[Race::race_gnome].statModifiers[Stat::stat_constitution] = 2;
	mRaceSpecs[Race::race_gnome].statModifiers[Stat::stat_strength] = -2;

	mRaceSpecs[Race::race_half_orc].statModifiers[Stat::stat_charisma] = -2;
	mRaceSpecs[Race::race_half_orc].statModifiers[Stat::stat_strength] = 2;

	mRaceSpecs[Race::race_halfling].statModifiers[Stat::stat_dexterity] = 2;
	mRaceSpecs[Race::race_halfling].statModifiers[Stat::stat_strength] = -2;

}

void D20RaceSys::GetRaceSpecsFromPython(){
	std::vector<int> _raceEnums;
	pythonRaceIntegration.GetRaceEnums(_raceEnums);
	std::sort(_raceEnums.begin(), _raceEnums.end());

	for (auto it : _raceEnums) {

		//if (!pythonRaceIntegration.IsEnabled(it))
		//	continue;

		raceEnums.push_back(it);
		auto race = (Race)it;
		RaceSpec &raceSpec = mRaceSpecs[race];

		raceSpec.baseRace = GetBaseRace(race);
		raceSpec.subrace = GetSubrace(race);
		raceSpec.isBaseRace = raceSpec.subrace == subrace_none;
		raceSpec.helpTopic = pythonRaceIntegration.GetRaceHelpTopic(it);

		// get class condition from py file
		raceSpec.conditionName = fmt::format("{}", pythonRaceIntegration.GetConditionName(it));
		if (raceSpec.conditionName.size() == 0) { // if none specified, get it from the pre-defined table for base races (applicable for vanilla races only...)
			raceSpec.conditionName = d20StatusSys.raceCondMap[(Race)raceSpec.baseRace];
		}
		else if (d20StatusSys.raceCondMap.find(race) == d20StatusSys.raceCondMap.end()) { // if condition name is specified, and was not found in the d20Status mapping , update it
			d20StatusSys.raceCondMap[race] = raceSpec.conditionName;
		}

		raceSpec.flags = pythonRaceIntegration.GetRaceDefinitionFlags(it);
		raceSpec.statModifiers = pythonRaceIntegration.GetStatModifiers(race);
		raceSpec.effectiveLevel = pythonRaceIntegration.GetLevelModifier(it);
		raceSpec.protoId = pythonRaceIntegration.GetProtoId(race);
		raceSpec.materialOffset = pythonRaceIntegration.GetMaterialOffset(race);
		auto minMaxHeightWeight = pythonRaceIntegration.GetMinMaxHeightWeight(race);
		if (minMaxHeightWeight.size() >= 8) {
			raceSpec.heightMale[0] = minMaxHeightWeight[0];
			raceSpec.heightMale[1] = minMaxHeightWeight[1];
			raceSpec.heightFemale[0] = minMaxHeightWeight[2];
			raceSpec.heightFemale[1] = minMaxHeightWeight[3];

			raceSpec.weightMale[0] = minMaxHeightWeight[4];
			raceSpec.weightMale[1] = minMaxHeightWeight[5];
			raceSpec.weightFemale[0] = minMaxHeightWeight[6];
			raceSpec.weightFemale[1] = minMaxHeightWeight[7];
		}

		
		if (raceSpec.isBaseRace){
			baseRaceEnums.push_back(it);
		}
			

		
		

		// skills
		/*for (auto skillEnum = static_cast<int>(skill_appraise); skillEnum < skill_count; skillEnum++) {
			raceSpec.classSkills[(SkillEnum)skillEnum] = pythonClassIntegration.IsClassSkill(it, (SkillEnum)skillEnum);
		}*/

	}

	baseRaceEnums.insert(baseRaceEnums.end() -1, race_dwarf);
	baseRaceEnums.insert(baseRaceEnums.end() - 1, race_elf);
	baseRaceEnums.insert(baseRaceEnums.end() - 1, race_gnome);
	baseRaceEnums.insert(baseRaceEnums.end() - 1, race_half_elf);
	baseRaceEnums.insert(baseRaceEnums.end() - 1, race_half_orc);
	baseRaceEnums.insert(baseRaceEnums.end() - 1, race_halfling);

	// Compile subrace lists
	for (auto it : mRaceSpecs){
		auto &entry = it.second;
		if (entry.isBaseRace) continue;

		mRaceSpecs[(Race)entry.baseRace].subraces.push_back(it.first);
	}
}

int D20RaceSys::GetStatModifier(Race race, int stat) {
	auto &spec = GetRaceSpec(race);
	if (stat <= stat_charisma)
		return spec.statModifiers[stat];
	return 0;
}

HairStyleRace D20RaceSys::GetHairStyle(Race race){

	switch (race){
	case race_human:
		return HairStyleRace::Human;
	case race_dwarf:
		return HairStyleRace::Dwarf;
	case race_elf:
		return HairStyleRace::Elf;
	case race_gnome:
		return HairStyleRace::Gnome;
	case race_half_elf:
		return HairStyleRace::HalfElf;
	case race_half_orc:
		return HairStyleRace::HalfOrc;
	case race_halfling:
		return HairStyleRace::Halfling;
	default:
		return HairStyleRace::Human;
	}
}

RaceBase D20RaceSys::GetBaseRace(Race race){
	return (RaceBase)((int)race & 0x1F);
}

Subrace D20RaceSys::GetSubrace(Race race) {
	return (Subrace)((int)race >> 5);
}

int D20RaceSys::GetProtoId(Race race){

	if (race < VANILLA_NUM_RACES){
		return 13000 + race * 2;
	}
	
	auto &raceSpec = GetRaceSpec(race);
	return raceSpec.protoId;
}

Race D20RaceSys::GetRaceEnum(const std::string & raceName)
{
	for (auto it : mRaceSpecs){
		if (it.second.conditionName == raceName){
			return it.first;
		}
	}
	auto result= temple::GetRef<Race(__cdecl)(const char*)>(0x10073B70)(raceName.c_str());
	return result;
}

int D20RaceSys::GetMinHeight(Race race, Gender genderId)
{
	if (race <= VANILLA_NUM_RACES)
		return temple::GetRef<int[]>(0x102EFCE0)[2*(int)race + (int)genderId];
	auto &raceSpecs = GetRaceSpec(race);
	if (genderId == Gender::Male)
		return raceSpecs.heightMale[0];
	else
		return raceSpecs.heightFemale[0];
}

int D20RaceSys::GetMaxHeight(Race race, Gender genderId)
{
	if (race <= VANILLA_NUM_RACES)
		return temple::GetRef<int[]>(0x102EFD18)[2 * (int)race + (int)genderId];
	auto &raceSpecs = GetRaceSpec(race);
	if (genderId == Gender::Male)
		return raceSpecs.heightMale[1];
	else
		return raceSpecs.heightFemale[1];
}

int D20RaceSys::GetMinWeight(Race race, Gender genderId)
{
	if (race <= VANILLA_NUM_RACES)
		return temple::GetRef<int[]>(0x102EFD50)[2 * (int)race + (int)genderId];
	auto &raceSpecs = GetRaceSpec(race);
	if (genderId == Gender::Male)
		return raceSpecs.weightMale[0];
	else
		return raceSpecs.weightFemale[0];
}

int D20RaceSys::GetMaxWeight(Race race, Gender genderId)
{
	if (race <= VANILLA_NUM_RACES)
		return temple::GetRef<int[]>(0x102EFD88)[2 * (int)race + (int)genderId];
	auto &raceSpecs = GetRaceSpec(race);
	if (genderId == Gender::Male)
		return raceSpecs.weightMale[1];
	else
		return raceSpecs.weightFemale[1];
}

bool D20RaceSys::HasSubrace(Race race){
	auto &raceSpec = GetRaceSpec(race);
	if (!raceSpec.isBaseRace){
		return false;
	}
		
	return raceSpec.subraces.size() > 0;
}

const std::vector<Race>& D20RaceSys::GetSubraces(RaceBase raceBase)
{
	auto race = (Race)raceBase;
	auto &raceSpec = GetRaceSpec(race);

	return raceSpec.subraces;
}

float D20RaceSys::GetModelScale(Race race, int genderId){
	if (race < VANILLA_NUM_RACES){
		return temple::GetRef<float[14]>(0x102FE188)[race * 2 + genderId];
	}

	auto raceBase = GetBaseRace(race);
	if (raceBase < VANILLA_NUM_RACES && raceBase >= 0)
		return GetModelScale((Race)raceBase, 0);

	return 1.0f;
}

int D20RaceSys::GetRaceMaterialOffset(Race race){
	if (race < VANILLA_NUM_RACES)
		return (race * 2);
	auto &raceSpec = GetRaceSpec(race);
	return raceSpec.materialOffset;
}

std::string D20RaceSys::GetRaceCondition(Race race){
	static std::vector<std::string> raceCondNames = { "Human", "Dwarf", "Elf", "Gnome", "Halfelf", "Halforc", "Halfling" };
	if (race < VANILLA_NUM_RACES){
		return raceCondNames[race];
	}

	auto &raceSpec = GetRaceSpec(race);
	return raceSpec.conditionName;
}

int D20RaceSys::GetLevelAdjustment(objHndl & objHnd)
{
	auto race = critterSys.GetRace(objHnd, false);
	auto &raceSpec = GetRaceSpec(race);
	auto lvlAdj = raceSpec.effectiveLevel;
	return lvlAdj;
}

RaceSpec& D20RaceSys::GetRaceSpec(Race race){
	if (mRaceSpecs.find(race) != mRaceSpecs.end()) {
		return mRaceSpecs[race];
	}
	logger->error(fmt::format("Bad race input {}, returning human instead", (int)race).c_str());
	return mRaceSpecs[Race::race_human];
}