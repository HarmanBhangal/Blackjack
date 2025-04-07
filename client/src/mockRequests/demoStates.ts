export const wagerPhase = {
    status: 0,
    gamblers: {
      1: {
        seatNumber: 3,
        bet: 0,
        hand: [""],
        funds: 200,
        active: true,
        hasWon: false,
        totalScore: "0",
      },
    },
    croupierCards: [""],
    croupierTotal: "0",
    croupierBust: false,
    remainingTime: 10,
    currentTurn: 1,
  };
  
  export const inPlayPhase = {
    status: 1,
    gamblers: {
      1: {
        seatNumber: 2,
        bet: 25,
        hand: ["10H", "8H"],
        funds: 200,
        active: true,
        hasWon: false,
        totalScore: "18",
      },
      2: {
        seatNumber: 3,
        bet: 25,
        hand: ["10H", "8H"],
        funds: 200,
        active: true,
        hasWon: false,
        totalScore: "18",
      },
    },
    croupierCards: ["10H", "8H"],
    croupierTotal: "18",
    croupierBust: false,
    remainingTime: 10,
    currentTurn: 1,
  };
  
  export const sampleGameState = {
    status: 1,
    remainingTime: 0,
    croupierBust: false,
    currentTurn: 3,
    otherGamblers: {
      1: {
        seatNumber: 1,
        bet: 25,
        hand: ["9H", "9H"],
        funds: 200,
        active: true,
        hasWon: false,
        totalScore: "18",
      },
      2: {
        seatNumber: 2,
        bet: 25,
        hand: ["8H", "9H"],
        funds: 200,
        active: true,
        hasWon: false,
        totalScore: "18",
      },
      3: {
        seatNumber: 3,
        bet: 25,
        hand: ["7H", "8H"],
        funds: 200,
        active: true,
        hasWon: false,
        totalScore: "18",
      },
      4: {
        seatNumber: 4,
        bet: 25,
        hand: ["6H"],
        funds: 200,
        active: true,
        hasWon: false,
        totalScore: "18",
      },
    },
  };
  