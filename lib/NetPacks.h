#pragma once

#include <boost/variant.hpp>

#include "BattleAction.h"
#include "HeroBonus.h"
#include "CCreatureSet.h"
#include "CMapInfo.h"
#include "StartInfo.h"
#include "ConstTransitivePtr.h"
#include "int3.h"
#include "ResourceSet.h"

/*
 * NetPacks.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CClient;
class CGameState;
class CGameHandler;
class CConnection;
class CCampaignState;
class CArtifact;
class CSelectionScreen;
class CGObjectInstance;
class CArtifactInstance;
//class CMapInfo;
struct ArtSlotInfo;

struct CPack
{
	ui16 type;

	CPack(){};
	virtual ~CPack(){};
	ui16 getType() const{return type;}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		tlog1 << "CPack serialized... this should not happen!\n";
	}
	DLL_LINKAGE void applyGs(CGameState *gs)
	{};
};

struct CPackForClient : public CPack
{
	CPackForClient(){type = 1;};

	CGameState* GS(CClient *cl);
	void applyFirstCl(CClient *cl)//called before applying to gs
	{};
	void applyCl(CClient *cl)//called after applying to gs
	{}; 
};

struct CPackForServer : public CPack
{
	ui8 player;
	CConnection *c;
	CGameState* GS(CGameHandler *gh);
	CPackForServer()
	{
		type = 2; 
		c = NULL;
		player = 255;
	};

	bool applyGh(CGameHandler *gh);//called after applying to gs
};


struct Query : public CPackForClient
{
	ui32 id;
};


struct MetaString : public CPack //2001 helper for object scrips
{
private:
	enum EMessage {TEXACT_STRING, TLOCAL_STRING, TNUMBER, TREPLACE_ESTRING, TREPLACE_LSTRING, TREPLACE_NUMBER, TREPLACE_PLUSNUMBER};
public:
	enum {GENERAL_TXT=1, XTRAINFO_TXT, OBJ_NAMES, RES_NAMES, ART_NAMES, ARRAY_TXT, CRE_PL_NAMES, CREGENS, MINE_NAMES, 
		MINE_EVNTS, ADVOB_TXT, ART_EVNTS, SPELL_NAME, SEC_SKILL_NAME, CRE_SING_NAMES, CREGENS4, COLOR, ART_DESCR};

	std::vector<ui8> message; //vector of EMessage

	std::vector<std::pair<ui8,ui32> > localStrings; //pairs<text handler type, text number>; types: 1 - generaltexthandler->all; 2 - objh->xtrainfo; 3 - objh->names; 4 - objh->restypes; 5 - arth->artifacts[id].name; 6 - generaltexth->arraytxt; 7 - creh->creatures[os->subID].namePl; 8 - objh->creGens; 9 - objh->mines[ID]->first; 10 - objh->mines[ID]->second; 11 - objh->advobtxt
	std::vector<std::string> exactStrings;
	std::vector<si32> numbers;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & exactStrings & localStrings & message & numbers;
	}
	void addTxt(ui8 type, ui32 serial)
	{
		message.push_back(TLOCAL_STRING);
		localStrings.push_back(std::pair<ui8,ui32>(type, serial));
	}
	MetaString& operator<<(const std::pair<ui8,ui32> &txt)
	{
		message.push_back(TLOCAL_STRING);
		localStrings.push_back(txt);
		return *this;
	}
	MetaString& operator<<(const std::string &txt)
	{
		message.push_back(TEXACT_STRING);
		exactStrings.push_back(txt);
		return *this;
	}
	MetaString& operator<<(int txt)
	{
		message.push_back(TNUMBER);
		numbers.push_back(txt);
		return *this;
	}
	void addReplacement(ui8 type, ui32 serial)
	{
		message.push_back(TREPLACE_LSTRING);
		localStrings.push_back(std::pair<ui8,ui32>(type, serial));
	}
	void addReplacement(const std::string &txt)
	{
		message.push_back(TREPLACE_ESTRING);
		exactStrings.push_back(txt);
	}
	void addReplacement(int txt)
	{
		message.push_back(TREPLACE_NUMBER);
		numbers.push_back(txt);
	}
	void addReplacement2(int txt)
	{
		message.push_back(TREPLACE_PLUSNUMBER);
		numbers.push_back(txt);
	}
	DLL_LINKAGE void addCreReplacement(TCreature id, TQuantity count); //adds sing or plural name;
	DLL_LINKAGE void addReplacement(const CStackBasicDescriptor &stack); //adds sing or plural name;
	DLL_LINKAGE std::string buildList () const;
	void clear()
	{
		exactStrings.clear();
		localStrings.clear();
		message.clear();
		numbers.clear();
	}
	DLL_LINKAGE void toString(std::string &dst) const;
	DLL_LINKAGE std::string toString() const;
	void getLocalString(const std::pair<ui8,ui32> &txt, std::string &dst) const;

	MetaString()
	{
		type = 2001;
	}
}; 

/***********************************************************************************************************/


struct PackageApplied : public CPackForClient //94
{
	PackageApplied() {type = 94;}
	PackageApplied(ui8 Result) : result(Result) {type = 94;}
	void applyCl(CClient *cl);

	ui8 result; //0 - something went wrong, request hasn't been realized; 1 - OK
	ui32 packType; //type id of applied package
	ui32 requestID; //an ID given by client to the request that was applied
	ui8 player;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & result & packType & requestID & player;
	}
};

struct SystemMessage : public CPackForClient //95
{
	SystemMessage(const std::string Text) : text(Text){type = 95;};
	SystemMessage(){type = 95;};
	void applyCl(CClient *cl);

	std::string text;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & text;
	}
};

struct PlayerBlocked : public CPackForClient //96
{
	PlayerBlocked(){type = 96;};
	void applyCl(CClient *cl);

	enum EReason { UPCOMING_BATTLE };

	ui8 reason;
	ui8 player;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & reason & player;
	}
};

