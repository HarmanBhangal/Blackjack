#include "ThreadWrapper.h"
#include "NetSocketServer.h"
#include "PosixSemaphore.h"
#include <stdlib.h>
#include <time.h>
#include <tuple>
#include <list>
#include <vector>
#include <algorithm>
#include <jsoncpp/json/json.h>

using namespace SyncTools;

NetSocketServer * serverInstance;
int globalPlayerID = 0;
const int MAX_PLAYERS = 4;

struct Card {
    std::string suit;
    int value;
    std::string toString() const {
        std::string res;
        switch (value) {
            case 1: res = "A"; break;
            case 11: res = "J"; break;
            case 12: res = "Q"; break;
            case 13: res = "K"; break;
            default: res = std::to_string(value); break;
        }
        return res + suit;
    }
};

int randomBetween(int min, int max) {
    return (rand() % (max - min + 1)) + min;
}

Card generateRandomCard() {
    Card c;
    std::string suits[] = {"D", "H", "S", "C"};
    c.suit = suits[randomBetween(0, 3)];
    c.value = randomBetween(1, 13);
    return c;
}

std::vector<Card> dealCards(int count, std::vector<Card> dealtCards) {
    std::vector<Card> cards;
    for (int i = 0; i < count; i++) {
        int dupCount = 6;
        Card newCard;
        while (dupCount == 6) {
            newCard = generateRandomCard();
            dupCount = 0;
            for (int j = 0; j < dealtCards.size(); j++) {
                if (dealtCards[j].value == newCard.value && dealtCards[j].suit == newCard.suit)
                    dupCount++;
            }
        }
        cards.push_back(newCard);
        dealtCards.push_back(newCard);
    }
    return cards;
}

Json::Value cardsToJson(const std::vector<Card> &cards) {
    Json::Value arr(Json::arrayValue);
    for (auto c : cards) {
        arr.append(c.toString());
    }
    return arr;
}

std::vector<int> sumCards(const std::vector<Card> &cards) {
    std::vector<int> totals = {0, 0};
    bool hasAce = false;
    for (auto c : cards) {
        if (c.value == 1) {
            hasAce = true;
            totals[1] = totals[0] + 10;
        }
        totals[0] += std::min(c.value, 10);
        if (hasAce) totals[1] += std::min(c.value, 10);
    }
    return totals;
}

std::string formatSum(const std::vector<int> &sums) {
    return (sums[1] != 0 ? std::to_string(sums[0]) + "/" + std::to_string(sums[1]) : std::to_string(sums[0]));
}

bool checkBusted(const std::vector<Card> &cards) {
    std::vector<int> totals = sumCards(cards);
    if (totals[1] == 0)
        return totals[0] > 21;
    else
        return totals[0] > 21 && totals[1] > 21;
}

bool isTurnComplete(const std::vector<Card> &cards, int maxVal) {
    std::vector<int> totals = sumCards(cards);
    return (totals[0] == maxVal || totals[1] == maxVal ||
            (totals[1] == 0 && totals[0] > maxVal) ||
            (*std::min_element(totals.begin(), totals.end())) > maxVal);
}

int optimalTotal(const std::vector<Card> &cards) {
    std::vector<int> totals = sumCards(cards);
    return (totals[0] > totals[1] && totals[0] <= 21) ? totals[0] : totals[1];
}

struct PlayerInfo {
    int id;
    int seat;
    int wager;
    std::vector<Card> hand;
    int balance;
    int activeStatus;
    bool turnComplete;
    int result;
    
    PlayerInfo(int pid, int seatNo, int bet, std::vector<Card> cards, int bal, int active, bool complete)
        : id(pid), seat(seatNo), wager(bet), hand(cards), balance(bal), activeStatus(active), turnComplete(complete), result(0) { }
    
    Json::Value toJson() {
        Json::Value obj(Json::objectValue);
        obj["seat"] = seat;
        obj["bet"] = wager;
        obj["cards"] = cardsToJson(hand);
        obj["balance"] = balance;
        obj["isActive"] = activeStatus;
        obj["cardSum"] = formatSum(sumCards(hand));
        obj["isBusted"] = checkBusted(hand);
        obj["hasWon"] = result;
        return obj;
    }
};

