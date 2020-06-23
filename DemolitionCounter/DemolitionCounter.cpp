#include "pch.h"
#include "DemolitionCounter.h"
#include <iostream>
#include <fstream>
#include "bakkesmod/wrappers/GameObject/Stats/StatEventWrapper.h"

BAKKESMOD_PLUGIN(DemolitionCounter, "Counts demolitions in online games", plugin_version, PLUGINTYPE_THREADED)

bool enabledCounter;
int demos = 0;
int gameDemos = 0;
int exterms = 0;
int gameExterms = 0;
int games = 0;

void DemolitionCounter::onLoad()
{
	cvarManager->log("Plugin loaded!");
	auto enabledVar = cvarManager->registerCvar("counter_enabled", "0", "activate/deactivate demolition counter");

	updateEnabled(enabledVar.getBoolValue());
	enabledVar.addOnValueChanged([this](std::string, CVarWrapper cvar) { updateEnabled(cvar.getBoolValue()); });

	auto startDemos = cvarManager->registerCvar("counter_set_demos", "0", "sets demolition value");

	demos = startDemos.getIntValue();
	startDemos.addOnValueChanged([this](std::string, CVarWrapper cvar) { demos = cvar.getIntValue(); writeDemos(); });

	auto startExterms = cvarManager->registerCvar("counter_set_exterms", "0", "sets extermination value");

	exterms = startExterms.getIntValue();
	startExterms.addOnValueChanged([this](std::string, CVarWrapper cvar) { exterms = cvar.getIntValue(); writeExterms(); });

	writeAll();
}

void DemolitionCounter::updateEnabled(bool enabled) {
	enabledCounter = enabled;

	if (enabled) {
		gameWrapper->HookEventWithCaller<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage", 
			std::bind(&DemolitionCounter::statEvent, this, std::placeholders::_1, std::placeholders::_2));

		gameWrapper->HookEventPost("Function Engine.PlayerInput.InitInputSystem", std::bind(&DemolitionCounter::startGame, this));
	}
	else {
		gameWrapper->UnhookEvent("Function TAGame.GFxHUD_TA.HandleStatTickerMessage");
		gameWrapper->UnhookEvent("Function Engine.PlayerInput.InitInputSystem");
	}
}

struct TheArgStruct
{
	uintptr_t Receiver;
	uintptr_t Victim;
	uintptr_t StatEvent;
};

void DemolitionCounter::statEvent(ServerWrapper caller, void* args) {
	auto tArgs = (TheArgStruct*)args;
	cvarManager->log("stat event!");

	auto victim = PriWrapper(tArgs->Victim);
	auto receiver = PriWrapper(tArgs->Receiver);
	auto statEvent = StatEventWrapper(tArgs->StatEvent);
	auto label = statEvent.GetLabel();
	cvarManager->log(label.ToString());

	if (!DemolitionCounter::isPrimaryPlayer(receiver)) {
		return;
	}

	// if the event is a demo
	if (label.ToString().compare("Demolition") == 0) {
		DemolitionCounter::demolition();
		return;
	}

	// if event is an exterm
	if (label.ToString().compare("Extermination") == 0) {
		DemolitionCounter::extermination();
		return;
	}
}

bool DemolitionCounter::isPrimaryPlayer(PriWrapper receiver) {
	ServerWrapper sw = gameWrapper->GetOnlineGame();

	if (sw.IsNull()) {
		cvarManager->log("null server");
		return false;
	}

	auto primary = sw.GetLocalPrimaryPlayer();

	if (primary.IsNull()) {
		cvarManager->log("null primary player");
		return false;
	}

	auto primaryPri = primary.GetPRI();

	if (primaryPri.IsNull()) {
		cvarManager->log("null primary pri");
		return false;
	}

	auto receiverID = receiver.GetUniqueId();
	auto primaryID = primaryPri.GetUniqueId();

	return receiverID.ID == primaryID.ID;
}

void DemolitionCounter::demolition() {
	cvarManager->log("main player demo");
	demos++;
	gameDemos++;
	cvarManager->log(std::to_string(demos));

	DemolitionCounter::writeDemos();
}

void DemolitionCounter::extermination() {
	cvarManager->log("main player exterm");
	exterms++;
	gameExterms++;
	cvarManager->log(std::to_string(exterms));

	DemolitionCounter::writeExterms();
}

void DemolitionCounter::writeDemos() {
	std::ofstream demoFile;
	demoFile.open("./DemolitionCounter/demolitions.txt");
	demoFile << std::to_string(demos);
	demoFile.close();

	std::ofstream gameDemoFile;
	gameDemoFile.open("./DemolitionCounter/gameDemolitions.txt");
	gameDemoFile << std::to_string(gameDemos);
	gameDemoFile.close();
}

void DemolitionCounter::writeExterms() {
	std::ofstream extermFile;
	extermFile.open("./DemolitionCounter/exterminations.txt");
	extermFile << std::to_string(exterms);
	extermFile.close();

	std::ofstream gameExtermFile;
	gameExtermFile.open("./DemolitionCounter/gameExterminations.txt");
	gameExtermFile << std::to_string(gameExterms);
	gameExtermFile.close();
}

void DemolitionCounter::writeAll() {
	writeDemos();
	writeExterms();
	writeGames();
}

void DemolitionCounter::writeGames() {
	std::ofstream gameFile;
	gameFile.open("./DemolitionCounter/games.txt");
	gameFile << std::to_string(games);
	gameFile.close();
}

void DemolitionCounter::startGame() {
	cvarManager->log("started game");
	gameExterms = 0;
	gameDemos = 0;
	games++;
	writeDemos();
	writeExterms();
	writeGames();
}

void DemolitionCounter::onUnload()
{
}