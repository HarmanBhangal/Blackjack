// GameServer.cpp - Updated with new naming conventions

#include <iostream>
#include "ThreadWrapper.h"       // New base thread wrapper header
#include "NetSocketServer.h"     // New socket server header
#include "PosixSemaphore.h"      // New semaphore header
#include "DataBuffer.h"          // New data buffer header (replacing ByteArray)
#include "NetSocket.h"           // New socket header (replacing Socket)
#include <stdlib.h>
#include <time.h>
#include <tuple>
#include <list>
#include <vector>
#include <algorithm>
#include <jsoncpp/json/json.h>

using namespace SyncTools;

// Global server instance and player counter
NetSocketServer *serverInstance;
int globalPlayerID = 0;
const int MAX_PLAYERS = 4;

// -------------------- Card and Helper Functions --------------------
struct NewCard {
    std::string suit;
    int numericValue;
    std::string toString() const {
        std::string result;
        switch(numericValue) {
            case 1: result = "A"; break;
            case 11: result = "J"; break;
            case 12: result = "Q"; break;
            case 13: result = "K"; break;
            default: result = std::to_string(numericValue); break;
        }
        return result + suit;
    }
};

int randomBetween(int min, int max) {
    return (rand() % (max - min + 1)) + min;
}

NewCard createRandomCard() {
    NewCard card;
    std::string suits[] = {"D", "H", "S", "C"};
    card.suit = suits[randomBetween(0, 3)];
    card.numericValue = randomBetween(1, 13);
    return card;
}

std::vector<NewCard> dealNewCards(int count, std::vector<NewCard> dealtCards) {
    std::vector<NewCard> cards;
    for (int i = 0; i < count; i++) {
        int duplicateCount = 6;
        NewCard newCard;
        while (duplicateCount == 6) {
            newCard = createRandomCard();
            duplicateCount = 0;
            for (int j = 0; j < dealtCards.size(); j++) {
                if (dealtCards[j].numericValue == newCard.numericValue &&
                    dealtCards[j].suit == newCard.suit) {
                    duplicateCount++;
                }
            }
        }
        cards.push_back(newCard);
        dealtCards.push_back(newCard);
    }
    return cards;
}

Json::Value convertCardsToJson(const std::vector<NewCard> &cards) {
    Json::Value arr(Json::arrayValue);
    for (auto card : cards) {
        arr.append(card.toString());
    }
    return arr;
}

std::vector<int> computeCardSum(const std::vector<NewCard> &cards) {
    std::vector<int> totals = {0, 0};
    bool hasAce = false;
    for (auto card : cards) {
        if (card.numericValue == 1) {
            hasAce = true;
            totals[1] = totals[0] + 10;
        }
        totals[0] += std::min(card.numericValue, 10);
        if (hasAce)
            totals[1] += std::min(card.numericValue, 10);
    }
    return totals;
}

std::string formatCardSumValue(const std::vector<int> &sums) {
    return (sums[1] != 0 ? std::to_string(sums[0]) + "/" + std::to_string(sums[1])
                         : std::to_string(sums[0]));
}

bool isBustedHand(const std::vector<NewCard> &cards) {
    std::vector<int> totals = computeCardSum(cards);
    if (totals[1] == 0)
        return totals[0] > 21;
    else
        return totals[0] > 21 && totals[1] > 21;
}

bool checkTurnCompletion(const std::vector<NewCard> &cards, int maxValue) {
    std::vector<int> totals = computeCardSum(cards);
    return (totals[0] == maxValue || totals[1] == maxValue ||
            (totals[1] == 0 && totals[0] > maxValue) ||
            (*std::min_element(totals.begin(), totals.end())) > maxValue);
}

int optimalTotal(const std::vector<NewCard> &cards) {
    std::vector<int> totals = computeCardSum(cards);
    return (totals[0] > totals[1] && totals[0] <= 21) ? totals[0] : totals[1];
}

// -------------------- Player and Game State Structures --------------------
struct PlayerDetails {
    int id;
    int seat;
    int wager;
    std::vector<NewCard> hand;
    int balance;
    int activeStatus;
    bool turnComplete;
    int result; // 1: win, 0: lose, 2: tie

    PlayerDetails(int pid, int seatNo, int bet, std::vector<NewCard> cards, int bal, int active, bool complete)
        : id(pid), seat(seatNo), wager(bet), hand(cards), balance(bal),
          activeStatus(active), turnComplete(complete), result(0) { }

    Json::Value toJson() {
        Json::Value obj(Json::objectValue);
        obj["seat"] = seat;
        obj["bet"] = wager;
        obj["cards"] = convertCardsToJson(hand);
        obj["balance"] = balance;
        obj["isActive"] = activeStatus;
        obj["cardSum"] = formatCardSumValue(computeCardSum(hand));
        obj["isBusted"] = isBustedHand(hand);
        obj["hasWon"] = result;
        return obj;
    }
};

