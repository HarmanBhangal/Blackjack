export interface CardAttributes {
    points: number;
    display: string | number;
  }
  
  export class DeckCard {
    private attributes: CardAttributes;
  
    get points() {
      return this.attributes.points;
    }
  
    get display() {
      return this.attributes.display;
    }
  
    constructor() {
      const randNum = Math.floor(Math.random() * 13) + 1;
      let effectivePoints = randNum > 10 ? 10 : randNum;
      let face: string | number = effectivePoints;
  
      if (randNum === 1) {
        face = "A";
      } else if (randNum === 11) {
        face = "J";
      } else if (randNum === 12) {
        face = "Q";
      } else if (randNum === 13) {
        face = "K";
      }
  
      this.attributes = { points: effectivePoints, display: face };
    }
  }
  