struct YourTurn : public CPackForClient //100
{
	YourTurn(){type = 100;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui8 player;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player;
	}
};

struct SetResource : public CPackForClient //102
{
	SetResource(){type = 102;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui8 player, resid;
	TResourceCap val;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player & resid & val;
	}
}; 
 struct SetResources : public CPackForClient //104
 {
 	SetResources(){type = 104;};
 	void applyCl(CClient *cl);
 	DLL_LINKAGE void applyGs(CGameState *gs);
 
 	ui8 player;
 	TResources res; //res[resid] => res amount
 
 	template <typename Handler> void serialize(Handler &h, const int version)
 	{
 		h & player & res;
 	}
 };

struct SetPrimSkill : public CPackForClient //105
{
	SetPrimSkill(){type = 105;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui8 abs; //0 - changes by value; 1 - sets to value
	si32 id;
	ui16 which;
	si64 val;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & abs & id & which & val;
	}
}; 
struct SetSecSkill : public CPackForClient //106
{
	SetSecSkill(){type = 106;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui8 abs; //0 - changes by value; 1 - sets to value
	si32 id;
	ui16 which, val;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & abs & id & which & val;
	}
}; 
struct HeroVisitCastle : public CPackForClient //108
{
	HeroVisitCastle(){flags=0;type = 108;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui8 flags; //1 - start
	ui32 tid, hid;

	bool start() //if hero is entering castle (if false - leaving)
	{
		return flags & 1;
	}
// 	bool garrison() //if hero is entering/leaving garrison (if false - it's only visiting hero)
// 	{
// 		return flags & 2;
// 	}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & flags & tid & hid;
	}
}; 
struct ChangeSpells : public CPackForClient //109
{
	ChangeSpells(){type = 109;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui8 learn; //1 - gives spell, 0 - takes
	ui32 hid;
	std::set<ui32> spells;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & learn & hid & spells;
	}
}; 

struct SetMana : public CPackForClient //110
{
	SetMana(){type = 110;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);


	si32 hid, val;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & val & hid;
	}
};

struct SetMovePoints : public CPackForClient //111
{
	SetMovePoints(){type = 111;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui32 hid, val;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & val & hid;
	}
};

struct FoWChange : public CPackForClient //112
{
	FoWChange(){type = 112;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	boost::unordered_set<int3, struct ShashInt3 > tiles;
	ui8 player, mode; //mode==0 - hide, mode==1 - reveal
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tiles & player & mode;
	}
}; 

struct SetAvailableHeroes : public CPackForClient //113
{
	SetAvailableHeroes()
	{
		type = 113;
		for (int i = 0; i < GameConstants::AVAILABLE_HEROES_PER_PLAYER; i++)
			army[i].clear();
	}
	~SetAvailableHeroes()
	{
	}
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui8 player;
	si32 hid[GameConstants::AVAILABLE_HEROES_PER_PLAYER]; //-1 if no hero
	CSimpleArmy army[GameConstants::AVAILABLE_HEROES_PER_PLAYER];
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player & hid & army;
	}
};

struct GiveBonus :  public CPackForClient //115
{
	GiveBonus(ui8 Who = 0)
	{
		who = Who; 
		type = 115;
	}

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	enum {HERO, PLAYER, TOWN};
	ui8 who; //who receives bonus, uses enum above
	ui32 id; //hero. town or player id - whoever receives it
	Bonus bonus;
	MetaString bdescr;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & bonus & id & bdescr & who;
	}
};

struct ChangeObjPos : public CPackForClient //116
{
	ChangeObjPos()
	{
		type = 116; 
		flags = 0;
	}
	void applyFirstCl(CClient *cl);
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui32 objid;
	int3 nPos;
	ui8 flags; //bit flags: 1 - redraw

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & objid & nPos & flags;
	}
};

struct PlayerEndsGame : public CPackForClient //117
{
	PlayerEndsGame()
	{
		type = 117;
	}

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui8 player;
	ui8 victory;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player & victory;
	}
};


struct RemoveBonus :  public CPackForClient //118
{
	RemoveBonus(ui8 Who = 0)
	{
		who = Who; 
		type = 118;
	}

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	enum {HERO, PLAYER, TOWN};
	ui8 who; //who receives bonus, uses enum above
	ui32 whoID; //hero, town or player id - whoever loses bonus

	//vars to identify bonus: its source
	ui8 source;
	ui32 id; //source id

	//used locally: copy of removed bonus
	Bonus bonus;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & source & id & who & whoID;
	}
};

struct UpdateCampaignState : public CPackForClient //119
{
	UpdateCampaignState()
	{
		type = 119;
	}

	CCampaignState *camp;
	void applyCl(CClient *cl);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & camp;
	}
};

struct RemoveObject : public CPackForClient //500
{
	RemoveObject(){type = 500;};
	RemoveObject(si32 ID){id = ID;type = 500;};
	void applyFirstCl(CClient *cl);
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	si32 id;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id;
	}
}; 
struct TryMoveHero : public CPackForClient //501
{
	TryMoveHero(){type = 501;humanKnows=false; attackedFrom = int3(-1, -1, -1);};
	void applyFirstCl(CClient *cl);
	void applyCl(CClient *cl);
	void applyGs(CGameState *gs);

	enum EResult
	{
		FAILED, SUCCESS, TELEPORTATION, RESERVED___, BLOCKING_VISIT, EMBARK, DISEMBARK
	};

	ui32 id, movePoints;
	ui8 result; //uses EResult
	int3 start, end; //h3m format
	boost::unordered_set<int3, ShashInt3> fowRevealed; //revealed tiles
	int3 attackedFrom; // Set when stepping into endangered tile.

	bool humanKnows; //used locally during applying to client

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & result & start & end & movePoints & fowRevealed & attackedFrom;
	}
};