struct GameStatus {
    int gameID;
    std::vector<PlayerDetails*> players;
    std::vector<NewCard> dealtCards;
    std::vector<NewCard> dealerHand;
    int phase; // 0: betting, 1: playing, 2: finished
    int timeLeft;
    int currentSeatPlaying;
    Json::Value broadcastState;

    GameStatus(int id, int ph, int timeRem, int seat)
        : gameID(id), phase(ph), timeLeft(timeRem), currentSeatPlaying(seat) { }

    int getPlayerCount() {
        return players.size();
    }

    int getActivePlayerCount() {
        int count = 0;
        for (auto p : players)
            count += (p->activeStatus == 0 ? 1 : 0);
        return count;
    }

    void updatePlayerSeats() {
        for (int i = 0; i < players.size(); i++) {
            players[i]->seat = i;
        }
    }

    void addPlayer(PlayerDetails *newPlayer) {
        PosixSemaphore sem("mutex", 1, true);
        sem.wait();
        players.push_back(newPlayer);
        updatePlayerSeats();
        sem.signal();
        std::cout << "Seat: " << players[0]->seat << std::endl;
    }

    void removePlayer(int removeID) {
        for (auto it = players.begin(); it != players.end(); ++it) {
            if ((*it)->id == removeID) {
                players.erase(it);
                break;
            }
        }
        updatePlayerSeats();
        std::cout << "Removing player " << removeID << " from Game#" << gameID << std::endl;
    }
};

std::vector<GameStatus*> gameSessions;

Json::Value playersJson(const std::vector<PlayerDetails*> &players) {
    Json::Value out(Json::objectValue);
    for (auto p : players)
        out[std::to_string(p->id)] = p->toJson();
    return out;
}