struct GameState {
    int gameID;
    std::vector<PlayerInfo*> players;
    std::vector<Card> dealt;
    std::vector<Card> dealerHand;
    int phase; // 0: betting, 1: playing, 2: finished
    int timeLeft;
    int currentSeat;
    Json::Value broadcast;
    
    GameState(int id, int ph, int timeRem, int seat)
        : gameID(id), phase(ph), timeLeft(timeRem), currentSeat(seat) { }
        
    int playerCount() {
        return players.size();
    }
    
    int activePlayerCount() {
        int count = 0;
        for (auto p : players)
            count += (p->activeStatus == 0 ? 1 : 0);
        return count;
    }
    
    void updateSeats() {
        for (int i = 0; i < players.size(); i++) {
            players[i]->seat = i;
        }
    }
    
    void addPlayer(PlayerInfo * newPlayer) {
        PosixSemaphore mutex("mutex", 1, true);
        mutex.wait();
        players.push_back(newPlayer);
        updateSeats();
        mutex.signal();
        std::cout << "Seat: " << players[0]->seat << std::endl;
    }
    
    void removePlayer(int removeID) {
        for (auto it = players.begin(); it != players.end(); ++it) {
            if ((*it)->id == removeID) {
                players.erase(it);
                break;
            }
        }
        updateSeats();
        std::cout << "Removing player " << removeID << " from Game#" << gameID << std::endl;
    }
};

std::vector<GameState*> games;

Json::Value playersToJson(const std::vector<PlayerInfo*> &players) {
    Json::Value result(Json::objectValue);
    for (auto p : players)
        result[std::to_string(p->id)] = p->toJson();
    return result;
}