// struct SetGarrisons : public CPackForClient //502
// {
// 	SetGarrisons(){type = 502;};
// 	void applyCl(CClient *cl);
// 	DLL_LINKAGE void applyGs(CGameState *gs);
// 
// 	std::map<ui32,CCreatureSet> garrs;
// 
// 	template <typename Handler> void serialize(Handler &h, const int version)
// 	{
// 		h & garrs;
// 	}
// }; 

struct NewStructures : public CPackForClient //504
{
	NewStructures(){type = 504;};
	void applyCl(CClient *cl);
	DLL_LINKAGE virtual void applyGs(CGameState *gs);

	si32 tid;
	std::set<si32> bid;
	si16 builded; 

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid & bid & builded;
	}
};
struct RazeStructures : public CPackForClient //505
{
	RazeStructures() {type = 505;};
	void applyCl (CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	si32 tid;
	std::set<si32> bid;
	si16 destroyed; 

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid & bid & destroyed;
	}
};
struct SetAvailableCreatures : public CPackForClient //506
{
	SetAvailableCreatures(){type = 506;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	si32 tid;
	std::vector<std::pair<ui32, std::vector<ui32> > > creatures;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid & creatures;
	}
};  
struct SetHeroesInTown : public CPackForClient //508
{
	SetHeroesInTown(){type = 508;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	si32 tid, visiting, garrison; //id of town, visiting hero, hero in garrison

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid & visiting & garrison;
	}
};

// struct SetHeroArtifacts : public CPackForClient //509
// {
// 	SetHeroArtifacts(){type = 509;};
// 	void applyCl(CClient *cl);
// 	DLL_LINKAGE void applyGs(CGameState *gs);
// 	DLL_LINKAGE void setArtAtPos(ui16 pos, const CArtifact* art);
// 
// 	si32 hid;
// 	std::vector<const CArtifact*> artifacts; //hero's artifacts from bag
// 	std::map<ui16, const CArtifact*> artifWorn; //map<position,artifact_id>; positions: 0 - head; 1 - shoulders; 2 - neck; 3 - right hand; 4 - left hand; 5 - torso; 6 - right ring; 7 - left ring; 8 - feet; 9 - misc1; 10 - misc2; 11 - misc3; 12 - misc4; 13 - mach1; 14 - mach2; 15 - mach3; 16 - mach4; 17 - spellbook; 18 - misc5
// 
// 	template <typename Handler> void serialize(Handler &h, const int version)
// 	{
// 		h & hid & artifacts & artifWorn;
// 	}
// 
// 	std::vector<const CArtifact*> equipped, unequipped; //used locally
// 	BonusList gained, lost; //used locally as hlp when applying
// };   

struct HeroRecruited : public CPackForClient //515
{
	HeroRecruited(){type = 515;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	si32 hid, tid; //subID of hero
	int3 tile;
	ui8 player;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hid & tid & tile & player;
	}
};  

struct GiveHero : public CPackForClient //516
{
	GiveHero(){type = 516;};
	void applyFirstCl(CClient *cl);
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui32 id; //object id
	ui8 player;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & player;
	}
};  

struct OpenWindow : public CPackForClient //517
{
	OpenWindow(){type = 517;};
	void applyCl(CClient *cl);

	enum EWindow {EXCHANGE_WINDOW, RECRUITMENT_FIRST, RECRUITMENT_ALL, SHIPYARD_WINDOW, THIEVES_GUILD,
	              UNIVERSITY_WINDOW, HILL_FORT_WINDOW, MARKET_WINDOW, PUZZLE_MAP, TAVERN_WINDOW};
	ui8 window;
	ui32 id1, id2;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & window & id1 & id2;
	}
};

struct NewObject  : public CPackForClient //518
{
	NewObject()
	{
		type = 518;
		id = -1;
	}
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui32 ID, subID;
	int3 pos;

	int id; //used locally, filled during applyGs

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID & subID & pos;
	}
};

struct SetAvailableArtifacts  : public CPackForClient //519
{
	SetAvailableArtifacts(){type = 519;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	si32 id; //two variants: id < 0: set artifact pool for Artifact Merchants in towns; id >= 0: set pool for adv. map Black Market (id is the id of Black Market instance then)
	std::vector<const CArtifact *> arts;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & arts;
	}
};

struct NewArtifact : public CPackForClient //520
{
	NewArtifact(){type = 520;};
	//void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ConstTransitivePtr<CArtifactInstance> art;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & art;
	}
};

struct StackLocation
{
	ConstTransitivePtr<CArmedInstance> army;
	TSlot slot;

	StackLocation()
	{
		slot = -1;
	}
	StackLocation(const CArmedInstance *Army, TSlot Slot)
	{
		army = const_cast<CArmedInstance*>(Army); //we are allowed here to const cast -> change will go through one of our packages... do not abuse!
		slot = Slot;
	}
	DLL_LINKAGE const CStackInstance *getStack();
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & army & slot;
	}
};

struct CGarrisonOperationPack : CPackForClient
{
};
struct CArtifactOperationPack : CPackForClient
{
};


struct ChangeStackCount : CGarrisonOperationPack  //521
{
	StackLocation sl;
	TQuantity count;
	ui8 absoluteValue; //if not -> count will be added (or subtracted if negative)

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & sl & count & absoluteValue;
	}
};

struct SetStackType : CGarrisonOperationPack  //522
{
	StackLocation sl;
	CCreature *type;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & sl & type;
	}
};

struct EraseStack : CGarrisonOperationPack  //523
{
	StackLocation sl;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & sl;
	}
};

struct SwapStacks : CGarrisonOperationPack  //524
{
	StackLocation sl1, sl2;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & sl1 & sl2;
	}
};

struct InsertNewStack : CGarrisonOperationPack //525
{
	StackLocation sl;
	CStackBasicDescriptor stack;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & sl & stack;
	}
};

//moves creatures from src stack to dst slot, may be used for merging/splittint/moving stacks
struct RebalanceStacks : CGarrisonOperationPack  //526
{
	StackLocation src, dst;
	TQuantity count;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & src & dst & count;
	}
};

typedef si32 TArtPos;