// -------------------- Dealer Thread Wrapper --------------------
class DealerThreadWrapper : public ThreadWrapper {
private:
    int refreshSec;
    int sessionIndex;
public:
    DealerThreadWrapper(int idx) : ThreadWrapper(1000), refreshSec(1), sessionIndex(idx) {
        start();
    }
    void updateBroadcastState() {
        gameSessions[sessionIndex]->broadcastState["gameID"] = gameSessions[sessionIndex]->gameID;
        gameSessions[sessionIndex]->broadcastState["dealerCards"] = convertCardsToJson(gameSessions[sessionIndex]->dealerHand);
        gameSessions[sessionIndex]->broadcastState["hasDealerBusted"] = isBustedHand(gameSessions[sessionIndex]->dealerHand);
        gameSessions[sessionIndex]->broadcastState["status"] = gameSessions[sessionIndex]->phase;
        gameSessions[sessionIndex]->broadcastState["timeRemaining"] = gameSessions[sessionIndex]->timeLeft;
        gameSessions[sessionIndex]->broadcastState["currentPlayerTurn"] = gameSessions[sessionIndex]->currentSeatPlaying;
        gameSessions[sessionIndex]->broadcastState["dealerSum"] = formatCardSumValue(computeCardSum(gameSessions[sessionIndex]->dealerHand));
        gameSessions[sessionIndex]->broadcastState["players"] = playersJson(gameSessions[sessionIndex]->players);
    }
    virtual long ThreadMain(void) override {
        PosixSemaphore broadcastSem("broadcast");
        PosixSemaphore lockSem("mutex", 1, true);
        while (true) {
            sleep(refreshSec);
            lockSem.wait();
            // Decrease time if players exist
            gameSessions[sessionIndex]->timeLeft -= (gameSessions[sessionIndex]->getPlayerCount() > 0 ? refreshSec : 0);
            bool advanceSeat = (gameSessions[sessionIndex]->phase == 1 &&
                                gameSessions[sessionIndex]->currentSeatPlaying < gameSessions[sessionIndex]->getActivePlayerCount() &&
                                gameSessions[sessionIndex]->players[gameSessions[sessionIndex]->currentSeatPlaying]->turnComplete);
            if (gameSessions[sessionIndex]->timeLeft <= 0 || advanceSeat) {
                if (advanceSeat)
                    gameSessions[sessionIndex]->currentSeatPlaying++;
                if (gameSessions[sessionIndex]->phase == 0) {
                    if (gameSessions[sessionIndex]->getPlayerCount() > 0) {
                        gameSessions[sessionIndex]->phase = 1;
                        if (gameSessions[sessionIndex]->dealtCards.size() >= 156)
                            gameSessions[sessionIndex]->dealtCards.clear();
                        gameSessions[sessionIndex]->dealerHand = dealNewCards(2, gameSessions[sessionIndex]->dealtCards);
                        for (int i = 0; i < gameSessions[sessionIndex]->getPlayerCount(); i++) {
                            if (gameSessions[sessionIndex]->players[i]->activeStatus == 1)
                                gameSessions[sessionIndex]->players[i]->activeStatus = 0;
                            if (gameSessions[sessionIndex]->players[i]->activeStatus == 0)
                                gameSessions[sessionIndex]->players[i]->hand = dealNewCards(2, gameSessions[sessionIndex]->dealtCards);
                            else if (gameSessions[sessionIndex]->players[i]->activeStatus == 2)
                                gameSessions[sessionIndex]->removePlayer(gameSessions[sessionIndex]->players[i]->id);
                        }
                        gameSessions[sessionIndex]->currentSeatPlaying = 0;
                    } else {
                        gameSessions[sessionIndex]->currentSeatPlaying = 0;
                        gameSessions[sessionIndex]->phase = 0;
                        gameSessions[sessionIndex]->dealerHand.clear();
                    }
                }
                else if (gameSessions[sessionIndex]->phase == 1 &&
                         gameSessions[sessionIndex]->currentSeatPlaying >= gameSessions[sessionIndex]->getPlayerCount()) {
                    while (!checkTurnCompletion(gameSessions[sessionIndex]->dealerHand, 17))
                        gameSessions[sessionIndex]->dealerHand.push_back(createRandomCard());
                    bool dealerBust = isBustedHand(gameSessions[sessionIndex]->dealerHand);
                    for (int i = 0; i < gameSessions[sessionIndex]->getPlayerCount(); i++) {
                        bool playerBust = isBustedHand(gameSessions[sessionIndex]->players[i]->hand);
                        if (!playerBust && (dealerBust || (optimalTotal(gameSessions[sessionIndex]->players[i]->hand) > optimalTotal(gameSessions[sessionIndex]->dealerHand)))) {
                            gameSessions[sessionIndex]->players[i]->result = 1;
                            gameSessions[sessionIndex]->players[i]->balance += gameSessions[sessionIndex]->players[i]->wager * 2;
                        }
                        else if (playerBust || (!dealerBust && (optimalTotal(gameSessions[sessionIndex]->players[i]->hand) < optimalTotal(gameSessions[sessionIndex]->dealerHand)))) {
                            gameSessions[sessionIndex]->players[i]->result = 0;
                            gameSessions[sessionIndex]->players[i]->balance -= gameSessions[sessionIndex]->players[i]->wager;
                        } else {
                            gameSessions[sessionIndex]->players[i]->result = 2;
                            gameSessions[sessionIndex]->players[i]->balance += gameSessions[sessionIndex]->players[i]->wager;
                        }
                    }
                    gameSessions[sessionIndex]->phase = 2;
                } else if (gameSessions[sessionIndex]->phase == 2) {
                    gameSessions[sessionIndex]->currentSeatPlaying = 0;
                    gameSessions[sessionIndex]->phase = 0;
                    gameSessions[sessionIndex]->dealerHand.clear();
                    for (int i = 0; i < gameSessions[sessionIndex]->getPlayerCount(); i++) {
                        if (gameSessions[sessionIndex]->players[i]->balance <= 0)
                            gameSessions[sessionIndex]->removePlayer(gameSessions[sessionIndex]->players[i]->id);
                        gameSessions[sessionIndex]->players[i]->turnComplete = false;
                        gameSessions[sessionIndex]->players[i]->wager = 0;
                        gameSessions[sessionIndex]->players[i]->hand.clear();
                    }
                }
                gameSessions[sessionIndex]->timeLeft = 10;
            }
            updateBroadcastState();
            lockSem.signal();
            for (int i = 0; i < gameSessions[sessionIndex]->getPlayerCount(); i++) {
                broadcastSem.signal();
            }
        }
        return 0;
    }
};

// -------------------- Player Reader Thread --------------------
class PlayerReaderThread : public ThreadWrapper {
private:
    int playerID;
    int sessionIndex;
public:
    NetSocket socketConn;
    PlayerReaderThread(NetSocket &sock, int pid, int idx)
        : ThreadWrapper(1000), socketConn(sock), sessionIndex(idx) {
        playerID = pid;
        start();
    }
    virtual long ThreadMain(void) override {
        std::cout << "Player reader thread started." << std::endl;
        PosixSemaphore broadcast("broadcast");
        Json::Value initMsg(Json::objectValue);
        initMsg["playerID"] = playerID;
        initMsg["gameState"] = gameSessions[sessionIndex]->broadcastState;
        SyncTools::DataBuffer initBuffer(initMsg.toStyledString());
        socketConn.writeData(initBuffer);
        while (true) {
            sleep(1);
            if (!socketConn.isOpenStatus())
                break;
            SyncTools::DataBuffer outBuffer(gameSessions[sessionIndex]->broadcastState.toStyledString());
            socketConn.writeData(outBuffer);
        }
        return 0;
    }
};

