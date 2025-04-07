export type GameDecision = {
    category: DecisionCategory;
    stake?: number;
    move?: MoveChoice;
  };
  
  enum DecisionCategory {
    WAGER,
    PLAY,
  }
  
  enum MoveChoice {
    DRAW,
    HOLD,
  }
  
  export { DecisionCategory, MoveChoice };
  