struct ArtifactLocation
{
	typedef boost::variant<ConstTransitivePtr<CGHeroInstance>, ConstTransitivePtr<CStackInstance> > TArtHolder;

	TArtHolder artHolder;
	TArtPos slot;

	ArtifactLocation()
	{
		artHolder = ConstTransitivePtr<CGHeroInstance>();
		slot = -1;
	}
	template <typename T>
	ArtifactLocation(const T *ArtHolder, TArtPos Slot)
	{

		artHolder = const_cast<T*>(ArtHolder); //we are allowed here to const cast -> change will go through one of our packages... do not abuse!
		slot = Slot;
	}
	ArtifactLocation(TArtHolder ArtHolder, TArtPos Slot)
	{
		artHolder = ArtHolder;
		slot = Slot;
	}

	template <typename T>
	bool isHolder(const T *t) const
	{
		if(auto ptrToT = boost::get<ConstTransitivePtr<T> >(&artHolder))
		{
			return ptrToT->get() == t;
		}
		return false;
	}

	DLL_LINKAGE const CArmedInstance *relatedObj() const; //hero or the stack owner
	DLL_LINKAGE int owningPlayer() const;
	DLL_LINKAGE CArtifactSet *getHolderArtSet();
	DLL_LINKAGE CBonusSystemNode *getHolderNode();
	DLL_LINKAGE const CArtifactSet *getHolderArtSet() const;
	DLL_LINKAGE const CBonusSystemNode *getHolderNode() const;

	DLL_LINKAGE const CArtifactInstance *getArt() const;
	DLL_LINKAGE CArtifactInstance *getArt();
	DLL_LINKAGE const ArtSlotInfo *getSlot() const;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & artHolder & slot;
	}
};

struct PutArtifact : CArtifactOperationPack  //526
{
	ArtifactLocation al;
	ConstTransitivePtr<CArtifactInstance> art;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & al & art;
	}
};

struct EraseArtifact : CArtifactOperationPack  //527
{
	ArtifactLocation al;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & al;
	}
};

struct MoveArtifact : CArtifactOperationPack  //528
{
	ArtifactLocation src, dst;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & src & dst;
	}
};

struct AssembledArtifact : CArtifactOperationPack  //529
{
	ArtifactLocation al; //where assembly will be put
	CArtifact *builtArt;
	//std::vector<CArtifactInstance *> constituents;


	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & al & builtArt/* & constituents*/;
	}
};

struct DisassembledArtifact : CArtifactOperationPack  //530
{
	ArtifactLocation al;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & al;
	}
};

struct HeroVisit : CPackForClient //531
{
	const CGHeroInstance *hero;
	const CGObjectInstance *obj;
	bool starting; //false -> ending

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hero & obj & starting;
	}
};

struct NewTurn : public CPackForClient //101
{
	enum weekType {NORMAL, DOUBLE_GROWTH, BONUS_GROWTH, DEITYOFFIRE, PLAGUE, NO_ACTION};

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	struct Hero
	{
		ui32 id, move, mana; //id is a general serial id
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & id & move & mana;
		}
		bool operator<(const Hero&h)const{return id < h.id;}
	};

	std::set<Hero> heroes; //updates movement and mana points
	//std::vector<SetResources> res;//resource list
	std::map<ui8, TResources> res; //player ID => resource value[res_id]
	std::vector<SetAvailableCreatures> cres;//creatures to be placed in towns
	ui32 day;
	bool resetBuilded;
	ui8 specialWeek; //weekType
	TCreature creatureid; //for creature weeks

	NewTurn(){type = 101;};

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroes & cres & res & day & resetBuilded & specialWeek & creatureid;
	}
}; 

struct Component : public CPack //2002 helper for object scrips informations
{
	enum EComponentType {PRIM_SKILL, SEC_SKILL, RESOURCE, CREATURE, ARTIFACT, EXPERIENCE, SPELL, MORALE=8, LUCK, BUILDING, HERO, FLAG};
	ui16 id, subtype; //id uses ^^^ enums, when id==EXPPERIENCE subtype==0 means exp points and subtype==1 levels)
	si32 val; // + give; - take
	si16 when; // 0 - now; +x - within x days; -x - per x days

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & subtype & val & when;
	}
	Component()
	{
		type = 2002;
	}
	DLL_LINKAGE explicit Component(const CStackBasicDescriptor &stack);
	Component(ui16 Type, ui16 Subtype, si32 Val, si16 When)
		:id(Type),subtype(Subtype),val(Val),when(When)
	{
		type = 2002;
	}
};

struct InfoWindow : public CPackForClient //103  - displays simple info window
{
	void applyCl(CClient *cl);

	MetaString text;
	std::vector<Component> components;
	ui8 player;
	ui16 soundID;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & text & components & player & soundID;
	}
	InfoWindow() 
	{ 
		type = 103;
		soundID = 0; 
	}
};

namespace ObjProperty
{
	//TODO: move non general properties out to the appropriate objs classes
	enum {OWNER = 1, BLOCKVIS = 2, PRIMARY_STACK_COUNT = 3, VISITORS = 4, VISITED = 5, ID = 6, AVAILABLE_CREATURE = 7, SUBID = 8,
		MONSTER_COUNT = 10, MONSTER_POWER = 11, MONSTER_EXP = 12, MONSTER_RESTORE_TYPE = 13};
}

struct SetObjectProperty : public CPackForClient//1001
{
	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	ui32 id;
	ui8 what; //1 - owner; 2 - blockvis; 3 - first stack count; 4 - visitors; 5 - visited; 6 - ID (if 34 then also def is replaced)
	ui32 val;
	SetObjectProperty(){type = 1001;};
	SetObjectProperty(ui32 ID, ui8 What, ui32 Val):id(ID),what(What),val(Val){type = 1001;};
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & what & val;
	}
};