// -------------------- Player Writer Thread --------------------
class PlayerWriterThread : public ThreadWrapper {
private:
    int playerID;
    int sessionIndex;
public:
    NetSocket socketConn;
    PlayerDetails data;
    PlayerWriterThread(NetSocket &sock, int pid, int idx)
        : ThreadWrapper(1000), socketConn(sock),
          data(pid, 0, 0, std::vector<NewCard>(), 200, 1, false), sessionIndex(idx) {
        playerID = pid;
        start();
    }
    virtual long ThreadMain(void) override {
        std::cout << "Player writer thread started for Game #" << gameSessions[sessionIndex]->gameID << std::endl;
        PosixSemaphore mutex("mutex");
        PlayerDetails *pdata = &data;
        gameSessions[sessionIndex]->addPlayer(pdata);
        while (true) {
            SyncTools::DataBuffer *buffer = new SyncTools::DataBuffer();
            if (socketConn.readData(*buffer) == 0) {
                std::cout << "Player-" << data.id << " left Game #" << gameSessions[sessionIndex]->gameID << std::endl;
                data.activeStatus = 2;
                break;
            }
            mutex.wait();
            std::string req = buffer->toString();
            Json::Value actionMsg(Json::objectValue);
            Json::Reader reader;
            reader.parse(req, actionMsg);
            if (actionMsg["type"].asString() == "TURN") {
                std::string act = actionMsg["action"].asString();
                std::cout << act << std::endl;
                if (act == "HIT") {
                    data.hand.push_back(createRandomCard());
                    data.turnComplete = checkTurnCompletion(data.hand, 21);
                } else {
                    data.turnComplete = true;
                }
            } else {
                int betAmt = actionMsg["betAmount"].asInt();
                data.balance -= betAmt;
                data.wager = betAmt;
            }
            mutex.signal();
        }
        return 0;
    }
};

// -------------------- Server Input Thread (Graceful Termination) --------------------
class ServerInputThread : public ThreadWrapper {
public:
    std::string input;
    ServerInputThread() : ThreadWrapper(1000) {
        start();
    }
    virtual long ThreadMain(void) override {
        while (true) {
            std::cin >> input;
            if (input == "done") {
                delete serverInstance;
                std::cout << "Closing the server." << std::endl;
                exit(0);
            }
        }
        return 0;
    }
};

// -------------------- Main Function --------------------
int main(int argc, char* argv[]) {
    std::cout << "----- C++ Game Server -----" << std::endl;
    int port = (argc >= 2) ? std::stoi(argv[1]) : 2000;
    serverInstance = new NetSocketServer(port);
    int curGameID = 0;
    // Create the first game session
    GameStatus *firstGame = new GameStatus(curGameID++, 0, 10, 0);
    gameSessions.push_back(firstGame);
    // Start the dealer thread for session 0
    DealerThreadWrapper dealerThread(0);
    PosixSemaphore mutex("mutex", 1, true);
    PosixSemaphore broadcast("broadcast", 0, true);
    bool playerJoined = false;
    std::cout << "Socket listening on " << port << std::endl;
    ServerInputThread *terminationInput = new ServerInputThread();
    srand(time(0));
    while (true) {
        try {
            NetSocket sock = serverInstance->acceptConnection();
            int gameIndex;
            playerJoined = false;
            for (int i = 0; i < gameSessions.size(); i++) {
                if (gameSessions[i]->getPlayerCount() < MAX_PLAYERS) {
                    gameIndex = i;
                    playerJoined = true;
                    break;
                }
            }
            if (!playerJoined) {
                std::cout << "All games were full. Creating a new game";
                GameStatus *newGame = new GameStatus(curGameID++, 0, 10, 0);
                gameSessions.push_back(newGame);
                gameIndex = curGameID - 1;
                while (gameSessions[gameIndex] == nullptr)
                    std::cout << ".";
                std::cout << "Test" << std::endl;
                DealerThreadWrapper *newDealer = new DealerThreadWrapper(gameIndex);
            }
            globalPlayerID++;
            std::cout << "GameID: " << gameIndex << std::endl;
            PlayerReaderThread *reader = new PlayerReaderThread(sock, globalPlayerID, gameIndex);
            PlayerWriterThread *writer = new PlayerWriterThread(sock, globalPlayerID, gameIndex);
        } catch (std::string err) {
            if (err == "Unexpected error in the server") {
                std::cout << "Server is terminated" << std::endl;
                break;
            }
        }
    }
    return 0;
}
