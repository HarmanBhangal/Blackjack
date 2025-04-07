export type ServerUpdate = {
    phase: UpdatePhase;
    gamblers: { [id: number]: Gambler };
    gameId: number;
    croupierCards: string[];
    croupierTotal: string;
    croupierBust: boolean;
    remainingTime: number;
    currentTurn: number;
    notification?: {
      message: string;
      status: NotificationStatus;
      closeAfter: number;
    };
  };
  
  enum UpdatePhase {
    WAGERING,
    IN_PLAY,
    COMPLETE,
    SERVER_OFFLINE
  }
  
  enum NotificationStatus {
    OK,
    ERROR,
    INFO,
  }
  
  export type GameInfo = {
    phase: UpdatePhase;
    remainingTime: number;
    croupierBust: boolean;
    currentTurn: number;
    gameId: number;
    otherGamblers?: { [id: number]: Gambler };
    notification?: {
      message: string;
      status: NotificationStatus;
      closeAfter: number;
    };
  };
  
  export type Gambler = {
    seatNumber: number; // a number 0 - 3
    displayName?: string;
    bet: number;
    hand: string[]; // e.g. "9S", "11H"
    funds: number;
    active: number; // marker for participation (0=playing, 1=waiting, etc.)
    busted: boolean;
    totalScore: string; // e.g. "11/17"
    outcome: number;
  };
  
  export type Croupier = {
    croupierCards: string[];
    totalScore: string;
  };
  
  export { UpdatePhase, NotificationStatus };
  