struct SetHoverName : public CPackForClient//1002
{
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui32 id;
	MetaString name;
	SetHoverName(){type = 1002;};
	SetHoverName(ui32 ID, MetaString& Name):id(ID),name(Name){type = 1002;};
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & name;
	}
};
struct HeroLevelUp : public Query//2000
{
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	si32 heroid;
	ui8 primskill, level;
	std::vector<ui16> skills;

	HeroLevelUp(){type = 2000;};
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & heroid & primskill & level & skills;
	}
};

struct TradeComponents : public CPackForClient, public CPackForServer
{
///used to handle info about components available in shops
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	si32 heroid;
	ui32 objectid;
	std::map<ui16, Component> available, chosen, bought;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroid & objectid & available & chosen & bought;
	}

};
//A dialog that requires making decision by player - it may contain components to choose between or has yes/no options
//Client responds with QueryReply, where answer: 0 - cancel pressed, choice doesn't matter; 1/2/...  - first/second/... component selected and OK pressed
//Until sending reply player won't be allowed to take any actions
struct BlockingDialog : public Query//2003
{
	enum {ALLOW_CANCEL = 1, SELECTION = 2};

	void applyCl(CClient *cl);

	MetaString text;
	std::vector<Component> components;
	ui8 player;
	ui8 flags;
	ui16 soundID;

	bool cancel() const
	{
		return flags & ALLOW_CANCEL;
	}
	bool selection() const
	{
		return flags & SELECTION;
	}

	BlockingDialog(bool yesno, bool Selection)
	{
		type = 2003;
		flags = 0;
		soundID = 0;
		if(yesno) flags |= ALLOW_CANCEL;
		if(Selection) flags |= SELECTION;
	}
	BlockingDialog()
	{
		type = 2003;
		flags = 0;
		soundID = 0;
	};

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & text & components & player & flags & soundID;
	}
};

struct GarrisonDialog : public Query//2004
{
	GarrisonDialog(){type = 2004;}
	void applyCl(CClient *cl);
	si32 objid, hid;
	bool removableUnits;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & objid & hid & removableUnits;
	}
};

struct BattleInfo;
struct BattleStart : public CPackForClient//3000
{
	BattleStart(){type = 3000;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	BattleInfo * info;

	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & info;
	}
};
struct BattleNextRound : public CPackForClient//3001
{	
	BattleNextRound(){type = 3001;};
	void applyFirstCl(CClient *cl);
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs( CGameState *gs );
	si32 round;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & round;
	}
};
struct BattleSetActiveStack : public CPackForClient//3002
{
	BattleSetActiveStack(){type = 3002;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui32 stack;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stack;
	}
};
struct BattleResult : public CPackForClient//3003
{
	BattleResult(){type = 3003;};
	void applyFirstCl(CClient *cl);
	void applyGs(CGameState *gs);

	ui8 result; //0 - normal victory; 1 - escape; 2 - surrender
	ui8 winner; //0 - attacker, 1 - defender, [2 - draw (should be possible?)]
	std::map<ui32,si32> casualties[2]; //first => casualties of attackers - map crid => number
	expType exp[2]; //exp for attacker and defender
	std::set<ui32> artifacts; //artifacts taken from loser to winner

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & result & winner & casualties[0] & casualties[1] & exp & artifacts;
	}
};

struct BattleStackMoved : public CPackForClient//3004
{
	ui32 stack;
	std::vector<BattleHex> tilesToMove;
	ui8 distance, teleporting;
	BattleStackMoved(){type = 3004;};
	void applyFirstCl(CClient *cl);
	void applyGs(CGameState *gs);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stack & tilesToMove & distance;
	}
};

struct StacksHealedOrResurrected : public CPackForClient //3013
{
	StacksHealedOrResurrected(){type = 3013;}

	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	struct HealInfo
	{
		ui32 stackID;
		ui32 healedHP;
		ui8 lowLevelResurrection; //in case this stack is resurrected by this heal, it will be marked as SUMMONED //TODO: replace with separate counter
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & stackID & healedHP & lowLevelResurrection;
		}
	};

	std::vector<HealInfo> healedStacks;
	ui8 lifeDrain; //if true, this heal is an effect of life drain
	ui8 tentHealing; //if true, than it's healing via First Aid Tent
	si32 drainedFrom; //if life drain - then stack life was drain from, if tentHealing - stack that is a healer

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & healedStacks & lifeDrain & tentHealing & drainedFrom;
	}
};

struct BattleStackAttacked : public CPackForClient//3005
{
	BattleStackAttacked(){flags = 0; type = 3005;};
	void applyFirstCl(CClient * cl);
	//void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui32 stackAttacked, attackerID;
	ui32 newAmount, newHP, killedAmount, damageAmount;
	enum EFlags {KILLED = 1, EFFECT = 2, SECONDARY = 4, REBIRTH = 8, CLONE_KILLED = 16};
	ui8 flags; //uses EFlags (above)
	ui32 effect; //set only if flag EFFECT is set
	std::vector<StacksHealedOrResurrected> healedStacks; //used when life drain


	bool killed() const//if target stack was killed
	{
		return flags & KILLED || flags & CLONE_KILLED;
	}
	bool cloneKilled() const
	{
		return flags & CLONE_KILLED;
	}
	bool isEffect() const//if stack has been attacked by a spell
	{
		return flags & EFFECT;
	}
	bool isSecondary() const//if stack was not a primary target (receives no spell effects)
	{
		return flags & SECONDARY;
	}
	bool willRebirth() const//if stack was not a primary target (receives no spell effects)
	{
		return flags & REBIRTH;
	}
	bool lifeDrain() const //if this attack involves life drain effect
	{
		return healedStacks.size() > 0;
	}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stackAttacked & attackerID & newAmount & newHP & flags & killedAmount & damageAmount & effect
			& healedStacks;
	}
	bool operator<(const BattleStackAttacked &b) const
	{
		return stackAttacked < b.stackAttacked;
	}
};