class DealerThreadWrapper : public ThreadWrapper {
private:
    int refreshInterval;
    int gameIndex;
public:
    DealerThreadWrapper(int idx) : ThreadWrapper(1000), refreshInterval(1), gameIndex(idx) {
        start();
    }
    void updateBroadcast() {
        games[gameIndex]->broadcast["gameID"] = games[gameIndex]->gameID;
        games[gameIndex]->broadcast["dealerCards"] = cardsToJson(games[gameIndex]->dealerHand);
        games[gameIndex]->broadcast["hasDealerBusted"] = checkBusted(games[gameIndex]->dealerHand);
        games[gameIndex]->broadcast["status"] = games[gameIndex]->phase;
        games[gameIndex]->broadcast["timeRemaining"] = games[gameIndex]->timeLeft;
        games[gameIndex]->broadcast["currentPlayerTurn"] = games[gameIndex]->currentSeat;
        games[gameIndex]->broadcast["dealerSum"] = formatSum(sumCards(games[gameIndex]->dealerHand));
        games[gameIndex]->broadcast["players"] = playersToJson(games[gameIndex]->players);
    }
    virtual long ThreadMain(void) override {
        PosixSemaphore broadcastSem("broadcast");
        PosixSemaphore mutex("mutex", 1, true);
        while(true) {
            sleep(refreshInterval);
            mutex.wait();
            games[gameIndex]->timeLeft -= (games[gameIndex]->playerCount() > 0 ? refreshInterval : 0);
            bool advanceTurn = (games[gameIndex]->phase == 1 &&
                                games[gameIndex]->currentSeat < games[gameIndex]->activePlayerCount() &&
                                games[gameIndex]->players[games[gameIndex]->currentSeat]->turnComplete);
            if (games[gameIndex]->timeLeft <= 0 || advanceTurn) {
                if (advanceTurn)
                    games[gameIndex]->currentSeat++;
                if (games[gameIndex]->phase == 0) {
                    if (games[gameIndex]->playerCount() > 0) {
                        games[gameIndex]->phase = 1;
                        if (games[gameIndex]->dealt.size() >= 156)
                            games[gameIndex]->dealt.clear();
                        games[gameIndex]->dealerHand = dealCards(2, games[gameIndex]->dealt);
                        for (int i = 0; i < games[gameIndex]->playerCount(); i++) {
                            if (games[gameIndex]->players[i]->activeStatus == 1)
                                games[gameIndex]->players[i]->activeStatus = 0;
                            if (games[gameIndex]->players[i]->activeStatus == 0)
                                games[gameIndex]->players[i]->hand = dealCards(2, games[gameIndex]->dealt);
                            else if (games[gameIndex]->players[i]->activeStatus == 2)
                                games[gameIndex]->removePlayer(games[gameIndex]->players[i]->id);
                        }
                        games[gameIndex]->currentSeat = 0;
                    } else {
                        games[gameIndex]->currentSeat = 0;
                        games[gameIndex]->phase = 0;
                        games[gameIndex]->dealerHand.clear();
                    }
                }
                else if (games[gameIndex]->phase == 1 &&
                         games[gameIndex]->currentSeat >= games[gameIndex]->playerCount()) {
                    while(!isTurnComplete(games[gameIndex]->dealerHand, 17))
                        games[gameIndex]->dealerHand.push_back(generateRandomCard());
                    
                    bool dealerBusted = checkBusted(games[gameIndex]->dealerHand);
                    for (int i = 0; i < games[gameIndex]->playerCount(); i++) {
                        bool playerBusted = checkBusted(games[gameIndex]->players[i]->hand);
                        if (!playerBusted && (dealerBusted || (optimalTotal(games[gameIndex]->players[i]->hand) > optimalTotal(games[gameIndex]->dealerHand)))) {
                            games[gameIndex]->players[i]->result = 1;
                            games[gameIndex]->players[i]->balance += games[gameIndex]->players[i]->wager * 2;
                        }
                        else if (playerBusted || (!dealerBusted && (optimalTotal(games[gameIndex]->players[i]->hand) < optimalTotal(games[gameIndex]->dealerHand)))) {
                            games[gameIndex]->players[i]->result = 0;
                            games[gameIndex]->players[i]->balance -= games[gameIndex]->players[i]->wager;
                        } else {
                            games[gameIndex]->players[i]->result = 2;
                            games[gameIndex]->players[i]->balance += games[gameIndex]->players[i]->wager;
                        }
                    }
                    games[gameIndex]->phase = 2;
                } else if (games[gameIndex]->phase == 2) {
                    games[gameIndex]->currentSeat = 0;
                    games[gameIndex]->phase = 0;
                    games[gameIndex]->dealerHand.clear();
                    for (int i = 0; i < games[gameIndex]->playerCount(); i++) {
                        if (games[gameIndex]->players[i]->balance <= 0)
                            games[gameIndex]->removePlayer(i);
                        games[gameIndex]->players[i]->turnComplete = false;
                        games[gameIndex]->players[i]->wager = 0;
                        games[gameIndex]->players[i]->hand.clear();
                    }
                }
                games[gameIndex]->timeLeft = 10;
            }
            updateBroadcast();
            mutex.signal();
            for (int i = 0; i < games[gameIndex]->playerCount(); i++) {
                broadcastSem.signal();
            }
        }
        return 0;
    }
};

class PlayerReaderThread : public ThreadWrapper {
private:
    int playerID;
    int gameIdx;
public:
    NetSocket socketConn;
    PlayerReaderThread(NetSocket & sock, int pid, int idx)
      : ThreadWrapper(1000), socketConn(sock), gameIdx(idx) {
        playerID = pid;
        start();
    }
    virtual long ThreadMain(void) override {
        std::cout << "Player reader thread started." << std::endl;
        PosixSemaphore broadcastSem("broadcast");
        Json::Value initMsg(Json::objectValue);
        initMsg["playerID"] = playerID;
        DataBuffer initBuffer(initMsg.toStyledString());
        socketConn.writeData(initBuffer);
        while(true) {
            sleep(1);
            if (!socketConn.isOpenStatus())
                break;
            DataBuffer outBuffer(games[gameIdx]->broadcast.toStyledString());
            socketConn.writeData(outBuffer);
        }
        return 0;
    }
};

