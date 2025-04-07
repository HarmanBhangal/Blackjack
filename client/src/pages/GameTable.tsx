import { useEffect, useState, useRef } from "react";
import { useTimer } from "../hooks/useTimer"; // new hook name
import { API_ENDPOINT, REFRESH_INTERVAL_MS } from "../constants/constants";
import axios from "axios";

// Updated types (from our renamed types file: serverUpdate.ts)
import { ServerUpdate, Gambler, GameInfo, Croupier } from "../types/serverUpdate";

// Updated components
import DecisionControls from "../components/DecisionControls";
import WagerSelector from "../components/WagerSelector";
import RivalHand from "../components/RivalHand";
import PingIndicator from "../components/PingIndicator";

function GameTable() {
  // Connection state
  const [isConnected, setIsConnected] = useState(false);
  const [dataLoaded, setDataLoaded] = useState(false);
  const [initialPollDone, setInitialPollDone] = useState(false);

  // Game state
  const [tableState, setTableState] = useState<GameInfo>({
    phase: 0,
    remainingTime: -1,
    croupierBust: false,
    currentTurn: 4,
    gameId: -1,
  });
  const remainingTimeRef = useRef(tableState.remainingTime);

  // Gambler (player) state
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

  // Croupier (dealer) state
  const [croupier, setCroupier] = useState<Croupier>({
    croupierCards: ["", ""],
    totalScore: "0",
  });

  // Seat positions (array of seat indices)
  const [seatPositions, setSeatPositions] = useState([-1, -1, -1, -1]);

  // Move/timer state for the gambler
  const [currentMove, setCurrentMove] = useState("STAND");
  const currentMoveRef = useRef(currentMove);
  currentMoveRef.current = currentMove;
  const [moveAllowed, setMoveAllowed] = useState(true);

  // Timeout flag
  const [inTimeout, setInTimeout] = useState(false);
  let timeoutHandle: NodeJS.Timer;

  // Result message for when game phase is finished
  const [resultMessage, setResultMessage] = useState<"dealer" | "you" | "tie" | "">("");

  // --- NEW: processChoice function ---  
  // This function handles the decision button clicks.
  const processChoice = (choice: string) => {
    if (choice === "CLEAR") {
      setGambler(prev => ({
        ...prev,
        funds: prev.funds + prev.bet,
        bet: 0,
      }));
    } else {
      setMoveAllowed(false);
      setCurrentMove(choice);
    }
  };

  // Poll the server for game updates at regular intervals
  useTimer(async () => {
    if (!isConnected || gamblerId === -1) return;
    console.log(`${API_ENDPOINT}/update/${gamblerId}`);
    const response = await axios.get(`${API_ENDPOINT}/update/${gamblerId}`);
    const update: ServerUpdate = response.data;
    console.log(update);

    remainingTimeRef.current = update.remainingTime;

    // Map other gamblers into rivalGamblers and update seat positions
    const rivalGamblers: { [key: number]: Gambler } = {};
    const newSeatPositions = [-1, -1, -1, -1];
    let idx = 0;
    for (let key in update.gamblers) {
      newSeatPositions[idx] = idx; // assign seat index
      if (parseInt(key) !== gamblerId) {
        rivalGamblers[idx] = update.gamblers[key];
      }
      idx++;
    }

    // Update croupier (dealer) state
    setCroupier({
      croupierCards: update.croupierCards,
      totalScore: update.croupierTotal,
    });

    // Update gambler state – if in wagering phase, preserve local funds and bet values
    const currentGambler = update.gamblers[gamblerId];
    if (update.phase === 0) {
      currentGambler.funds = gambler.funds;
      currentGambler.bet = gambler.bet;
    }
    setGambler(currentGambler);

    // Update table (game) state
    let newGameState: GameInfo = {
      phase: update.phase,
      remainingTime: update.remainingTime,
      croupierBust: update.croupierBust,
      currentTurn: update.currentTurn,
      gameId: update.gameId,
    };
    if (Object.keys(rivalGamblers).length > 0) {
      newGameState = { ...newGameState, otherGamblers: rivalGamblers };
    }
    setTableState(newGameState);
    setSeatPositions(newSeatPositions);
    setDataLoaded(true);

    // Determine result message if game phase is complete
    if (update.phase === 2) {
      if (currentGambler.outcome === 0) {
        setResultMessage("dealer");
      } else if (currentGambler.outcome === 1) {
        setResultMessage("you");
      } else {
        setResultMessage("tie");
      }
    } else {
      setResultMessage("");
    }

    if (!initialPollDone) setInitialPollDone(true);
  }, REFRESH_INTERVAL_MS);

  // Establish connection to the game server
  const initiateConnection = async () => {
    const connResponse = await axios.get(API_ENDPOINT + "/connect");
    console.log(connResponse.data);
    gamblerIdRef.current = connResponse.data.id;
    setGamblerId(connResponse.data.id);
  };

  // Effect for wagering phase: automatically post bet at timeout
  useEffect(() => {
    if (tableState.phase === 0 && !inTimeout && initialPollDone && remainingTimeRef.current > 1) {
      setInTimeout(true);
      timeoutHandle = setTimeout(async () => {
        setInTimeout(false);
        setMoveAllowed(true);
        await axios.post(API_ENDPOINT + `/action/${gamblerIdRef.current}`, {
          type: "BET",
          betAmount: betAmountRef.current,
        });
      }, (remainingTimeRef.current - 1) * 1000);
    }
  }, [tableState.phase, initialPollDone, tableState.remainingTime]);

  // Effect for playing phase: automatically post turn action at timeout
  useEffect(() => {
    if (
      tableState.phase === 1 &&
      tableState.currentTurn === gambler.seatNumber &&
      !inTimeout &&
      remainingTimeRef.current > 1
    ) {
      setInTimeout(true);
      timeoutHandle = setTimeout(async () => {
        setInTimeout(false);
        setMoveAllowed(true);
        await axios.post(API_ENDPOINT + `/action/${gamblerId}`, {
          type: "TURN",
          action: currentMoveRef.current,
        });
      }, (remainingTimeRef.current - 1) * 1000);
    }
  }, [gambler.totalScore, tableState.currentTurn]);

  // On mount, establish connection to the game server
  useEffect(() => {
    setIsConnected(false);
    initiateConnection();
    setIsConnected(true);
  }, []);

  return (
    <div className="w-96 max-w-sm h-screen flex items-center flex-col">
      {(!isConnected && !dataLoaded) ? (
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
                      className={`text-3xl text-primary absolute -top-8 -right-0 ${resultMessage === "you" ? "line-through" : ""}`}
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
                {seatPositions.map((seat, idx) => {
                  if (seat === gambler.seatNumber) {
                    return (
                      <div
                        key={idx}
                        className="spot w-20 h-32 border-solid border-slate-100 rounded-md border-4 opacity-75 bg-slate-800 text-white flex justify-center items-center"
                      >
                        {seat === tableState.currentTurn ? (
                          <PingIndicator topOffset="-top-6" rightOffset="right-[40%]" />
                        ) : null}
                        YOUR PLACE
                      </div>
                    );
                  } else if (seat !== -1) {
                    return (
                      <div className="spot" key={idx}>
                        {seat === tableState.currentTurn ? (
                          <PingIndicator topOffset="top-4" rightOffset="right-[50%]" />
                        ) : null}
                        {tableState.phase === 0 ? (
                          <div className="w-20 h-32 border-solid border-slate-100 rounded-md border-4 opacity-75 bg-slate-800 text-white flex justify-center items-center">
                            RIVALS
                          </div>
                        ) : (
                          <RivalHand cards={tableState.otherGamblers ? tableState.otherGamblers[seat]["hand"] : [""]} />
                        )}
                      </div>
                    );
                  } else {
                    return (
                      <div key={idx} className="spot w-20 h-32 border-solid border-slate-100 rounded-md border-4 opacity-50"></div>
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
                  <WagerSelector updateGambler={setGambler} />
                </>
              ) : (
                <>
                  <div id="gamblerHand" className="flex justify-center mt-14 relative scale-75 flex-wrap w-[350px]">
                    <h1 className={`text-3xl text-primary absolute -top-8 -right-0 ${resultMessage === "dealer" ? "line-through" : ""}`}>
                      {gambler.totalScore}
                    </h1>
                    {gambler.active === 1 ? (
                      <div className="flex justify-center items-center text-2xl">
                        WAITING FOR NEXT ROUND...
                      </div>
                    ) : (
                      <>
                        {gambler.hand.map((cardStr, index) => {
                          const parts = cardStr.split("");
                          const suit = parts.pop();
                          const num = parts.join("");
                          const extraStyle = index > 2 ? "absolute sendToAbove -bottom-32" : "";
                          const imagePath = `/src/assets/cards/card${suit}.png`;
                          return (
                            <div
                              key={index}
                              style={{ backgroundImage: `url(${imagePath})` }}
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
                    {resultMessage !== "" ? (
                      <h1 className="text-primary text-4xl absolute -top-10 z-50 animate-getAlertWinner">
                        {resultMessage} {resultMessage === "tie" ? "" : "win"} {resultMessage === "you" ? ":)" : ":("}
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
                  <DecisionControls processChoice={processChoice} mode="IN_PLAY" isEnabled={tableState.currentTurn === gambler.seatNumber && moveAllowed && gambler.active === 0} />
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