struct BattleAttack : public CPackForClient//3006
{
	BattleAttack(){flags = 0; type = 3006;};
	void applyFirstCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	std::vector<BattleStackAttacked> bsa;
	ui32 stackAttacking;
	ui8 flags; //uses Eflags (below)
	enum EFlags{SHOT = 1, COUNTER = 2, LUCKY = 4, UNLUCKY = 8, BALLISTA_DOUBLE_DMG = 16, DEATH_BLOW = 32};

	bool shot() const//distance attack - decrease number of shots
	{
		return flags & SHOT;
	}
	bool counter() const//is it counterattack?
	{
		return flags & COUNTER;
	}
	bool lucky() const
	{
		return flags & LUCKY;
	}
	bool unlucky() const
	{
		//TODO: support?
		return flags & UNLUCKY;
	}
	bool ballistaDoubleDmg() const //if it's ballista attack and does double dmg
	{
		return flags & BALLISTA_DOUBLE_DMG;
	}
	bool deathBlow() const
	{
		return flags & DEATH_BLOW;
	}
	//bool killed() //if target stack was killed
	//{
	//	return bsa.killed();
	//}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & bsa & stackAttacking & flags;
	}
};

struct StartAction : public CPackForClient//3007
{
	StartAction(){type = 3007;};
	StartAction(const BattleAction &act){ba = act; type = 3007;};
	void applyFirstCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	BattleAction ba;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ba;
	}
};

struct EndAction : public CPackForClient//3008
{
	EndAction(){type = 3008;};
	void applyCl(CClient *cl);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
	}
};

struct BattleSpellCast : public CPackForClient//3009
{
	BattleSpellCast(){type = 3009;};
	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	si32 dmgToDisplay; //this amount will be displayed as amount of damage dealt by spell
	ui8 side; //which hero did cast spell: 0 - attacker, 1 - defender
	ui32 id; //id of spell
	ui8 skill; //caster's skill level
	ui8 spellCost;
	ui8 manaGained; //mana channeling ability
	BattleHex tile; //destination tile (may not be set in some global/mass spells
	std::vector<ui32> resisted; //ids of creatures that resisted this spell
	std::set<ui32> affectedCres; //ids of creatures affected by this spell, generally used if spell does not set any effect (like dispel or cure)
	TCreature attackerType;//id of caster to generate console message; -1 if not set (eg. spell casted by artifact)
	ui8 castedByHero; //if true - spell has been casted by hero, otherwise by a creature
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & dmgToDisplay & side & id & skill & spellCost & manaGained & tile & resisted & affectedCres & attackerType & castedByHero;
	}
};

struct SetStackEffect : public CPackForClient //3010
{
	SetStackEffect(){type = 3010;};
	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	std::vector<ui32> stacks; //affected stacks (IDs)
	std::vector<Bonus> effect; //bonuses to apply
	std::vector<std::pair<ui32, Bonus> > uniqueBonuses; //bonuses per single stack
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stacks & effect & uniqueBonuses;
	}
};

struct StacksInjured : public CPackForClient //3011
{
	StacksInjured(){type = 3011;}
	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	std::vector<BattleStackAttacked> stacks;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stacks;
	}
};

struct BattleResultsApplied : public CPackForClient //3012
{
	BattleResultsApplied(){type = 3012;}

	ui8 player1, player2;

	void applyCl(CClient *cl);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player1 & player2;
	}
};

struct ObstaclesRemoved : public CPackForClient //3014
{
	ObstaclesRemoved(){type = 3014;}

	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	std::set<si32> obstacles; //uniqueIDs of removed obstacles

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & obstacles;
	}
};

struct CatapultAttack : public CPackForClient //3015
{
	CatapultAttack(){type = 3015;}

	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	std::set< std::pair< std::pair< ui8, si16 >, ui8> > attackedParts; // < <attackedPartOfWall, attacked hex >, damageDealt>
	//attackedPartOfWall; //[0] - keep, [1] - bottom tower, [2] - bottom wall, [3] - below gate, [4] - over gate, [5] - upper wall, [6] - uppert tower, [7] - gate;
	//damageDealt;

	int attacker; //if -1, then a spell caused this

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & attackedParts & attacker;
	}
};

struct BattleStacksRemoved : public CPackForClient //3016
{
	BattleStacksRemoved(){type = 3016;}

	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	std::set<ui32> stackIDs; //IDs of removed stacks

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stackIDs;
	}
};

struct BattleStackAdded : public CPackForClient //3017
{
	BattleStackAdded(){type = 3017;};

	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	int attacker; // if true, stack belongs to attacker
	int creID;
	int amount;
	int pos;
	int summoned; //if true, remove it afterwards

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & attacker & creID & amount & pos & summoned;
	}
};

struct BattleSetStackProperty : public CPackForClient //3018
{
	BattleSetStackProperty(){type = 3018;};

	enum BattleStackProperty {CASTS, ENCHANTER_COUNTER, UNBIND, CLONED};

	DLL_LINKAGE void applyGs(CGameState *gs);
	//void applyCl(CClient *cl){};

	int stackID;
	int which; //using enum values
	int val;
	int absolute;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stackID & which & val & absolute;
	}
};

struct BattleTriggerEffect : public CPackForClient //3019
{ //activated at the beginning of turn
	BattleTriggerEffect(){type = 3019;};

	DLL_LINKAGE void applyGs(CGameState *gs); //effect
	void applyCl(CClient *cl); //play animations & stuff

	//enum BattleEffect {REGENERATION, MANA_DRAIN, FEAR, MANA_CHANNELING, ENCHANTER, UNBIND, POISON, ENCHANTED, SUMMONER};

	int stackID;
	int effect; //use enumBattleEffect or corresponding Bonus type for sanity?
	int val;
	int additionalInfo;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stackID & effect & val & additionalInfo;
	}
};

struct ShowInInfobox : public CPackForClient //107
{
	ShowInInfobox(){type = 107;};
	ui8 player;
	Component c;
	MetaString text;

	void applyCl(CClient *cl);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player & c & text;
	}
};

