import { useEffect, useState, useRef } from "react";
import { useInterval } from "../hooks/useInterval";
import { API_ENDPOINT, REFRESH_INTERVAL_MS } from "../constants/constants";
import axios from "axios";

// Types
import { ServerUpdate, Gambler, GameInfo, Croupier } from "../types/serverUpdate";

// Components
import DecisionControls from "../components/Controls";
import BetSelector from "../components/Betting";
import RivalHand from "../components/OpponentsHand";
import PingIndicator from "../components/PingAnimation";

function GameTable() {
  const [connectionEstablished, setConnectionEstablished] = useState(false);
  const [dataLoaded, setDataLoaded] = useState(false);
  const [initialPollCompleted, setInitialPollCompleted] = useState(false);

  const [tableState, setTableState] = useState<GameInfo>({
    phase: 0,
    remainingTime: -1,
    croupierBust: false,
    currentTurn: 4,
    gameId: -1,
  });
  const gamblerTimeRef = useRef(tableState.remainingTime);

  const [gamblerId, setGamblerId] = useState(-1);
  const gamblerIdRef = useRef(gamblerId);
  const [gambler, setGambler] = useState<Gambler>({
    seatNumber: 4, // above valid range initially
    bet: 0,
    hand: [""],
    funds: 200,
    active: 1,
    busted: false,
    totalScore: "0",
    outcome: 0
  });
  const betAmountRef = useRef(gambler.bet);
  betAmountRef.current = gambler.bet;

  const [croupier, setCroupier] = useState<Croupier>({
    croupierCards: ["", ""],
    totalScore: "0",
  });

  const [positions, setPositions] = useState([-1, -1, -1, -1]);

  const [currentMove, setCurrentMove] = useState("STAND");
  const currentMoveRef = useRef(currentMove);
  currentMoveRef.current = currentMove;
  const [moveAllowed, setMoveAllowed] = useState(true);

  const [inDelay, setInDelay] = useState(false);

  let delayTimer: NodeJS.Timer;

  const [resultWinner, setResultWinner] = useState<"dealer" | "you" | "tie" | "">("");

  const processChoice = (choice: string) => {
    if (choice === "CLEAR") {
      setGambler(prev => ({ ...prev, funds: prev.funds + prev.bet, bet: 0 }));
    } else {
      setMoveAllowed(false);
      setCurrentMove(choice);
    }
  };

  useInterval(async () => {
    if (!connectionEstablished || gamblerId === -1) return;
    console.log(`${API_ENDPOINT}/update/${gamblerId}`);
    const resp = await axios.get(`${API_ENDPOINT}/update/${gamblerId}`);
    const update: ServerUpdate = resp.data;
    console.log(update);

    gamblerTimeRef.current = update.remainingTime;

    const rivalGamblers: { [key: number]: Gambler } = {};
    const tempPositions = [-1, -1, -1, -1];
    let idx = 0;
    for (let key in update.gamblers) {
      tempPositions[idx] = idx;
      if (parseInt(key) !== gamblerId) {
        rivalGamblers[idx] = update.gamblers[key];
      }
      idx++;
    }

    setCroupier({
      croupierCards: update.croupierCards,
      totalScore: update.croupierTotal,
    });

    const currentGambler = update.gamblers[gamblerId];
    if (update.phase === 0) {
      currentGambler.funds = gambler.funds;
      currentGambler.bet = gambler.bet;
    }
    setGambler(currentGambler);

    let newTableState: GameInfo = {
      phase: update.phase,
      remainingTime: update.remainingTime,
      croupierBust: update.croupierBust,
      currentTurn: update.currentTurn,
      gameId: update.gameId,
    };

    if (Object.keys(rivalGamblers).length > 0) {
      newTableState = { ...newTableState, otherGamblers: rivalGamblers };
    }
    setTableState(newTableState);

    setPositions(tempPositions);
    setDataLoaded(true);

    if (update.phase === 2) {
      if (currentGambler.outcome === 0) {
        setResultWinner("dealer");
      } else if (currentGambler.outcome === 1) {
        setResultWinner("you");
      } else {
        setResultWinner("tie");
      }
    } else {
      setResultWinner("");
    }

    if (!initialPollCompleted) setInitialPollCompleted(true);
  }, REFRESH_INTERVAL_MS);

  const initiateConnection = async () => {
    const connData = await axios.get(API_ENDPOINT + "/connect");
    console.log(connData.data);
    gamblerIdRef.current = connData.data.id;
    setGamblerId(connData.data.id);
  };

  useEffect(() => {
    if (tableState.phase === 0 && !inDelay && initialPollCompleted && gamblerTimeRef.current > 1) {
      setInDelay(true);
      delayTimer = setTimeout(async () => {
        setInDelay(false);
        setMoveAllowed(true);
        await axios.post(API_ENDPOINT + `/action/${gamblerIdRef.current}`, {
          type: "BET",
          betAmount: betAmountRef.current,
        });
      }, (gamblerTimeRef.current - 1) * 1000);
    }
  }, [tableState.phase, initialPollCompleted, tableState.remainingTime]);

  useEffect(() => {
    if (tableState.phase === 1 && tableState.currentTurn === gambler.seatNumber && !inDelay && gamblerTimeRef.current > 1) {
      setInDelay(true);
      delayTimer = setTimeout(async () => {
        setInDelay(false);
        setMoveAllowed(true);
        await axios.post(API_ENDPOINT + `/action/${gamblerId}`, {
          type: "TURN",
          action: currentMoveRef.current,
        });
      }, (gamblerTimeRef.current - 1) * 1000);
    }
  }, [gambler.totalScore, tableState.currentTurn]);

  useEffect(() => {
    setConnectionEstablished(false);
    initiateConnection();
    setConnectionEstablished(true);
  }, []);

  return (
    <div className="w-96 max-w-sm h-screen flex items-center flex-col">
      {(!connectionEstablished && !dataLoaded) ? (
        <div>connecting to the table</div>
      ) : (
        <>
          {tableState.phase === 3 ? (
            <>server closed</>
          ) : (
            <>
              <div className="absolute left-2 top-2 opacity-25 text-2xl">
                Table: {tableState.gameId} <br />
                Time Remaining: {tableState.remainingTime}
              </div>
              {tableState.phase === 0 ? (
                <>
                  <h1 className="text-6xl font-bold mb-8 mt-8">WAGER TIME...</h1>
                  <h1 className="text-2xl font-bold mb-32">
                    select a chip to place your wager
                  </h1>
                </>
              ) : (
                <>
                  <h1 className="text-3xl text-primary">CROUPIER</h1>
                  <div
                    id="croupierHand"
                    className="flex justify-center mt-14 relative scale-[0.8] flex-wrap w-[300px]"
                  >
                    <h1
                      className={`text-3xl text-primary absolute -top-8 -right-0 ${
                        resultWinner === "you" ? "line-through" : ""
                      }`}
                    >
                      {tableState.phase === 2 ? croupier.totalScore : '---'}
                    </h1>
                    {croupier.croupierCards.map((cardStr, idx) => {
                      const parts = cardStr.split("");
                      const symbol = parts.pop();
                      const num = parts.join("");
                      if (tableState.phase === 1) {
                        const hiddenStyle =
                          idx === 0
                            ? "bg-[url('/src/assets/cards/cardBack.png')] -rotate-6 -mt-3"
                            : "bg-[url('/src/assets/cards/card.png')]";
                        return (
                          <div
                            key={idx}
                            className={`flex justify-center items-center text-4xl text-[#6D5C5C] bg-cover w-24 h-36 -ml-6 ${hiddenStyle}`}
                          >
                            {idx !== 0 ? num : ""}
                          </div>
                        );
                      } else {
                        const offsetStyle = idx > 2 ? `absolute left-${idx > 3 ? 12 : 6} top-28` : "";
                        const animateStyle = idx > 1 ? "animate-getCard" : "";
                        return (
                          <div
                            key={idx}
                            className={`flex justify-center items-center text-4xl text-[#6D5C5C] bg-[url('/src/assets/cards/card.png')] bg-cover w-24 h-36 -ml-6 ${animateStyle} ${offsetStyle}`}
                          >
                            {num}
                          </div>
                        );
                      }
                    })}
                  </div>
                  <div className="bg-[url('/src/assets/cards/dealerShadow.png')] bg-cover bg-no-repeat w-40 h-4 mt-10 opacity-25 blur-sm"></div>
                </>
              )}

              <div id="tableArea" className="flex absolute bottom-64">
                {positions.map((pos, index) => {
                  if (pos === gambler.seatNumber) {
                    return (
                      <div
                        key={index}
                        className="spot w-20 h-32 border-solid border-slate-100 rounded-md border-4 opacity-75 bg-slate-800 text-white flex justify-center items-center"
                      >
                        {pos === tableState.currentTurn ? (
                          <PingIndicator topOffset="-top-6" rightOffset="right-[40%]" />
                        ) : null}
                        YOUR PLACE
                      </div>
                    );
                  } else if (pos !== -1) {
                    return (
                      <div className="spot" key={index}>
                        {pos === tableState.currentTurn ? (
                          <PingIndicator topOffset="top-4" rightOffset="right-[50%]" />
                        ) : null}
                        {tableState.phase === 0 ? (
                          <div className="w-20 h-32 border-solid border-slate-100 rounded-md border-4 opacity-75 bg-slate-800 text-white flex justify-center items-center">
                            RIVALS
                          </div>
                        ) : (
                          <RivalHand
                            cards={
                              tableState.otherGamblers
                                ? tableState.otherGamblers[pos]["hand"]
                                : [""]
                            }
                          />
                        )}
                      </div>
                    );
                  } else {
                    return (
                      <div
                        key={index}
                        className="spot w-20 h-32 border-solid border-slate-100 rounded-md border-4 opacity-50"
                      ></div>
                    );
                  }
                })}
              </div>

              {tableState.phase === 0 ? (
                <>
                  <div className="bg-[url('/src/assets/poker_chip_bg.png')] bg-no-repeat bg-cover w-48 h-48 opacity-50 flex justify-center items-center">
                    {gambler.bet > 0 ? (
                      <h1 className="text-2xl">{gambler.bet}</h1>
                    ) : (
                      <h1 className="text-2xl">0</h1>
                    )}
                  </div>
                  <BetSelector updateGambler={setGambler} />
                </>
              ) : (
                <>
                  <div
                    id="gamblerHand"
                    className="flex justify-center mt-14 relative scale-75 flex-wrap w-[350px]"
                  >
                    <h1
                      className={`text-3xl text-primary absolute -top-8 -right-0 ${
                        resultWinner === "dealer" ? "line-through" : ""
                      }`}
                    >
                      {gambler.totalScore}
                    </h1>
                    {gambler.active === 1 ? (
                      <div className="flex justify-center items-center text-2xl">
                        WAITING FOR NEXT ROUND...
                      </div>
                    ) : (
                      <>
                        {gambler.hand.map((cardStr, idx) => {
                          let parts = cardStr.split("");
                          const suit = parts.pop();
                          const num = parts.join("");
                          const extraStyle = idx > 2 ? "absolute sendToAbove -bottom-32" : "";
                          const imgPath = `/src/assets/cards/card${suit}.png`;
                          return (
                            <div
                              key={idx}
                              style={{ backgroundImage: `url(${imgPath})` }}
                              className={`flex justify-center items-center text-6xl text-[#6D5C5C] bg-cover w-[7rem] h-[11rem] -ml-6 animate-getCard -rotate-2 ${extraStyle}`}
                            >
                              {num}
                            </div>
                          );
                        })}
                      </>
                    )}
                    {parseInt(gambler.totalScore) > 21 ? (
                      <h1 className="text-red-500 text-4xl absolute top-0 z-50 animate-getAlert">
                        exceeded
                      </h1>
                    ) : null}
                    {resultWinner !== "" ? (
                      <h1 className="text-primary text-4xl absolute -top-10 z-50 animate-getAlertWinner">
                        {resultWinner} {resultWinner === "tie" ? "" : "win"} {resultWinner === "you" ? ":)" : ":("}
                      </h1>
                    ) : null}
                  </div>
                  <div className="bg-[url('/src/assets/cards/yourShadow.png')] bg-cover blur-sm opacity-25 bg-no-repeat w-60 h-10 mt-10 relative scale-[0.8] md:scale-100 -z-50"></div>
                </>
              )}

              <div className="mt-14">
                <div className="w-full h-[6px] rounded-full bg-loading opacity-50 mb-4 relative"></div>
                {tableState.phase === 0 ? (
                  <DecisionControls processChoice={processChoice} mode="WAGERING" />
                ) : (
                  <DecisionControls
                    processChoice={processChoice}
                    mode="IN_PLAY"
                    isEnabled={
                      tableState.currentTurn === gambler.seatNumber && moveAllowed && gambler.active === 0
                    }
                  />
                )}
              </div>

              <div className="absolute right-8 bottom-8 w-64 h-16 border-2 rounded-md flex justify-center flex-col p-4 opacity-50">
                <h1 className="text-2xl">{"WAGER: " + gambler.bet}</h1>
                <h1 className="text-2xl">{"FUNDS: " + gambler.funds}</h1>
              </div>
            </>
          )}
        </>
      )}
    </div>
  );
}

export default GameTable;
