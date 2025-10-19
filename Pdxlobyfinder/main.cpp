#include <iostream>
#include <Windows.h>
#include <string>
#include <winhttp.h>
#include <vector>
#include "sdk/public/steam/steam_api.h"
#include "sdk/public/steam/isteamuserstats.h"
#include "sdk/public/steam/isteamremotestorage.h"
#include "sdk/public/steam/isteammatchmaking.h"
#include "sdk/public/steam/steam_gameserver.h"
#include "sdk/public/steam/isteamfriends.h"

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "steam_api.lib")
#pragma comment(lib, "steam_api64")

struct Player {

};


struct LobbyJoinRequest {
    CCallResult<LobbyJoinRequest, LobbyEnter_t> callResult;
    CSteamID lobbyID;

    void OnLobbyEnter(LobbyEnter_t* pCallback, bool bIOFailure) {
        if (!bIOFailure && pCallback->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess) {
            // Lobby Name
            char key[k_nMaxLobbyKeyLength];
            char value[k_cubChatMetadataMax];
            std::string lobbyName = "Unknown";
            std::string serverID = "Unknown";
            int numPlayers = SteamMatchmaking()->GetNumLobbyMembers(lobbyID);

            if (numPlayers - 1 >= 0) {
                int nData = SteamMatchmaking()->GetLobbyDataCount(lobbyID);
                for (int i = 0; i < nData; i++) {
                    if (SteamMatchmaking()->GetLobbyDataByIndex(lobbyID, i, key, k_nMaxLobbyKeyLength, value, k_cubChatMetadataMax)) {
                        if (strcmp(key, "name") == 0) lobbyName = value;

                        if (strcmp(key, "__gameserverSteamID") == 0) serverID = value;
                    }
                }
                

                std::cout << "\nLobby Name: " << lobbyName << "\n";
                std::cout << "\Server ID: " << serverID << "\n";
                //std::string command = "Test";
                //SteamMatchmaking()->SendLobbyChatMsg(lobbyID, command.c_str(), command.size() + 1);
                // Get your own SteamID
                CSteamID mySteamID = SteamUser()->GetSteamID();

                // List players excluding yourself

                std::cout << "Players (" << numPlayers - 1 << "):\n";
                for (int j = 0; j < numPlayers; j++) {
                    CSteamID memberID = SteamMatchmaking()->GetLobbyMemberByIndex(lobbyID, j);
                    if (memberID == mySteamID) continue;

                    const char* playerName = SteamFriends()->GetFriendPersonaName(memberID);
                    std::cout << "- " << (playerName ? playerName : "Unknown")
                        << " (" << memberID.ConvertToUint64() << ")\n";
                }
            }
        }
        else {
            std::cout << "Failed to join lobby.\n";
        }

        // Leave the lobby after reading info
        SteamMatchmaking()->LeaveLobby(lobbyID);
        delete this; // clean up
    }
};

class LobbyHandler {
public:
    CCallResult<LobbyHandler, LobbyMatchList_t> m_CallResultLobbyMatchList;
    std::vector<LobbyJoinRequest*> pendingJoins;

    void FindLobbies() {
        SteamMatchmaking()->AddRequestLobbyListResultCountFilter(50); // max per request
        SteamMatchmaking()->AddRequestLobbyListDistanceFilter(k_ELobbyDistanceFilterWorldwide);
        //SteamMatchmaking()->AddRequestLobbyListStringFilter("version", "Countenance v1.16.9.8bdd (c09b)", k_ELobbyComparisonEqual);
        //SteamMatchmaking()->AddRequestLobbyListStringFilter("password", "0", k_ELobbyComparisonEqual);
        SteamMatchmaking()->AddRequestLobbyListStringFilter("desc", "hoi4", k_ELobbyComparisonEqual);
        SteamMatchmaking()->AddRequestLobbyListStringFilter("mod", "hoi4", k_ELobbyComparisonEqual);
        //SteamMatchmaking()->AddRequestLobbyListStringFilter("name", "! ! Join for Complain --->", k_ELobbyComparisonNotEqual);
        SteamAPICall_t hSteamAPICall = SteamMatchmaking()->RequestLobbyList();
        m_CallResultLobbyMatchList.Set(hSteamAPICall, this, &LobbyHandler::OnLobbyMatchList);
    }

    void OnLobbyMatchList(LobbyMatchList_t* pCallback, bool bIOFailure) {
        if (bIOFailure) {
            std::cout << "Failed to get lobby list.\n";
            return;
        }

        int totalLobbies = pCallback->m_nLobbiesMatching;
        std::cout << "Found " << totalLobbies << " HOI4 lobbies.\n";

        for (int i = 0; i < totalLobbies; i++) {
            CSteamID lobbyID = SteamMatchmaking()->GetLobbyByIndex(i);

            // Create a join request for each lobby
            LobbyJoinRequest* request = new LobbyJoinRequest();
            request->lobbyID = lobbyID;

            SteamAPICall_t hCall = SteamMatchmaking()->JoinLobby(lobbyID);
            request->callResult.Set(hCall, request, &LobbyJoinRequest::OnLobbyEnter);

            pendingJoins.push_back(request);
        }
    }
};

int main() {
    if (!SteamAPI_Init()) {
        std::cout << "SteamAPI_Init() failed. Make sure Steam is running!\n";
        return -1;
    }

    LobbyHandler handler;

    std::cout << "Searching for HOI4 lobbies...\n";
    handler.FindLobbies();

    // Keep running Steam callbacks
    while (true) {
        SteamAPI_RunCallbacks();
        Sleep(16); // ~60 FPS
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            std::cout << "\n\n\nSearching for HOI4 lobbies...\n";
            handler.FindLobbies();
        }

    }

    SteamAPI_Shutdown();
    return 0;
}