struct AdvmapSpellCast : public CPackForClient //108
{
	AdvmapSpellCast(){type = 108;}
	const CGHeroInstance * caster;
	si32 spellID;

	void applyCl(CClient *cl);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & caster & spellID;
	}
};

/***********************************************************************************************************/

struct CommitPackage : public CPackForServer
{
	bool freePack; //for local usage, DO NOT serialize
	bool applyGh(CGameHandler *gh);
	CPackForClient *packToCommit;

	CommitPackage()
	{
		freePack = true;
	}
	~CommitPackage()
	{
		if(freePack)
			delete packToCommit;
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & packToCommit;
	}
};


struct CloseServer : public CPackForServer
{
	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{}
};

struct EndTurn : public CPackForServer
{
	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{}
};

struct DismissHero : public CPackForServer
{
	DismissHero(){};
	DismissHero(si32 HID) : hid(HID) {};
	si32 hid;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hid;
	}
};

struct MoveHero : public CPackForServer
{
	MoveHero(){};
	MoveHero(const int3 &Dest, si32 HID) : dest(Dest), hid(HID){};
	int3 dest;
	si32 hid;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & dest & hid;
	}
};

struct CastleTeleportHero : public CPackForServer
{
	CastleTeleportHero(){};
	CastleTeleportHero(const si32 HID, si32 Dest, ui8 Source ) : dest(Dest), hid(HID), source(Source){};
	si32 dest;
	si32 hid;
	si8 source;//who give teleporting, 1=castle gate

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & dest & hid;
	}
};

struct ArrangeStacks : public CPackForServer
{
	ArrangeStacks(){};
	ArrangeStacks(ui8 W, ui8 P1, ui8 P2, si32 ID1, si32 ID2, si32 VAL)
		:what(W),p1(P1),p2(P2),id1(ID1),id2(ID2),val(VAL) {};

	ui8 what; //1 - swap; 2 - merge; 3 - split
	ui8 p1, p2; //positions of first and second stack
	si32 id1, id2; //ids of objects with garrison
	si32 val;
	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & what & p1 & p2 & id1 & id2 & val;
	}
};

struct DisbandCreature : public CPackForServer
{
	DisbandCreature(){};
	DisbandCreature(ui8 Pos, si32 ID):pos(Pos),id(ID){};
	ui8 pos; //stack pos
	si32 id; //object id

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & pos & id;
	}
};

struct BuildStructure : public CPackForServer
{
	BuildStructure(){};
	BuildStructure(si32 TID, si32 BID):bid(BID),tid(TID){};
	si32 bid, tid; //structure and town ids

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid & bid;
	}
};
struct RazeStructure : public BuildStructure
{
	RazeStructure(){};
	//RazeStructure(si32 TID, si32 BID):bid(BID),tid(TID){};

	bool applyGh(CGameHandler *gh);
};
struct RecruitCreatures : public CPackForServer
{
	RecruitCreatures(){};
	RecruitCreatures(si32 TID, si32 CRID, si32 Amount, si32 Level):tid(TID),crid(CRID),amount(Amount),level(Level){};
	si32 tid; //town id
	ui32 crid, amount;//creature ID and amount
	si32 level;//dwelling level to buy from, -1 if any
	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid & crid & amount & level;
	}
};

struct UpgradeCreature : public CPackForServer
{
	UpgradeCreature(){};
	UpgradeCreature(ui8 Pos, si32 ID, si32 CRID):pos(Pos),id(ID), cid(CRID){};
	ui8 pos; //stack pos
	si32 id; //object id
	si32 cid; //id of type to which we want make upgrade

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & pos & id & cid;
	}
};

struct GarrisonHeroSwap : public CPackForServer
{
	GarrisonHeroSwap(){};
	GarrisonHeroSwap(si32 TID):tid(TID){};
	si32 tid; 

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid;
	}
};

struct ExchangeArtifacts : public CPackForServer
//TODO: allow exchange between heroes, stacks and commanders
{
	ArtifactLocation src, dst;
	ExchangeArtifacts(){};

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & src & dst;
	}
};

struct AssembleArtifacts : public CPackForServer
{
	AssembleArtifacts(){};
	AssembleArtifacts(si32 _heroID, ui16 _artifactSlot, bool _assemble, ui32 _assembleTo)
		: heroID(_heroID), artifactSlot(_artifactSlot), assemble(_assemble), assembleTo(_assembleTo){};
	si32 heroID;
	ui16 artifactSlot;
	bool assemble; // True to assemble artifact, false to disassemble.
	ui32 assembleTo; // Artifact to assemble into.

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroID & artifactSlot & assemble & assembleTo;
	}
};

struct BuyArtifact : public CPackForServer
{
	BuyArtifact(){};
	BuyArtifact(si32 HID, si32 AID):hid(HID),aid(AID){};
	si32 hid, aid; //hero and artifact id

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hid & aid;
	}
};

struct TradeOnMarketplace : public CPackForServer
{
	TradeOnMarketplace(){};

	const CGObjectInstance *market;
	const CGHeroInstance *hero; //needed when trading artifacts / creatures
	ui8 mode;//enum EEMarketMode
	ui32 r1, r2; //mode 0: r1 - sold resource, r2 - bought res (exception: when sacrificing art r1 is art id [todo: make r2 preferred slot?]
	ui32 val; //units of sold resource

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & market & hero & mode & r1 & r2 & val;
	}
};

struct SetFormation : public CPackForServer
{
	SetFormation(){};
	SetFormation(si32 HID, ui8 Formation):hid(HID),formation(Formation){};
	si32 hid;
	ui8 formation;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hid & formation;
	}
};

struct HireHero : public CPackForServer
{
	HireHero(){};
	HireHero(si32 HID, si32 TID):hid(HID),tid(TID){};
	si32 hid, tid; //available hero serial and town (tavern) id
	ui8 player;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hid & tid & player;
	}
};

struct BuildBoat : public CPackForServer 
{
	BuildBoat(){};
	si32 objid; //where player wants to buy a boat

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & objid;
	}

};