class PlayerWriterThread : public ThreadWrapper {
private:
    int playerID;
    int gameIdx;
public:
    NetSocket socketConn;
    PlayerInfo playerData;
    PlayerWriterThread(NetSocket & sock, int pid, int idx)
      : ThreadWrapper(1000), socketConn(sock),
        playerData(pid, 0, 0, std::vector<Card>(), 200, 1, false), gameIdx(idx) {
        playerID = pid;
        start();
    }
    virtual long ThreadMain(void) override {
        std::cout << "Player writer thread started for Game #" << games[gameIdx]->gameID << std::endl;
        PosixSemaphore mutex("mutex");
        PlayerInfo * pdata = &playerData;
        games[gameIdx]->addPlayer(pdata);
        while (true) {
            DataBuffer * buffer = new DataBuffer();
            if (socketConn.readData(*buffer) == 0) {
                std::cout << "Player-" << playerData.id << " left Game #" << games[gameIdx]->gameID << std::endl;
                playerData.activeStatus = 2;
                break;
            }
            mutex.wait();
            std::string req = buffer->toString();
            Json::Value actionMsg;
            Json::Reader reader;
            reader.parse(req, actionMsg);
            if (actionMsg["type"].asString() == "TURN") {
                std::string act = actionMsg["action"].asString();
                std::cout << act << std::endl;
                if (act == "HIT") {
                    playerData.hand.push_back(generateRandomCard());
                    playerData.turnComplete = isTurnComplete(playerData.hand, 21);
                } else {
                    playerData.turnComplete = true;
                }
            } else {
                int betAmount = actionMsg["betAmount"].asInt();
                playerData.balance -= betAmount;
                playerData.wager = betAmount;
            }
            mutex.signal();
        }
        return 0;
    }
};

class ServerInputThread : public ThreadWrapper {
public:
    std::string userInput;
    ServerInputThread() : ThreadWrapper(1000) { start(); }
    virtual long ThreadMain(void) override {
        while (true) {
            std::cin >> userInput;
            if (userInput == "done") {
                delete serverInstance;
                std::cout << "Closing the server." << std::endl;
                exit(0);
            }
        }
        return 0;
    }
};

int main(int argc, char* argv[]) {
    std::cout << "----- C++ Game Server -----" << std::endl;
    int port = (argc >= 2) ? std::stoi(argv[1]) : 2000;
    serverInstance = new NetSocketServer(port);
    int currentGameID = 0;
    GameState * initialGame = new GameState(currentGameID++, 0, 10, 0);
    games.push_back(initialGame);
    DealerThreadWrapper dealerThread(0);
    PosixSemaphore mutex("mutex", 1, true);
    PosixSemaphore broadcastSem("broadcast", 0, true);
    bool playerJoined = false;
    std::cout << "Socket listening on " << port << std::endl;
    ServerInputThread * inputThread = new ServerInputThread();
    srand(time(0));
    while (true) {
        try {
            NetSocket sock = serverInstance->acceptConnection();
            int gameIdx;
            playerJoined = false;
            for (int i = 0; i < games.size(); i++) {
                if (games[i]->playerCount() < MAX_PLAYERS) {
                    gameIdx = i;
                    playerJoined = true;
                    break;
                }
            }
            if (!playerJoined) {
                std::cout << "All games full. Creating a new game." << std::endl;
                GameState * newGame = new GameState(currentGameID++, 0, 10, 0);
                games.push_back(newGame);
                gameIdx = currentGameID - 1;
                while(games[gameIdx] == nullptr)
                    std::cout << ".";
                DealerThreadWrapper * newDealer = new DealerThreadWrapper(gameIdx);
            }
            globalPlayerID++;
            std::cout << "GameID: " << gameIdx << std::endl;
            PlayerReaderThread * reader = new PlayerReaderThread(sock, globalPlayerID, gameIdx);
            PlayerWriterThread * writer = new PlayerWriterThread(sock, globalPlayerID, gameIdx);
        } catch (std::string err) {
            if (err == "Unexpected error in socket server") {
                std::cout << "Server terminated" << std::endl;
                break;
            }
        }
    }
}