struct QueryReply : public CPackForServer
{
	QueryReply(){type = 6000;};
	QueryReply(ui32 QID, ui32 Answer):qid(QID),answer(Answer){type = 6000;};
	ui32 qid, answer; //hero and artifact id
	ui8 player;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & qid & answer & player;
	}
};

struct MakeAction : public CPackForServer
{
	MakeAction(){};
	MakeAction(const BattleAction &BA):ba(BA){};
	BattleAction ba;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ba;
	}
};

struct MakeCustomAction : public CPackForServer
{
	MakeCustomAction(){};
	MakeCustomAction(const BattleAction &BA):ba(BA){};
	BattleAction ba;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ba;
	}
};

struct DigWithHero : public CPackForServer
{
	DigWithHero(){}
	si32 id; //digging hero id

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id;
	}
};

struct CastAdvSpell : public CPackForServer
{
	CastAdvSpell(){}
	si32 hid; //hero id
	ui32 sid; //spell id
	int3 pos; //selected tile (not always used)

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hid & sid & pos;
	}
};

/***********************************************************************************************************/

struct SaveGame : public CPackForClient, public CPackForServer
{
	SaveGame(){};
	SaveGame(const std::string &Fname) :fname(Fname){};
	std::string fname;

	void applyCl(CClient *cl);
	void applyGs(CGameState *gs){};
	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & fname;
	}
};

struct PlayerMessage : public CPackForClient, public CPackForServer //513
{
	PlayerMessage(){CPackForClient::type = 513;};
	PlayerMessage(ui8 Player, const std::string &Text)
		:player(Player),text(Text)
	{CPackForClient::type = 513;};
	void applyCl(CClient *cl);
	void applyGs(CGameState *gs){};
	bool applyGh(CGameHandler *gh);

	ui8 player;
	std::string text;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & text & player;
	}
}; 


struct SetSelection : public CPackForClient, public CPackForServer //514
{
	SetSelection(){CPackForClient::type = 514;};
	DLL_LINKAGE void applyGs(CGameState *gs);
	bool applyGh(CGameHandler *gh);
	void applyCl(CClient *cl);

	ui8 player;
	ui32 id;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & player;
	}
};

struct CenterView : public CPackForClient//515
{
	CenterView(){CPackForClient::type = 515;};
	void applyCl(CClient *cl);

	ui8 player;
	int3 pos;
	ui32 focusTime; //ms

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & pos & player & focusTime;
	}
};

/***********************************************************************************************************/

struct CPackForSelectionScreen : public CPack
{
	void apply(CSelectionScreen *selScreen){}; //that functions are implemented in CPreGame.cpp
};

class CPregamePackToPropagate  : public CPackForSelectionScreen
{};

class CPregamePackToHost  : public CPackForSelectionScreen
{};

struct ChatMessage : public CPregamePackToPropagate
{
	std::string playerName, message;

	void apply(CSelectionScreen *selScreen);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & playerName & message;
	}
};

struct QuitMenuWithoutStarting : public CPregamePackToPropagate
{
	void apply(CSelectionScreen *selScreen); //that functions are implemented in CPreGame.cpp

	template <typename Handler> void serialize(Handler &h, const int version)
	{}
};

struct PlayerJoined : public CPregamePackToHost
{
	std::string playerName;
	ui8 connectionID;

	void apply(CSelectionScreen *selScreen); //that functions are implemented in CPreGame.cpp

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & playerName & connectionID;
	}
};

struct SelectMap : public CPregamePackToPropagate
{
	const CMapInfo *mapInfo;
	bool free;

	SelectMap(const CMapInfo &src)
	{
		mapInfo = &src;
		free = false;
	}
	SelectMap()
	{
		mapInfo = NULL;
		free = true;
	}
	~SelectMap()
	{
		if(free)
			delete mapInfo;
	}

	void apply(CSelectionScreen *selScreen); //that functions are implemented in CPreGame.cpp

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & mapInfo;
	}

};

struct UpdateStartOptions : public CPregamePackToPropagate
{
	StartInfo *options;
	bool free;

	void apply(CSelectionScreen *selScreen); //that functions are implemented in CPreGame.cpp

	UpdateStartOptions(StartInfo &src)
	{
		options = &src;
		free = false;
	}
	UpdateStartOptions()
	{
		options = NULL;
		free = true;
	}
	~UpdateStartOptions()
	{
		if(free)
			delete options;
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & options;
	}

};

struct PregameGuiAction : public CPregamePackToPropagate
{
	enum {NO_TAB, OPEN_OPTIONS, OPEN_SCENARIO_LIST};

	ui8 action;

	void apply(CSelectionScreen *selScreen); //that functions are implemented in CPreGame.cpp

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & action;
	}
};

struct RequestOptionsChange : public CPregamePackToHost
{
	enum {TOWN, HERO, BONUS};
	ui8 what;
	si8 direction; //-1 or +1 
	ui8 playerID;

	RequestOptionsChange(ui8 What, si8 Dir, ui8 Player)
		:what(What), direction(Dir), playerID(Player)
	{}
	RequestOptionsChange(){}

	void apply(CSelectionScreen *selScreen); //that functions are implemented in CPreGame.cpp

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & what & direction & playerID;
	}
};

struct PlayerLeft : public CPregamePackToPropagate
{
	ui8 playerID;

	void apply(CSelectionScreen *selScreen); //that functions are implemented in CPreGame.cpp

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & playerID;
	}
};

struct PlayersNames : public CPregamePackToPropagate
{
public:
	std::map<ui32, std::string> playerNames;

	void apply(CSelectionScreen *selScreen); //that functions are implemented in CPreGame.cpp

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & playerNames;
	}
};

struct StartWithCurrentSettings : public CPregamePackToPropagate
{
public:
	void apply(CSelectionScreen *selScreen); //that functions are implemented in CPreGame.cpp

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		//h & playerNames;
	}
};
